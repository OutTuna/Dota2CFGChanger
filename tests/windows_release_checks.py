from pathlib import Path
import re
import subprocess
import sys

executable = Path(sys.argv[1]).resolve()
result = subprocess.run(["dumpbin", "/dependents", str(executable)],
                        capture_output=True, text=True, check=True)
dependencies = re.findall(r"^\s+([^\s]+\.dll)\s*$", result.stdout, re.MULTILINE | re.IGNORECASE)
if not dependencies:
    raise SystemExit("No PE dependencies found; inspect dumpbin output")
runtime = [name for name in dependencies if re.match(r"(?:msvcp|vcruntime|concrt|ucrtbased)\d*", name, re.IGNORECASE)]
if runtime:
    raise SystemExit("Standalone release requires redistributable DLLs: " + ", ".join(runtime))
print("Standalone PE dependencies passed: " + ", ".join(dependencies))
