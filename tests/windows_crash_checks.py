import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time

if os.name != "nt":
    raise SystemExit("Run from an x64 MSVC developer shell on Windows")

project = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="dota-windows-crash-") as directory:
    root = Path(directory)
    header = root / "hooks.h"
    header.write_text(r'''
#include <filesystem>
void crash_copy(const std::filesystem::path&, const std::filesystem::path&, std::filesystem::copy_options);
void crash_rename(const std::filesystem::path&, const std::filesystem::path&);
void crash_rename(const std::filesystem::path&, const std::filesystem::path&, std::error_code&);
''')
    source = (project / "src/core/file_ops.cpp").read_text(encoding="utf-8")
    (root / "instrumented.cpp").write_text('#include "hooks.h"\n' + source.replace(
        "fs::copy(", "crash_copy(").replace("fs::rename(", "crash_rename("), encoding="utf-8")
    (root / "driver.cpp").write_text(r'''
#include "file_ops.h"
#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>
namespace fs = std::filesystem;
fs::path fixture;
std::wstring phase;
void pause_at(const wchar_t* value) {
    if (phase != value) return;
    std::ofstream(fixture / "ready") << "ready";
    for (;;) std::this_thread::sleep_for(std::chrono::milliseconds(10));
}
void crash_copy(const fs::path& a, const fs::path& b, fs::copy_options options) {
    fs::copy(a, b, options);
    pause_at(L"staged");
}
void crash_rename(const fs::path& a, const fs::path& b) {
    fs::rename(a, b);
    pause_at(b.filename() == "570.dotamanager_backup" ? L"saved" : L"installed");
}
void crash_rename(const fs::path& a, const fs::path& b, std::error_code& error) { fs::rename(a, b, error); }
int wmain(int argc, wchar_t** argv) {
    if (argc != 3) return 3;
    fixture = argv[1]; phase = argv[2];
    try { replace_config_directory(fixture / "source/570", fixture / "destination/570"); }
    catch (const ConfigFileError& error) { std::cout << error.key; return 2; }
    return 0;
}
''', encoding="utf-8")
    executable = root / "crash_checks.exe"
    subprocess.run(["cl", "/nologo", "/std:c++17", "/O2", "/MT", "/utf-8", "/EHsc",
                    "/I" + str(project / "src/core"), str(root / "instrumented.cpp"),
                    str(root / "driver.cpp"), "/Fe:" + str(executable), "/link", "advapi32.lib"],
                   cwd=root, check=True)
    for phase in ("staged", "saved", "installed"):
        fixture = root / (phase + " пробел Unicode")
        source = fixture / "source/570"
        destination = fixture / "destination/570"
        temporary = destination.with_name("570.dotamanager_tmp")
        backup = destination.with_name("570.dotamanager_backup")
        lock = destination.with_name("570.dotamanager_lock")
        source.mkdir(parents=True)
        destination.mkdir(parents=True)
        (source / "config").write_bytes(b"new")
        (destination / "config").write_bytes(b"old")
        child = subprocess.Popen([str(executable), str(fixture), phase])
        try:
            deadline = time.monotonic() + 10
            while not (fixture / "ready").exists():
                assert child.poll() is None, phase
                assert time.monotonic() < deadline, phase
                time.sleep(0.01)
            contender = subprocess.run([str(executable), str(fixture), "none"], capture_output=True)
            assert contender.returncode == 2 and contender.stdout == b"operation_locked", phase
        finally:
            child.kill()
            child.wait(timeout=5)
        assert lock.is_dir(), phase
        old = backup if backup.exists() else destination
        assert (old / "config").read_bytes() == b"old", phase
        if phase == "saved":
            assert not destination.exists() and temporary.is_dir()
        if phase == "installed":
            assert (destination / "config").read_bytes() == b"new" and not temporary.exists()
        recovery = fixture / "preserved"
        recovery.mkdir()
        for path in (destination, temporary, backup):
            if path.exists():
                shutil.copytree(path, recovery / path.name)
        if backup.exists():
            if destination.exists():
                destination.rename(fixture / "installed-before-recovery")
            backup.rename(destination)
        if temporary.exists():
            temporary.rename(fixture / "staging-before-recovery")
        assert (destination / "config").read_bytes() == b"old", phase
        lock.rmdir()
        subprocess.run([str(executable), str(fixture), "none"], check=True)
        assert (destination / "config").read_bytes() == b"new" and not lock.exists(), phase
        print(phase + ": process kill, competing process, preserved old state, manual recovery and retry passed")
