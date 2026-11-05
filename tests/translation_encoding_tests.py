from pathlib import Path
import argparse
import csv
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from translation_encoding import compatible_translation
from translate_with_codex import controls_valid, mask_controls, restore_controls
from runtime_fixtures import Script, archive

EXE = None


class TranslationEncodingTests(unittest.TestCase):
    def test_reported_dialogue(self):
        text = compatible_translation("「谢谢你呀♪」")
        self.assertEqual(text, "「谢谢你呀～」")
        text.encode("gbk")

    def test_pit_faces_and_emphasis(self):
        text = compatible_translation(" >> １ （´∀｀）ﾉｼ 今@r(1,・)回")
        self.assertEqual(text, " >> １ （′Ａ｀）ノシ 今@r(1,·)回")
        self.assertTrue(controls_valid("今@r(1,・)回", "这@r(1,·)次"))
        self.assertFalse(controls_valid("今@r(1,・)回", "这@r(1,一)次"))
        source = "今@r(1,・)回"
        masked, found = mask_controls(source)
        self.assertEqual(restore_controls(masked, found), source)

    def test_unknown_character_rejected(self):
        with self.assertRaises(UnicodeEncodeError):
            compatible_translation("😀")

    def test_original_encoding_kept(self):
        self.assertEqual(compatible_translation("♪", "CP932"), "♪")

    def test_builder_and_runtime_validator(self):
        if EXE is None:
            self.skipTest("runtime executable not supplied")
        with tempfile.TemporaryDirectory(prefix="yukari-patch-encoding-") as tmp:
            root = Path(tmp)
            script = Script()
            script.call("StrOut", "原文")
            offset = script.elements[script.code[-1]][2]
            (root / "data01.ykc").write_bytes(archive([("start.yks", script.finish())]))
            patch = root / "translation_patch"
            patch.write_text("YUKARI_TRANSLATION_PATCH_1\n@encoding\tCP936\n" +
                             f"{'start.yks'.encode().hex()}\t{offset:x}\t{'原文'.encode().hex()}\t{'谢谢♪'.encode().hex()}\n")
            command = [str(EXE), "validate-translation-patch", str(root), str(patch)]
            invalid = subprocess.run(command, capture_output=True, text=True)
            self.assertNotEqual(invalid.returncode, 0)
            self.assertIn("start.yks", invalid.stderr)
            self.assertIn("encoding=CP936", invalid.stderr)
            table = root / "translation.csv"
            with table.open("w", encoding="utf-8", newline="") as file:
                writer = csv.DictWriter(file, fieldnames=["id", "source", "translation"])
                writer.writeheader()
                writer.writerow({"id": f"start.yks#str:{offset:x}", "source": "原文", "translation": "谢谢♪"})
            builder = Path(__file__).resolve().parents[1] / "tools/build_translation_patch.py"
            subprocess.run([sys.executable, str(builder), str(table), str(patch)], check=True, capture_output=True)
            valid = subprocess.run(command, capture_output=True, text=True)
            self.assertEqual(valid.returncode, 0, valid.stderr)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path)
    args, remaining = parser.parse_known_args()
    EXE = args.exe.resolve() if args.exe else None
    unittest.main(argv=[sys.argv[0], *remaining])
