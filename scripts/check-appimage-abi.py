import os
from pathlib import Path
import re
import subprocess
import sys

LIMITS = {"GLIBC": (2, 35), "GLIBCXX": (3, 4, 30), "CXXABI": (1, 3, 13)}


def required_versions(output):
    in_needs = False
    versions = []
    for line in output.splitlines():
        if line.startswith("Version "):
            in_needs = line.startswith("Version needs section")
        if in_needs:
            versions.extend(re.findall(r"Name: ((?:GLIBCXX|GLIBC|CXXABI)_[\w.]+)", line))
    return versions


def check(root):
    checked = set()
    failures = []
    for candidate in sorted(root.rglob("*")):
        if not candidate.is_file():
            continue
        path = candidate.resolve()
        if path in checked:
            continue
        with path.open("rb") as stream:
            if stream.read(4) != b"\x7fELF":
                continue
        checked.add(path)
        output = subprocess.check_output(
            ["readelf", "--version-info", str(path)], text=True,
            env={**os.environ, "LC_ALL": "C"},
        )
        versions = required_versions(output)
        print(f"{candidate.relative_to(root)}: {', '.join(sorted(set(versions))) or 'no versioned runtime requirements'}")
        for version in versions:
            family, number = version.split("_", 1)
            if not re.fullmatch(r"\d+(?:\.\d+)+", number):
                failures.append(f"{candidate}: unsupported requirement {version}")
            elif tuple(map(int, number.split("."))) > LIMITS[family]:
                failures.append(f"{candidate}: {version} exceeds Ubuntu 22.04 baseline")
    if not checked:
        raise RuntimeError("No ELF files found in extracted AppImage")
    if failures:
        raise RuntimeError("\n".join(failures))
    print(f"ABI check passed for {len(checked)} ELF files")


if __name__ == "__main__":
    check(Path(sys.argv[1]).resolve())
