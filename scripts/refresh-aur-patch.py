from pathlib import Path
import difflib
import hashlib
import re
import sys

project = Path(__file__).resolve().parents[1]
baseline = Path(sys.argv[1]).resolve()
recipe = project / "packaging/aur"
parts = []
for name in (recipe / "source-files.txt").read_text(encoding="utf-8").splitlines():
    old, new = baseline / name, project / name
    before = old.read_text(encoding="utf-8").splitlines(keepends=True) if old.exists() else []
    after = new.read_text(encoding="utf-8").splitlines(keepends=True)
    for line in difflib.unified_diff(before, after,
        fromfile="a/" + name if old.exists() else "/dev/null", tofile="b/" + name):
        parts.append(line)
        if not line.endswith("\n"):
            parts.append("\n\\ No newline at end of file\n")
patch = recipe / "packaging-support.patch"
patch_text = "".join(parts)
digest = hashlib.sha256(patch_text.encode("utf-8")).hexdigest()
pkgbuild = recipe / "PKGBUILD"
text = pkgbuild.read_text(encoding="utf-8")
text, count = re.subn(r"(?m)^            '[^']+'\)$", "            '" + digest + "')", text)
if count != 1:
    raise SystemExit("Expected the last SHA-256 entry to describe the packaging patch")
metadata = recipe / ".SRCINFO"
info = metadata.read_text(encoding="utf-8")
checksums = list(re.finditer(r"(?m)^(\s*sha256sums = )[^\n]+$", info))
if not checksums:
    raise SystemExit("Missing patch checksum in .SRCINFO")
last = checksums[-1]
info = info[:last.start()] + last.group(1) + digest + info[last.end():]
patch.write_text(patch_text, encoding="utf-8", newline="\n")
pkgbuild.write_text(text, encoding="utf-8", newline="\n")
metadata.write_text(info, encoding="utf-8", newline="\n")
print("Patch and patch checksums refreshed")
