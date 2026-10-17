from pathlib import Path
import subprocess
import sys
import tempfile

checker = Path(__file__).resolve().parents[1] / "scripts/check-aur-sources.py"
with tempfile.TemporaryDirectory(prefix="dotamanager-aur-coverage-") as temporary:
    root = Path(temporary)
    baseline, project = root / "baseline", root / "project"
    for folder in (baseline, project):
        folder.mkdir()
        (folder / "CMakeLists.txt").write_text("project(Test)\n")
        (folder / "main.cpp").write_text("int main() {}\n")
        (folder / "src").mkdir()
    def check(expected, name=""):
        result = subprocess.run([sys.executable, str(checker), str(baseline), str(project)],
            capture_output=True, text=True)
        assert (result.returncode == 0) == expected, result.stdout + result.stderr
        if name:
            assert name in result.stderr
    check(True)
    (project / "src/new.cpp").write_text("new source\n")
    check(False, "src/new.cpp")
    (baseline / "src/new.cpp").write_text("old source\n")
    check(False, "src/new.cpp")
    (baseline / "src/new.cpp").write_text("new source\n")
    check(True)
    (project / "tests").mkdir()
    (project / "tests/new_checks.cpp").write_text("new regression\n")
    check(False, "tests/new_checks.cpp")
    (baseline / "tests").mkdir()
    (baseline / "tests/new_checks.cpp").write_text("new regression\n")
    (project / "tests/__pycache__").mkdir()
    (project / "tests/__pycache__/ignored.pyc").write_bytes(b"cache")
    check(True)
print("AUR coverage rejects missing and outdated sources without relying on a manual list")
