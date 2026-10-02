import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time


def wait_until(condition, description, seconds=30):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        if condition():
            return
        time.sleep(0.1)
    raise RuntimeError(description)


def check(image):
    with tempfile.TemporaryDirectory(prefix="dotamanager-restart-") as tmp:
        folder = Path(tmp) / "path with spaces and кириллица"
        folder.mkdir()
        target = folder / "DotaManager.AppImage"
        staged = Path(str(target) + ".update-new")
        backup = Path(str(target) + ".update-old")
        helper = folder / "helper.AppImage"
        for path in (target, staged, helper):
            shutil.copy2(image, path)
            path.chmod(0o700)
        parent = subprocess.Popen(["sleep", "60"])
        job = folder / "job.json"
        digest = hashlib.sha256(staged.read_bytes()).hexdigest()
        job.write_text(json.dumps({"parent": parent.pid, "target": str(target),
                                   "staged": str(staged), "digest": digest,
                                   "size": staged.stat().st_size}))
        environment = os.environ.copy()
        environment["APPIMAGE_EXTRACT_AND_RUN"] = "1"
        process = None
        try:
            with (folder / "helper.log").open("w+") as log:
                process = subprocess.Popen([str(helper), "--apply-update", str(job)],
                                           env=environment, stdout=log, stderr=log)
                wait_until(lambda: (folder / "ready").exists() or process.poll() is not None,
                           "AppImage updater did not become ready")
                if process.poll() is not None:
                    raise RuntimeError("AppImage updater exited before becoming ready")
                if not staged.exists() or backup.exists():
                    raise RuntimeError("Updater changed executable before parent exit")
                parent.terminate()
                parent.wait(timeout=5)
                if process.wait(timeout=40) != 0:
                    raise RuntimeError("AppImage update helper failed")
                windows = subprocess.check_output(["xwininfo", "-root", "-tree"], text=True)
                if "Dota 2 CFG Changer" not in windows:
                    raise RuntimeError("Updated AppImage did not leave its main window open")
                if staged.exists() or backup.exists() or job.exists():
                    raise RuntimeError("AppImage replacement was not completed")
                if hashlib.sha256(target.read_bytes()).hexdigest() != digest:
                    raise RuntimeError("Installed AppImage checksum differs from verified download")
                print("AppImage helper replaced the file and restarted the application successfully")
        finally:
            if parent.poll() is None:
                parent.kill()
                parent.wait()
            if process is not None and process.poll() is None:
                process.kill()
                process.wait()
            print((folder / "helper.log").read_text() if (folder / "helper.log").exists() else "")


if __name__ == "__main__":
    check(Path(sys.argv[1]).resolve())
