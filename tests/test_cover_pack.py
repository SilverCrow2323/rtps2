import json
import tempfile
import unittest
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

    def test_rejects_invalid_png(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); source = root / "in"; source.mkdir()
            (source / "bad.png").write_bytes(b"not png")
            with self.assertRaisesRegex(CoverPackError, "Cannot decode"):
                build_pack(source, root / "out")

if __name__ == "__main__":
    unittest.main()
