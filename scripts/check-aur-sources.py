from pathlib import Path
import sys

baseline, project = map(Path, sys.argv[1:3])
paths = {"CMakeLists.txt", "main.cpp"}
for directory in ("src", "tests", "cmake", "resources"):
    paths.update(path.relative_to(project).as_posix()
        for path in (project / directory).rglob("*")
        if path.is_file() and "__pycache__" not in path.parts and path.suffix != ".pyc")
for name in sorted(paths):
    if not (baseline / name).is_file() or (baseline / name).read_bytes() != (project / name).read_bytes():
        raise SystemExit(f"AUR patch is out of date for {name}; regenerate before pushing")
print("Patched stable release matches the packaging implementation under test")
