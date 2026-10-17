from pathlib import Path
import difflib
import hashlib
import re
import sys

project = Path(__file__).resolve().parents[1]
baseline = Path(sys.argv[1]).resolve()
recipe = project / "packaging/aur"
parts = []
for name in (recipe / "source-files.txt").read_text().splitlines():
    old, new = baseline / name, project / name
    before = old.read_text(encoding="utf-8").splitlines(keepends=True) if old.exists() else []
    after = new.read_text(encoding="utf-8").splitlines(keepends=True)
    for line in difflib.unified_diff(before, after,
        fromfile="a/" + name if old.exists() else "/dev/null", tofile="b/" + name):
        parts.append(line)
        if not line.endswith("\n"):
            parts.append("\n\\ No newline at end of file\n")
patch = recipe / "packaging-support.patch"
patch.write_text("".join(parts), encoding="utf-8", newline="\n")
digest = hashlib.sha256(patch.read_bytes()).hexdigest()
pkgbuild = recipe / "PKGBUILD"
text = pkgbuild.read_text(encoding="utf-8")
text, count = re.subn(r"(?m)^            '[^']+'\)$", "            '" + digest + "')", text)
if count != 1:
    raise SystemExit("Expected the last SHA-256 entry to describe the packaging patch")
pkgbuild.write_text(text, encoding="utf-8", newline="\n")
print("Patch refreshed. Regenerate .SRCINFO with makepkg --printsrcinfo > .SRCINFO on Arch.")
