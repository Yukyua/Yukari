"""Synthetic fixtures only: format boundaries and SDL upload, no game compatibility claim."""
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib

EXE = str(Path(sys.argv.pop(1)).resolve())

def p32(*v):
    return struct.pack('<' + 'I' * len(v), *v)

def set32(data, offset, value):
    result = bytearray(data)
    result[offset:offset+4] = p32(value)
    return bytes(result)

def png():
    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', 2, 2, 8, 6, 0, 0, 0))
            + chunk(b'IDAT', zlib.compress((b'\0' + b'\xff\0\0\xff' * 2) * 2)) + chunk(b'IEND', b''))

def bmp():
    pixels = (b'\0\0\xff' * 2 + b'\0\0') * 2
    return b'BM' + p32(54 + len(pixels)) + b'\0'*4 + p32(54, 40, 2, 2) + struct.pack('<HH', 1, 24) + p32(0, len(pixels), 0, 0, 0, 0) + pixels

def ykg(color=None, alpha=b'', frames=b''):
    color = png() if color is None else color
    return (b'YKG000\0\0' + p32(64) + b'\0'*28
            + p32(64, len(color), 64+len(color) if alpha else 0, len(alpha),
                  64+len(color)+len(alpha) if frames else 0, len(frames)) + color + alpha + frames)

def yks(encrypted=True):
    data = bytearray()
    def string(s):
        at = len(data)
        data.extend(s.encode('cp932') + b'\0')
        return at
    def integer(v):
        at = len(data)
        data.extend(struct.pack('<i', v))
        return at
    func = string('表示')
    text = string('こんにちは')
    label = string('start')
    kind = string('Flag')
    minus = integer(-1)
    number = integer(42)
    ident = integer(7)
    index = b''.join(p32(*e) for e in [
        (0, func, 0, 0), (5, 0, text, 0), (4, 0, number, 0),
        (1, label, minus, 0), (7, kind, 0, 0), (8, kind, 0, ident),
        (9, kind, 0, ident), (10, 0, 3, 0)])
    code = p32(3, 0, 3, 1, 2, 5, 4, 6, 7)
    payload = bytes(v ^ (0xaa if encrypted else 0) for v in data)
    return (b'YKS001' + struct.pack('<H', encrypted)
            + p32(48, 0, 48, len(code)//4, 48+len(code), len(index)//16,
                  48+len(code)+len(index), len(data), 4, 0) + code + index + payload)

def ykc(files, version=1):
    body = bytearray(24)
    body[:8] = b'YKC001\0\0' if version == 1 else b'YKC002\0\0'
    body[8:12] = p32(24)
    entries = []
    for name, data in files:
        name_at = len(body)
        encoded = name.encode('cp932' if version == 1 else 'utf-8') + b'\0'
        body.extend(encoded)
        at = len(body)
        body.extend(data)
        entries.append(p32(name_at, len(encoded), at, len(data), 0))
    index_at = len(body)
    body.extend(b''.join(entries))
    body[16:24] = p32(index_at, len(entries)*20)
    return bytes(body)

