"""Opt-in smoke checks for a separately supplied original reference game install.

python3 tests/original_game.py --game-dir /path/to/game --exe build/yukari
Without --game-dir (or YUKARI_GAME_DIR), all tests skip. Commercial
resources are never copied into fixtures. This is deliberately not in CTest.
"""
import argparse
import os
from pathlib import Path
import subprocess
import unittest


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--game-dir', default=os.environ.get('YUKARI_GAME_DIR'))
parser.add_argument('--exe', default=str(Path(__file__).resolve().parents[1] / 'build' / 'yukari'))
options = parser.parse_args()


@unittest.skipUnless(options.game_dir, 'requires an explicitly supplied original game directory')
class OriginalGame(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.directory = Path(options.game_dir).resolve()
        cls.exe = str(Path(options.exe).resolve())
        cls.manifests = {}
        for number in range(1, 6):
            path = cls.directory / f'data{number:02}.ykc'
            result = subprocess.run([cls.exe, 'list', str(path)], capture_output=True,
                                    text=True, timeout=30, check=True)
            cls.manifests[number] = [line.split('\t', 2)[2] for line in result.stdout.splitlines()]

    def run_yukari(self, command, archive, entry, *extra):
        result = subprocess.run([self.exe, command, str(self.directory / archive), entry, *extra],
                                capture_output=True, text=True, timeout=30,
                                env=dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_RENDER_DRIVER='software'))
        self.assertEqual(result.returncode, 0, result.stderr)
        return result

    def test_original_manifest(self):
        self.assertEqual([len(self.manifests[i]) for i in range(1, 6)], [572, 2193, 4261, 2061, 15656])
        self.assertEqual(sum(n.endswith('.yks') for n in self.manifests[1]), 571)
        for name in ('default.ini', 'start.yks', r'yks\system\optionsystem.yks', r'yks\title\title_main.yks'):
            self.assertIn(name, self.manifests[1])
        self.assertIn(r'ykg\title\title_bace.ykg', self.manifests[2])

    def test_startup_scripts_parse(self):
        for name in ('start.yks', r'yks\system\optionsystem.yks', r'yks\title\title_main.yks'):
            with self.subTest(entry=name):
                result = self.run_yukari('script-entry', 'data01.ykc', name)
                self.assertIn('# YKS001 encrypted=1', result.stdout)
                if name == 'start.yks':
                    self.assertIn('title_main.yks', result.stdout)
        for name in (r'yks\system\OptionSystem.yks', r'yks\system\createText.yks', r'YKS\TITLE\TITLE_MAIN.YKS'):
            with self.subTest(mixed_case_reference=name):
                result = self.run_yukari('game-script', '.', name)
                self.assertIn('# YKS001 encrypted=1', result.stdout)

    def test_color_planes_decode_and_upload(self):
        samples = [
            (r'ykg\bg\12th_bg_01a.ykg', '1124x676', False),
            ('system.ykg', '672x32', True),
            (r'ykg\effect\eff_orz_01.ykg', '1024x576', True),
            (r'ykg\title\title_bace.ykg', '1024x576', False),
            (r'ykg\title\logo\title_moji.ykg', '464x237', False),
        ]
        for entry, dimensions, warns in samples:
            with self.subTest(entry=entry):
                result = self.run_yukari('view-entry', 'data02.ykc', entry, '--once')
                self.assertIn('Preview ' + dimensions, result.stdout)
                self.assertEqual('separate alpha mask and animation are not applied' in result.stderr, warns)

    def test_vm_startup_stops_at_first_missing_service(self):
        result = subprocess.run([self.exe, 'vm', str(self.directory), '--trace'],
                                capture_output=True, text=True, timeout=30)
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn('unimplemented engine function: WindowNameSet', result.stderr)
        self.assertIn(r'yks\system\OptionSystem.yks:word=0xa', result.stderr)
        self.assertIn('start.yks:word=0x28  Return', result.stderr)


if __name__ == '__main__':
    unittest.main(argv=[__file__], verbosity=2)
