import json
import tempfile
import unittest
from unittest import mock
from pathlib import Path
from tools.nexus_asset_pack import encode_png_rgba
from tools.nexus_cover_pack import CoverPackError, SIZE, build_pack, fit_cover, verify_pack

def write_png(path: Path, width: int, height: int, color: tuple[int, int, int, int]):
    pixels = bytes(color) * (width * height)
    path.write_bytes(encode_png_rgba(width, height, pixels))

class CoverPackTests(unittest.TestCase):
    def test_letterbox_preserves_aspect_ratio(self):
        source = bytes([255, 0, 0, 255]) * (256 * 128)
        result = fit_cover(256, 128, source)
        self.assertEqual(len(result), SIZE * SIZE * 4)
        self.assertEqual(result[0:4], bytes([0, 0, 0, 255]))
        center = ((64 * SIZE) + 64) * 4
        self.assertEqual(result[center:center + 4], bytes([255, 0, 0, 255]))

    def test_build_and_verify_deterministic_pack(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source, output = root / "covers-in", root / "covers-out"
            source.mkdir()
            write_png(source / "game-b.png", 64, 96, (20, 40, 60, 255))
            write_png(source / "game-a.png", 96, 64, (80, 100, 120, 255))
            first = build_pack(source, output)
            self.assertEqual(first["count"], 2)
            self.assertEqual([x["source"] for x in first["entries"]], ["game-a.png", "game-b.png"])
            self.assertEqual(verify_pack(output)["count"], 2)
            before = (output / "game-a.rgba").read_bytes()
            build_pack(source, output)
            self.assertEqual((output / "game-a.rgba").read_bytes(), before)

    def test_rejects_modified_payload(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); source = root / "in"; output = root / "out"; source.mkdir()
            write_png(source / "one.png", 32, 32, (1, 2, 3, 255))
            build_pack(source, output)
            payload = output / "one.rgba"
            data = bytearray(payload.read_bytes()); data[-1] ^= 0xFF; payload.write_bytes(data)
            with self.assertRaisesRegex(CoverPackError, "SHA-256"):
                verify_pack(output)

    def test_rejects_duplicate_normalized_names(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); source = root / "in"; source.mkdir()
            write_png(source / "A B.png", 8, 8, (1, 2, 3, 255))
            write_png(source / "A_B.png", 8, 8, (4, 5, 6, 255))
            with self.assertRaisesRegex(CoverPackError, "Duplicate output"):
                build_pack(source, root / "out")

    def test_rejects_invalid_png_without_publishing_partial_payloads(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); source = root / "in"; output = root / "out"; source.mkdir()
            write_png(source / "a-valid.png", 8, 8, (1, 2, 3, 255))
            (source / "z-invalid.png").write_bytes(b"not png")
            with self.assertRaisesRegex(CoverPackError, "Cannot decode"):
                build_pack(source, output)
            self.assertFalse((output / "a-valid.rgba").exists())

    def test_failed_publish_restores_previous_complete_pack(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); source = root / "in"; output = root / "out"; source.mkdir()
            write_png(source / "game-a.png", 32, 32, (1, 2, 3, 255))
            write_png(source / "game-b.png", 32, 32, (4, 5, 6, 255))
            build_pack(source, output)
            old_manifest = (output / "covers.json").read_bytes()
            old_a = (output / "game-a.rgba").read_bytes()
            old_b = (output / "game-b.rgba").read_bytes()
            write_png(source / "game-a.png", 32, 32, (9, 8, 7, 255))
            real_replace = __import__("os").replace
            def fail_second_payload(src, dst):
                if Path(dst).name == "game-b.rgba" and Path(src).parent.name == "payload":
                    raise OSError("simulated commit failure")
                return real_replace(src, dst)
            with mock.patch("tools.nexus_cover_pack.os.replace", side_effect=fail_second_payload):
                with self.assertRaisesRegex(CoverPackError, "previous pack restored"):
                    build_pack(source, output)
            self.assertEqual((output / "covers.json").read_bytes(), old_manifest)
            self.assertEqual((output / "game-a.rgba").read_bytes(), old_a)
            self.assertEqual((output / "game-b.rgba").read_bytes(), old_b)
            self.assertEqual(verify_pack(output)["count"], 2)

    def test_build_rejects_symlinked_output_payload(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); source = root / "in"; output = root / "out"; source.mkdir(); output.mkdir()
            write_png(source / "one.png", 8, 8, (1, 2, 3, 255))
            outside = root / "outside.rgba"
            outside.write_bytes(b"do not overwrite")
            (output / "one.rgba").symlink_to(outside)
            with self.assertRaisesRegex(CoverPackError, "must not be a symlink"):
                build_pack(source, output)
            self.assertEqual(outside.read_bytes(), b"do not overwrite")

    def test_build_rejects_symlinked_manifest(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); source = root / "in"; output = root / "out"; source.mkdir(); output.mkdir()
            write_png(source / "one.png", 8, 8, (1, 2, 3, 255))
            outside = root / "manifest.json"
            outside.write_text("keep", encoding="utf-8")
            (output / "covers.json").symlink_to(outside)
            with self.assertRaisesRegex(CoverPackError, "Manifest path must not be a symlink"):
                build_pack(source, output)
            self.assertEqual(outside.read_text(encoding="utf-8"), "keep")

    def test_verifier_rejects_symlinked_payload_and_malformed_entries(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); source = root / "in"; output = root / "out"; source.mkdir()
            write_png(source / "one.png", 8, 8, (1, 2, 3, 255))
            build_pack(source, output)
            payload = output / "one.rgba"
            data = payload.read_bytes()
            payload.unlink()
            outside = root / "outside.rgba"
            outside.write_bytes(data)
            payload.symlink_to(outside)
            with self.assertRaisesRegex(CoverPackError, "must not be a symlink"):
                verify_pack(output)
            payload.unlink()
            payload.write_bytes(data)
            manifest_path = output / "covers.json"
            manifest = json.loads(manifest_path.read_text())
            manifest["entries"] = [None]
            manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
            with self.assertRaisesRegex(CoverPackError, "entry must be an object"):
                verify_pack(output)

    def test_verifier_rejects_duplicate_json_keys_at_root_and_nested_levels(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); source = root / "in"; output = root / "out"; source.mkdir()
            write_png(source / "one.png", 8, 8, (1, 2, 3, 255))
            build_pack(source, output)
            manifest_path = output / "covers.json"
            for payload in (
                '{"format":"NXC1","format":"NXC1"}',
                '{"format":"NXC1","entries":[{"output":"one.rgba","output":"other.rgba"}]}',
            ):
                with self.subTest(payload=payload):
                    manifest_path.write_text(payload, encoding="utf-8")
                    with self.assertRaisesRegex(CoverPackError, "duplicate JSON key"):
                        verify_pack(output)

    def test_verifier_rejects_boolean_version_and_count(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); source = root / "in"; output = root / "out"; source.mkdir()
            write_png(source / "one.png", 8, 8, (1, 2, 3, 255))
            build_pack(source, output)
            manifest_path = output / "covers.json"
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            manifest["version"] = True
            manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
            with self.assertRaisesRegex(CoverPackError, "inconsistent cover pack format"):
                verify_pack(output)
            manifest["version"] = 1
            manifest["count"] = True
            manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
            with self.assertRaisesRegex(CoverPackError, "entry count"):
                verify_pack(output)

    def test_verifier_rejects_case_insensitive_duplicate_output_names(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); source = root / "in"; output = root / "out"; source.mkdir()
            write_png(source / "one.png", 8, 8, (1, 2, 3, 255))
            build_pack(source, output)
            payload = (output / "one.rgba").read_bytes()
            (output / "ONE.rgba").write_bytes(payload)
            manifest_path = output / "covers.json"
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            duplicate = dict(manifest["entries"][0])
            duplicate["output"] = "ONE.rgba"
            manifest["entries"].append(duplicate)
            manifest["count"] = 2
            manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
            with self.assertRaisesRegex(CoverPackError, "Duplicate cover output"):
                verify_pack(output)

if __name__ == "__main__":
    unittest.main()