class Formats(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
    def tearDown(self):
        self.tmp.cleanup()
    def file(self, name, data):
        p = self.root / name
        p.write_bytes(data)
        return str(p)
    def run_yukari(self, *args, ok=True):
        env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_RENDER_DRIVER='software')
        p = subprocess.run([EXE, *map(str, args)], capture_output=True, text=True, env=env, timeout=10)
        self.assertEqual(p.returncode, 0 if ok else 1, p.stdout + p.stderr)
        return p.stdout + p.stderr
    def test_ykc_versions_and_raw_extraction(self):
        for version in (1, 2):
            name = '画像/テスト.yks'
            raw = yks()
            path = self.file(f'data{version}.ykc', ykc([(name, raw)], version))
            self.assertIn(name, self.run_yukari('list', path))
            output = self.root / f'extract{version}.yks'
            self.run_yukari('extract', path, name, output)
            self.assertEqual(output.read_bytes(), raw)
            self.run_yukari('extract', path, name, output, ok=False)
            self.assertIn('こんにちは', self.run_yukari('script-entry', path, name))
    def test_script_encryption_and_types(self):
        for encrypted in (False, True):
            output = self.run_yukari('script', self.file('test.yks', yks(encrypted)))
            for expected in ('表示', 'こんにちは', '42', 'link=-1', 'Flag', ':7', '$3', '00000001'):
                self.assertIn(expected, output)
    def test_original_resource_lookup_and_package_order(self):
        # Mixed-case startup references must find lowercase archived filenames.
        name = r'yks\system\optionsystem.yks'
        self.file('data01.ykc', ykc([(name, yks())]))
        self.assertIn('こんにちは', self.run_yukari('game-script', self.root, name.upper()))
        # data04 wins even when data01 has a valid competing script.
        self.file('data04.ykc', ykc([(name, b'not a script')]))
        self.run_yukari('game-script', self.root, name, ok=False)
        # The profile must not load translation patch containers.
        (self.root/'data04.ykc').unlink()
        (self.root/'data01.ykc').rename(self.root/'DATA01.cnp')
        self.run_yukari('game-script', self.root, name, ok=False)
    def test_case_collision_first_entry_wins(self):
        path = self.file('data.ykc', ykc([('A', b'first'), ('a', b'second')]))
        output = self.root/'selected'
        self.run_yukari('extract', path, 'a', output)
        self.assertEqual(output.read_bytes(), b'first')
    def test_graphic_metadata(self):
        raw = ykg(alpha=png(), frames=p32(0, 0, 2, 2, 100, 2, 9, 1))
        output = self.run_yukari('graphic', self.file('image.ykg', raw))
        self.assertIn('frames=1', output)
        self.assertIn('duration_raw=100 type=2 unknown=9 action=1', output)
    def test_sdl_upload_and_present(self):
        variants = [png(), bmp(), ykg(), ykg(color=b'\x89GNP' + png()[4:])]
        for i, raw in enumerate(variants):
            self.assertIn('Preview 2x2', self.run_yukari('view', self.file(f'image{i}', raw), '--once'))
        archive = self.file('data.ykc', ykc([('image.ykg', ykg())]))
        self.assertIn('Preview 2x2', self.run_yukari('view-entry', archive, 'image.ykg', '--once'))
    def test_truncations(self):
        for command, raw in [('list', ykc([('a', b'123')])), ('script', yks()), ('graphic', ykg())]:
            for cut in (0, 4, 8, 16, 23, len(raw)-1):
                self.run_yukari(command, self.file('broken', raw[:cut]), ok=False)
    def test_archive_bounds(self):
        raw = ykc([('a', b'123')])
        index = struct.unpack_from('<I', raw, 16)[0]
        for bad in (set32(raw, 16, 0xffffffff), set32(raw, 20, 19),
                    set32(raw, index, 0xffffffff), set32(raw, index+4, 0),
                    set32(raw, index+8, 0xffffffff), set32(raw, index+12, 0xffffffff)):
            self.run_yukari('list', self.file('bad', bad), ok=False)
    def test_script_bounds_and_unknowns(self):
        raw = yks()
        index = struct.unpack_from('<I', raw, 24)[0]
        for bad in (set32(raw, 20, 0xffffffff), set32(raw, 28, 0xffffffff),
                    set32(raw, 32, 0xffffffff), set32(raw, 48, 9999),
                    set32(raw, 48+8, 9999), set32(raw, index, 11),
                    set32(raw, index+4, 0xffffffff), set32(raw, 8, 64)):
            self.run_yukari('script', self.file('bad', bad), ok=False)
    def test_graphic_bounds(self):
        raw = ykg()
        for bad in (set32(raw, 0x28, 0xffffffff), set32(raw, 0x2c, 0xffffffff),
                    set32(raw, 0x34, 0xffffffff), set32(raw, 0x3c, 1)):
            self.run_yukari('graphic', self.file('bad', bad), ok=False)
    def test_missing_member_and_invalid_text(self):
        archive = self.file('a.ykc', ykc([('a', b'123')]))
        self.run_yukari('extract', archive, 'missing', self.root/'out', ok=False)
        raw = bytearray(ykc([('a', b'123')]))
        raw[24:26] = b'\x81\0'
        self.run_yukari('list', self.file('invalid.ykc', raw), ok=False)
    def test_invalid_image_and_dimensions(self):
        self.run_yukari('view', self.file('bad', b'garbage'), '--once', ok=False)
        self.run_yukari('view', self.file('huge.bmp', set32(bmp(), 18, 0x7fffffff)), '--once', ok=False)

if __name__ == '__main__':
    unittest.main()
