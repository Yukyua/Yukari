"""Opt-in original-game startup/render test; no commercial fixtures are bundled.

python3 tests/title.py --game-dir /path/to/game --exe build/yukari
Without both explicit arguments the test skips. All state/captures are temporary.
PNG decoding uses only the Python standard library.
"""
import argparse
import hashlib
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest
import zlib


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--game-dir', type=Path)
parser.add_argument('--exe', type=Path)
parser.add_argument('--gpu', action='store_true', help='Require real Metal rendering; needs a macOS display session')
options = parser.parse_args()
if bool(options.game_dir) != bool(options.exe):
    parser.error('--game-dir and --exe must be supplied together')


def sha256(path):
    result = hashlib.sha256()
    with path.open('rb') as file:
        for chunk in iter(lambda: file.read(4 * 1024 * 1024), b''):
            result.update(chunk)
    return result.hexdigest()


def png(data):
    """Decode noninterlaced 8-bit RGB/RGBA PNG, including all five row filters."""
    if data[:8] != b'\x89PNG\r\n\x1a\n':
        raise ValueError('not a PNG')
    position, compressed = 8, bytearray()
    width = height = channels = None
    while position < len(data):
        size = struct.unpack_from('>I', data, position)[0]
        kind = data[position + 4:position + 8]
        payload = data[position + 8:position + 8 + size]
        checksum = struct.unpack_from('>I', data, position + 8 + size)[0]
        if zlib.crc32(kind + payload) != checksum:
            raise ValueError('PNG CRC mismatch')
        if kind == b'IHDR':
            width, height, depth, color, compression, filtering, interlace = struct.unpack('>IIBBBBB', payload)
            if depth != 8 or color not in (2, 6) or compression or filtering or interlace:
                raise ValueError('expected noninterlaced 8-bit RGB/RGBA PNG')
            channels = 3 if color == 2 else 4
        elif kind == b'IDAT':
            compressed.extend(payload)
        elif kind == b'IEND':
            break
        position += size + 12
    if not width or not height or width * height > 64_000_000:
        raise ValueError('invalid PNG dimensions')
    raw = zlib.decompress(compressed)
    stride = width * channels
    if len(raw) != height * (stride + 1):
        raise ValueError('invalid PNG decompressed size')
    pixels = bytearray(height * stride)
    for y in range(height):
        row_start = y * (stride + 1)
        method = raw[row_start]
        if method > 4:
            raise ValueError('invalid PNG filter')
        output = y * stride
        for x in range(stride):
            left = pixels[output + x - channels] if x >= channels else 0
            above = pixels[output + x - stride] if y else 0
            upper_left = pixels[output + x - stride - channels] if y and x >= channels else 0
            predictor = 0
            if method == 1:
                predictor = left
            elif method == 2:
                predictor = above
            elif method == 3:
                predictor = (left + above) // 2
            elif method == 4:
                p = left + above - upper_left
                distances = (abs(p - left), abs(p - above), abs(p - upper_left))
                predictor = (left, above, upper_left)[distances.index(min(distances))]
            pixels[output + x] = (raw[row_start + 1 + x] + predictor) & 255
    return width, height, channels, pixels


def rgb(image, x, y):
    width, _, channels, pixels = image
    offset = (y * width + x) * channels
    return pixels[offset:offset + 3]


