#!/usr/bin/env python3
"""Static extraction, archive precedence and lossless translation-export fixtures."""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest
from runtime_fixtures import Script, archive

EXE = None

class TextDump(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='yukari-text-dump-')
        self.root = Path(self.temp.name)
        self.game = self.root / 'game'
        self.game.mkdir()

    def tearDown(self):
        self.temp.cleanup()

    def run_dump(self, output, ok=True):
        before = {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                  for p in self.game.iterdir() if p.is_file()}
        result = subprocess.run([str(EXE), 'dump-text', str(self.game), str(output)],
                                capture_output=True, text=True,
                                env=dict(os.environ, SDL_VIDEODRIVER='must-not-initialize-video'), timeout=30)
        self.assertEqual(result.returncode, 0 if ok else 1, result.stdout + result.stderr)
        after = {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                 for p in self.game.iterdir() if p.is_file()}
        self.assertEqual(before, after, 'dump modified game sources')
        self.assertNotIn('RENDERER', result.stdout)
        return result

    def fixture(self):
        s = Script()
        text = '「日本語」,"quoted"\r\n\t@r(0,ルビ)\\path\x01'
        s.call('StrOut', 7, text)
        original = s.code[-1]
        alias = s.element(5, 0, s.elements[original][2], 0)
        s.call('StrOutNW')
        s.code[-1] = 1
        s.code.append(alias)
        s.value(text)  # Same text at another offset must remain independent.
        s.value('未使用の文字列')
        s.value('')
        for function, args in [('NameOut', ['主人公']), ('Case2', [1, '左へ']),
                               ('Index', ['第一章']), ('WindowNameSet', ['題名']),
                               ('GraphicLoad', [0, 'images\\背景.ykg', 0, 0]),
                               ('ScriptCall', ['next.yks']), ('DefaultFontSet', [0, 'ＭＳ ゴシック', 27, 400]),
                               ('UnknownDialog', ['未知の項目'])]:
            s.call(function, *args)
        s.call('ScriptExit')
        return s, text, original, alias

    def test_lossless_literals_aliases_references_and_determinism(self):
        s, text, original, alias = self.fixture()
        raw = s.finish()
        package = archive([('Scene.YKS', raw), ('image.ykg', b'not an image')])
        (self.game / 'data01.ykc').write_bytes(package)
        # Translation inputs are never mounted.
        (self.game / 'data01.cnp').write_bytes(b'not an archive')
        output = self.root / 'export'
        result = self.run_dump(output)
        rows = [json.loads(line) for line in (output / 'texts.jsonl').read_text().splitlines()]
        manifest = json.loads((output / 'manifest.json').read_text())
        self.assertEqual(manifest['script_count'], 1)
        self.assertEqual(manifest['string_count'], len(rows))
        self.assertEqual(manifest['use_count'], sum(len(row['uses']) for row in rows))
        self.assertIn(f'strings={len(rows)}', result.stdout)
        duplicate_text = [row for row in rows if row['source'] == text]
        self.assertEqual(len(duplicate_text), 2)
        used = next(row for row in duplicate_text if row['uses'])
        self.assertEqual(used['elements'], [original, alias])
        self.assertEqual([use['function'] for use in used['uses']], ['StrOut', 'StrOutNW'])
        self.assertEqual([use['argument'] for use in used['uses']], [1, 0])
        self.assertEqual(used['kinds'], ['dialogue'])
        self.assertNotEqual(duplicate_text[0]['id'], duplicate_text[1]['id'])
        self.assertIn('unused', next(row for row in rows if row['source'] == '')['kinds'])
        categories = {row['source']: row['kinds'] for row in rows}
        for source, expected in [('主人公', 'name'), ('左へ', 'choice'), ('第一章', 'chapter'),
                                 ('題名', 'ui'), ('images\\背景.ykg', 'resource'),
                                 ('next.yks', 'script'), ('ＭＳ ゴシック', 'font'), ('未知の項目', 'other')]:
            self.assertIn(expected, categories[source])
        self.assertEqual(bytes.fromhex(used['source_hex']), text.encode('cp932'))
        at = used['file_offset']
        stored = bytes(byte ^ 0xaa for byte in raw[at:at+len(text.encode('cp932'))])
        self.assertEqual(stored, text.encode('cp932'))
        self.assertEqual(package[used['archive_offset']:used['archive_offset']+len(stored)], raw[at:at+len(stored)])
        with (output / 'translation.csv').open(encoding='utf-8-sig', newline='') as file:
            csv_rows = list(csv.DictReader(file))
        self.assertEqual(len(csv_rows), len(rows))
        self.assertTrue(all(row['translation'] == '' for row in csv_rows))
        self.assertEqual(next(row['source'] for row in csv_rows if row['id'] == used['id']), text)
        again = self.root / 'again'
        self.run_dump(again)
        for file in output.iterdir():
            self.assertEqual(file.read_bytes(), (again / file.name).read_bytes())

    def test_original_mount_order_shadowing_and_unencrypted_yks(self):
        s = Script(); s.call('StrOut', '有効な原版')
        raw = bytearray(s.finish())
        raw[6] = 0
        offset = struct.unpack_from('<I', raw, 32)[0]
        raw[offset:] = bytes(byte ^ 0xaa for byte in raw[offset:])
        other = Script(); other.call('StrOut', '別スクリプト')
        (self.game / 'data04.ykc').write_bytes(archive([('SCENE.yks', bytes(raw))]))
        (self.game / 'data01.ykc').write_bytes(archive([('scene.YKS', b'broken shadowed script'),
                                                      ('NEXT.YKS', other.finish()),
                                                      ('next.yks', b'broken later duplicate')]))
        output = self.root / 'export'
        self.run_dump(output)
        rows = [json.loads(line) for line in (output / 'texts.jsonl').read_text().splitlines()]
        self.assertEqual({row['source'] for row in rows}, {'有効な原版', '別スクリプト'})
        winner = next(row for row in rows if row['source'] == '有効な原版')
        self.assertEqual(winner['archive'], 'data04.ykc')
        self.assertEqual(winner['script'], 'SCENE.yks')
        self.assertTrue(winner['id'].startswith('scene.yks#str:'))
        manifest = json.loads((output / 'manifest.json').read_text())
        self.assertEqual(manifest['script_count'], 2)
        self.assertFalse(next(row for row in manifest['scripts'] if row['script'] == 'SCENE.yks')['encrypted'])

    def test_existing_output_and_game_subdirectory_are_rejected(self):
        s = Script(); s.call('StrOut', 'TEXT')
        (self.game / 'data01.ykc').write_bytes(archive([('start.yks', s.finish())]))
        existing = self.root / 'existing'; existing.mkdir()
        sentinel = existing / 'user.csv'; sentinel.write_bytes(b'KEEP')
        self.run_dump(existing, ok=False)
        self.assertEqual(sentinel.read_bytes(), b'KEEP')
        inside = self.game / 'dump'
        self.run_dump(inside, ok=False)
        self.assertFalse(inside.exists())
        link = self.root / 'linked-game'; link.symlink_to(self.game, target_is_directory=True)
        self.run_dump(link / 'dump', ok=False)
        self.assertFalse(inside.exists())

    def test_bad_script_removes_partial_export_and_reports_its_name(self):
        s = Script(); s.call('StrOut', 'GOOD')
        (self.game / 'data01.ykc').write_bytes(archive([('a.yks', s.finish()), ('z-bad.yks', b'broken')]))
        output = self.root / 'failed'
        result = self.run_dump(output, ok=False)
        self.assertIn('z-bad.yks', result.stderr)
        self.assertFalse(output.exists(), 'partial output was left as an apparently complete translation dump')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    args, remaining = parser.parse_known_args()
    EXE = args.exe.resolve()
    unittest.main(argv=[__file__, *remaining])
