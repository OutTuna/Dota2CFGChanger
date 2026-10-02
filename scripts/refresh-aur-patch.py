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
    before = old.read_text().splitlines(keepends=True) if old.exists() else []
    after = new.read_text().splitlines(keepends=True)
    parts.extend(difflib.unified_diff(before, after,
        fromfile="a/" + name if old.exists() else "/dev/null", tofile="b/" + name))
patch = recipe / "packaging-support.patch"
patch.write_text("".join(parts))
digest = hashlib.sha256(patch.read_bytes()).hexdigest()
pkgbuild = recipe / "PKGBUILD"
text = pkgbuild.read_text()
text, count = re.subn(r"(?m)^            '[^']+'\)$", "            '" + digest + "')", text)
if count != 1:
    raise SystemExit("Expected the last SHA-256 entry to describe the packaging patch")
pkgbuild.write_text(text)
print("Patch refreshed. Regenerate .SRCINFO with makepkg --printsrcinfo > .SRCINFO on Arch.")
