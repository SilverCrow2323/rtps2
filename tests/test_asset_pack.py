import hashlib
import tempfile
import unittest
from pathlib import Path

from tools.nexus_asset_pack import (
    AssetError, HEADER_SIZE, ICON_NAMES, build_icon_atlas, decode_png_rgba,
    encode_png_rgba, make_pack, verify_pack,
)


def write_png(path, width, height, rgba):
    path.write_bytes(encode_png_rgba(width, height, rgba))


class AssetPackTests(unittest.TestCase):
    def test_png_roundtrip(self):
        pixels=bytes([10,20,30,255,40,50,60,128])
        self.assertEqual(decode_png_rgba(encode_png_rgba(2,1,pixels)),(2,1,pixels))

    def test_bad_png_crc_rejected(self):
        png=bytearray(encode_png_rgba(1,1,bytes([1,2,3,255])))
        png[-5]^=1
        with self.assertRaisesRegex(AssetError,"CRC"):
            decode_png_rgba(bytes(png))

    def test_non_power_of_two_rejected(self):
        with self.assertRaisesRegex(AssetError,"powers of two"):
            make_pack(3,2,bytes(24))

    def test_header_and_sha(self):
        pixels=bytes([9,8,7,128])*(8*4)
        data=make_pack(8,4,pixels)
        self.assertEqual(len(data),HEADER_SIZE+len(pixels))
        self.assertEqual(verify_pack(data)["sha256"],hashlib.sha256(pixels).hexdigest())

    def test_modified_payload_rejected(self):
        data=bytearray(make_pack(4,4,bytes(64))); data[-1]^=255
        with self.assertRaisesRegex(AssetError,"SHA-256"):
            verify_pack(bytes(data))

    def test_atlas_is_deterministic_and_gs_alpha_is_scaled(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp)
            for i,name in enumerate(ICON_NAMES):
                write_png(root/(name+".png"),64,64,bytes([i*20,40,80,255])*(64*64))
            payload,manifest=build_icon_atlas(root)
            self.assertEqual(len(payload),256*128*4)
            self.assertEqual(payload[:4],bytes([0,40,80,128]))
            self.assertEqual(manifest["store"]["x"],192)
            unused=(64*256+128)*4
            self.assertEqual(payload[unused:unused+4],bytes(4))
            self.assertEqual(build_icon_atlas(root)[0],payload)

    def test_missing_source_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaisesRegex(AssetError,"missing icon source"):
                build_icon_atlas(Path(tmp))

    def test_wrong_dimensions_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp)
            for name in ICON_NAMES:
                size=32 if name=="games" else 64
                write_png(root/(name+".png"),size,size,bytes([1,2,3,255])*(size*size))
            with self.assertRaisesRegex(AssetError,"exactly 64x64"):
                build_icon_atlas(root)


if __name__=="__main__":
    unittest.main()
