from pathlib import Path
import sys

baseline, project = map(Path, sys.argv[1:3])
paths = (project / "packaging/aur/source-files.txt").read_text().splitlines()
for name in paths:
    if (baseline / name).read_bytes() != (project / name).read_bytes():
        raise SystemExit(f"AUR patch is out of date for {name}; regenerate before pushing")
print("Patched stable release matches the packaging implementation under test")
