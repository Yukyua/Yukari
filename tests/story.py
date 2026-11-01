"""Opt-in original-game story, menu, save and reload integration test.

python3 tests/story.py --game-dir /path/to/game --exe build/yukari
Both arguments are required to run. Game assets remain read-only and all state,
action files and screenshots are created in a temporary directory.
"""
import argparse
import os
import re
from pathlib import Path
import subprocess
import tempfile
import unittest

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--game-dir', type=Path)
parser.add_argument('--exe', type=Path)
parser.add_argument('--gpu', action='store_true', help='Require real Metal rendering; needs a macOS display session')
options = parser.parse_args()
if bool(options.game_dir) != bool(options.exe):
    parser.error('--game-dir and --exe must be supplied together')

# Reuse the asset-independent standard-library PNG reader. title.py accepts the
# same two opt-in arguments; importing it neither runs tests nor starts a game.
from title import png, rgb


def text_crop(image):
    return [bytes(rgb(image, x, y)) for y in range(457, 547) for x in range(235, 970)]


def dialogue_mask(crop):
    # Dialogue is near-white, while the original window/background is tinted.
    # Compare glyphs separately so animated background grain does not make a
    # correct text restore fail a whole-RGB screenshot comparison.
    return [min(pixel) > 200 and max(pixel) - min(pixel) < 30 for pixel in crop]


@unittest.skipUnless(options.game_dir and options.exe, 'requires explicit --game-dir and --exe')
class StoryMenus(unittest.TestCase):
    def test_story_menus_and_cross_process_reload(self):
        game, executable = options.game_dir.resolve(), options.exe.resolve()
        originals = sorted(game.glob('*.exe')) + sorted(game.glob('data*.ykc')) + sorted((game / 'save').glob('*'))
        originals = [p for p in originals if p.is_file()]
        before = {path: (path.stat().st_size, path.stat().st_mtime_ns) for path in originals}
        names = {path.name for path in game.iterdir()}
        with tempfile.TemporaryDirectory(prefix='yukari-story-') as directory:
            root = Path(directory)
            state = root / 'state'
            environment = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy',
                               SDL_RENDER_DRIVER='software')

            if options.gpu:
                environment = dict(os.environ, SDL_AUDIODRIVER='dummy', SDL_RENDER_DRIVER='metal', SDL_VIDEO_HIGHDPI_DISABLED='1')
                environment.pop('SDL_VIDEODRIVER', None)

            def play(name, actions, extra=()):
                action_file = root / f'{name}.actions'
                action_file.write_text('\n'.join(actions) + '\n')
                command = [str(executable), 'play', str(game), '--fast', '--mute',
                           '--state-dir', str(state), '--max-ms', '180000', '--seed', '0',
                           '--actions', str(action_file), *extra]
                if options.gpu: command.append('--require-gpu')
                result = subprocess.run(command, cwd=root, env=environment, capture_output=True,
                                        text=True, timeout=120)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                return result.stdout

            def capture(name):
                image = png((root / f'{name}.png').read_bytes())
                self.assertEqual(image[:2], (1024, 576))
                return image

            # Explicitly click the title before issuing story actions. Actions
            # also execute at selection waits, so --new-game alone would allow
            # an initial capture/save action to run on the title screen.
            output = play('story', [
                'click 200 370', 'capture page1.png', 'save 777', 'advance',
                'capture page2.png', 'history', 'capture history.png', 'history-close', 'menu option', 'capture option.png',
                'click 900 340', 'capture option-closed.png', 'menu save', 'capture save.png',
                'click 200 300', 'click 400 320', 'click 900 340',
                'load 777', 'capture restored.png', 'load-original 777',
                'capture native.png', 'advance', 'capture native-next.png', 'quit',
            ])
            self.assertIn('TITLE_READY ', output)
            self.assertGreaterEqual(output.count('TEXT_READY '), 2)
            self.assertIn('SAVE_READY slot=777', output)
            self.assertIn('SAVE_READY slot=3', output, 'save menu did not save its selected slot')
            self.assertIn('LOAD_READY slot=777', output)
            self.assertIn('LOAD_ORIGINAL_READY slot=777', output)
            self.assertTrue((state / 'savedata.003.yukari').is_file())
            self.assertTrue((state / 'savedata.777.yukari').is_file())
            self.assertTrue((state / 'savedata.777').read_bytes().startswith(b'Yuka  '),
                            'native save is missing its original Yuka container header')
            page1, page2, restored = (capture(name) for name in ('page1', 'page2', 'restored'))
            first, second, reloaded = map(text_crop, (page1, page2, restored))
            self.assertGreater(sum(a != b for a, b in zip(first, second)), 100,
                               'advancing did not change the dialogue area')
            first_mask, reloaded_mask = map(dialogue_mask, (first, reloaded))
            self.assertGreater(sum(first_mask), 100, 'first dialogue has no visible white glyphs')
            self.assertLess(sum(a != b for a, b in zip(first_mask, reloaded_mask)), max(10, sum(first_mask) // 100),
                            'reload did not restore the original dialogue area\n' + output)
            for name in ('history', 'option', 'save'):
                menu = capture(name)
                changes = sum(rgb(menu, x, y) != rgb(page2, x, y)
                              for y in range(0, 576, 4) for x in range(0, 1024, 4))
                self.assertGreater(changes, 1000, f'{name} menu did not appear')

            closed = dialogue_mask(text_crop(capture('option-closed')))
            second_mask = dialogue_mask(second)
            self.assertLess(sum(a != b for a, b in zip(closed, second_mask)), max(10, sum(second_mask) // 100), 'option menu erased the current dialogue')

            locations = re.findall(r'TEXT_READY page=\d+ script=(\S+) word=(\d+)', output)
            self.assertGreaterEqual(len(locations), 3, output)
            self.assertEqual(locations[-1], locations[1], 'native reload resumed at the wrong script position')
            capture('native')
            capture('native-next')

            output = play('restart', ['capture restarted.png', 'quit'], ['--load-slot', '777'])
            self.assertIn('LOAD_READY slot=777', output)
            restarted = text_crop(capture('restarted'))
            self.assertLess(sum(a != b for a, b in zip(first_mask, dialogue_mask(restarted))), max(10, sum(first_mask) // 100),
                            'cross-process reload did not restore dialogue')
            output = play('native-restart', ['capture native-restarted.png', 'advance',
                                            'capture native-restarted-next.png', 'quit'],
                          ['--load-original-slot', '777'])
            self.assertIn('TITLE_READY ', output, 'native import must bootstrap original engine setup')
            self.assertIn('LOAD_ORIGINAL_READY slot=777', output)
            native_locations = re.findall(r'TEXT_READY page=\d+ script=(\S+) word=(\d+)', output)
            self.assertTrue(native_locations, output)
            self.assertEqual(native_locations[-1], locations[1], 'cross-process native reload resumed incorrectly')
            capture('native-restarted')
            capture('native-restarted-next')
            self.assertFalse((root / '.yukari').exists(), 'state escaped the selected directory')
        self.assertEqual({path.name for path in game.iterdir()}, names,
                         'runtime created files in the original game directory')
        self.assertEqual({path: (path.stat().st_size, path.stat().st_mtime_ns) for path in originals}, before,
                         'runtime changed an original executable or resource archive')


if __name__ == '__main__':
    unittest.main(argv=[__file__], verbosity=2)
