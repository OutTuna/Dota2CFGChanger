import contextlib
import importlib.util
import io
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location(
    "appimage_abi", Path(__file__).resolve().parents[1] / "scripts/check-appimage-abi.py"
)
abi = importlib.util.module_from_spec(spec)
spec.loader.exec_module(abi)


class AppImageChecks(unittest.TestCase):
    def test_only_runtime_requirements_are_checked(self):
        output = """Version definition section '.gnu.version_d':
  Name: GLIBCXX_3.4.32
Version needs section '.gnu.version_r':
  Name: GLIBC_2.35
  Name: GLIBCXX_3.4.30
  Name: CXXABI_1.3.13
"""
        self.assertEqual(abi.required_versions(output),
                         ["GLIBC_2.35", "GLIBCXX_3.4.30", "CXXABI_1.3.13"])

    def test_packaged_library_cannot_raise_baseline(self):
        for version in ("GLIBC_2.38", "GLIBCXX_3.4.32", "CXXABI_1.3.14", "GLIBC_PRIVATE"):
            with self.subTest(version=version), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp)
                (root / "AppRun").write_bytes(b"\x7fELF")
                (root / "libcrypto.so.3").write_bytes(b"\x7fELF")

                def readelf(args, **kwargs):
                    requirement = version if args[-1].endswith("libcrypto.so.3") else "GLIBC_2.35"
                    return "Version needs section '.gnu.version_r':\n  Name: " + requirement

                with patch.object(abi.subprocess, "check_output", side_effect=readelf):
                    with contextlib.redirect_stdout(io.StringIO()):
                        with self.assertRaisesRegex(RuntimeError, version):
                            abi.check(root)

    def test_baseline_passes_with_apprun_symlink(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "DotaManager").write_bytes(b"\x7fELF")
            (root / "AppRun").symlink_to("DotaManager")
            (root / "icon.png").write_bytes(b"PNG")
            output = "Version needs section '.gnu.version_r':\n  Name: GLIBC_2.35\n  Name: GLIBCXX_3.4.30"
            with patch.object(abi.subprocess, "check_output", return_value=output) as readelf:
                with contextlib.redirect_stdout(io.StringIO()):
                    abi.check(root)
                self.assertEqual(readelf.call_count, 1)

    def test_empty_package_fails(self):
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaisesRegex(RuntimeError, "No ELF files"):
                abi.check(Path(tmp))


if __name__ == "__main__":
    unittest.main()