@unittest.skipUnless(options.game_dir and options.exe, 'requires explicit --game-dir and --exe')
class TitleStartup(unittest.TestCase):
    def test_fresh_start_hover_and_persistent_restart(self):
        game, exe = options.game_dir.resolve(), options.exe.resolve()
        archives = [game / f'data{i:02}.ykc' for i in range(1, 6)]
        before = {path: sha256(path) for path in archives}
        top_level = {path.name for path in game.iterdir()}
        with tempfile.TemporaryDirectory(prefix='yukari-title-') as directory:
            root = Path(directory)
            state = root / 'state'
            env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy',
                       SDL_RENDER_DRIVER='software')
            if options.gpu:
                env = dict(os.environ, SDL_AUDIODRIVER='dummy', SDL_RENDER_DRIVER='metal', SDL_VIDEO_HIGHDPI_DISABLED='1')
                env.pop('SDL_VIDEODRIVER', None)

            def play(output, hover=False):
                command = [str(exe), 'play', str(game), '--fast', '--mute', '--until-title',
                           '--state-dir', str(state), '--max-ms', '60000', '--seed', '0',
                           '--screenshot', str(output), '--trace']
                if hover:
                    command.extend(['--hover', '180', '380'])
                if options.gpu: command.append('--require-gpu')
                result = subprocess.run(command, cwd=root, env=env, capture_output=True,
                                        text=True, timeout=90)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertRegex(result.stderr, r'(?m)^start\.yks:word=0x0\s',
                                 'startup must execute original start.yks')
                self.assertIn('TITLE_READY ', result.stdout)
                self.assertIn(r'yks\title\title_main.yks:word=0x74', result.stderr)
                return png(output.read_bytes())

            self.assertFalse(state.exists(), 'first startup must have fresh state')
            normal = play(root / 'normal.png')
            self.assertEqual(normal[:2], (1024, 576))
            self.assertGreater(len({bytes(rgb(normal, x, y)) for y in range(0, 576, 8)
                                    for x in range(0, 1024, 8)}), 128, 'title screenshot is blank')
            saved = state / 'yukari-system.dat'
            self.assertTrue(saved.is_file(), 'system state was not persisted')
            saved_text = saved.read_text()
            self.assertTrue(saved_text.startswith('YUKARI_SYSTEM_1\n'))
            self.assertIn('F 60 1\n', saved_text, 'fresh-start initialization was not saved')
            native_system = state / 'System.dat'
            self.assertTrue(native_system.is_file(), 'native system state was not persisted')
            native_bytes = native_system.read_bytes()
            self.assertGreaterEqual(len(native_bytes), 0x7c)
            self.assertEqual(struct.unpack_from('<I', native_bytes, 0x10)[0], 0x7c,
                             'native System.dat header size is incorrect')
            self.assertEqual({p.name for p in state.iterdir()}, {'yukari-system.dat', 'System.dat', 'Already.dat', 'savedata.000', 'savedata.000.yukari'})

            # Compare button locations against the original background, rather
            # than inferring button count from unrelated system-menu sprites.
            background = root / 'background.ykg'
            extraction = subprocess.run([str(exe), 'extract', str(archives[1]),
                                         r'ykg\title\title_bace.ykg', str(background)],
                                        capture_output=True, text=True, timeout=30)
            self.assertEqual(extraction.returncode, 0, extraction.stderr)
            blob = background.read_bytes()
            color_offset, color_size = struct.unpack_from('<II', blob, 0x28)
            color = bytearray(blob[color_offset:color_offset + color_size])
            self.assertEqual(color[:4], b'\x89GNP')
            color[1:4] = b'PNG'
            base = png(color)
            present = []
            for x in (162, 306, 450, 594, 738):
                changes = sum(rgb(normal, xx, yy) != rgb(base, xx, yy)
                              for yy in range(350, 474) for xx in range(x, x + 124))
                present.append(changes > 100)
            self.assertIn(sum(present), (4, 5), 'expected four or five visible title buttons')
            self.assertTrue(all(present[i] for i in (0, 1, 2, 4)), 'a required title button is missing')

            # This second process restores the same state and still enters via
            # start.yks. Hover must affect a local region of the rendered title.
            hovered = play(root / 'hover.png', hover=True)
            self.assertEqual(hovered[:2], (1024, 576))
            changed = [(x, y) for y in range(576) for x in range(1024)
                       if rgb(normal, x, y) != rgb(hovered, x, y)]
            self.assertGreater(len(changed), 100, 'hover did not change the title image')
            self.assertLess(len(changed), 1024 * 576 // 4, 'hover unexpectedly changed the whole title')
            self.assertTrue(any(162 <= x < 286 and 350 <= y < 474 for x, y in changed),
                            'hover did not alter the first button')
            self.assertEqual({p.name for p in state.iterdir()}, {'yukari-system.dat', 'System.dat', 'Already.dat', 'savedata.000', 'savedata.000.yukari'})
            self.assertFalse((root / '.yukari').exists(), 'state escaped the explicit state directory')
        self.assertEqual({path.name for path in game.iterdir()}, top_level,
                         'startup created files inside the original game directory')
        self.assertEqual({path: sha256(path) for path in archives}, before,
                         'startup modified original archive bytes')


if __name__ == '__main__':
    unittest.main(argv=[__file__], verbosity=2)
