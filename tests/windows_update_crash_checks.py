import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time

if os.name != "nt" or len(sys.argv) != 2:
    raise SystemExit("Run from an x64 MSVC developer shell: python tests/windows_update_crash_checks.py <build>")

project = Path(__file__).resolve().parents[1]
build = Path(sys.argv[1]).resolve()
library = build / "Release/dotamanager_core.lib"
if not library.is_file():
    raise SystemExit("Build the MSVC Release core first")
cache = (build / "CMakeCache.txt").read_text(encoding="utf-8")
json_source = next((line.split("=", 1)[1] for line in cache.splitlines()
                    if line.startswith("nlohmann_json_SOURCE_DIR:")), "")
json_include = Path(json_source) / "include" if json_source else build / "_deps/json-src/include"
if not (json_include / "nlohmann/json.hpp").is_file():
    raise SystemExit("Cannot find the configured nlohmann/json headers")

with tempfile.TemporaryDirectory(prefix="dota-update-crash-") as directory:
    root = Path(directory)
    (root / "hooks.h").write_text(r'''
#include <filesystem>
void crash_rename(const std::filesystem::path&, const std::filesystem::path&);
void crash_rename(const std::filesystem::path&, const std::filesystem::path&, std::error_code&);
bool crash_remove(const std::filesystem::path&, std::error_code&);
''')
    source = (project / "src/core/update_install.cpp").read_text(encoding="utf-8")
    (root / "instrumented.cpp").write_text('#include "hooks.h"\n' + source.replace(
        "fs::rename(", "crash_rename(").replace("fs::remove(", "crash_remove("), encoding="utf-8")
    (root / "driver.cpp").write_text(r'''
#include "update_install.h"
#include "checksum.h"
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
void crash_rename(const fs::path& a, const fs::path& b) {
    fs::rename(a, b);
    pause_at(b.extension() == ".update-old" ? L"saved" : L"installed");
}
void crash_rename(const fs::path& a, const fs::path& b, std::error_code& error) { fs::rename(a, b, error); }
bool crash_remove(const fs::path& path, std::error_code& error) {
    if (path.extension() == ".update-old") pause_at(L"acknowledged");
    return fs::remove(path, error);
}
int wmain(int argc, wchar_t** argv) {
    if (argc != 3) return 3;
    fixture = argv[1]; phase = argv[2];
    const auto target = fixture / "application.exe";
    auto staged = target; staged += ".update-new";
    ReleaseInfo release;
    release.size = fs::file_size(staged); release.digest = file_sha256(staged);
    try {
        replace_update_executable(staged, target, release, [] {
            std::ofstream(fixture / "launch-confirmed") << "confirmed";
            return true;
        });
    } catch (const std::exception& error) { std::cout << error.what(); return 2; }
    return 0;
}
''', encoding="utf-8")
    executable = root / "update_crash_checks.exe"
    subprocess.run(["cl", "/nologo", "/std:c++17", "/O2", "/MT", "/utf-8", "/EHsc",
                    "/I" + str(project / "src"), "/I" + str(project / "src/core"),
                    "/I" + str(project / "src/platform"), "/I" + str(build / "generated"),
                    "/I" + str(json_include),
                    str(root / "instrumented.cpp"), str(root / "driver.cpp"), str(library),
                    "/Fe:" + str(executable), "/link", "bcrypt.lib", "advapi32.lib",
                    "shell32.lib", "ole32.lib", "user32.lib"], cwd=root, check=True)
    for phase in ("saved", "installed", "acknowledged"):
        fixture = root / (phase + " пробел Unicode")
        fixture.mkdir()
        target = fixture / "application.exe"
        staged = fixture / "application.exe.update-new"
        backup = fixture / "application.exe.update-old"
        target.write_bytes(b"old application bytes")
        staged.write_bytes(b"new application bytes")
        child = subprocess.Popen([str(executable), str(fixture), phase])
        try:
            deadline = time.monotonic() + 10
            while not (fixture / "ready").exists():
                assert child.poll() is None, phase
                assert time.monotonic() < deadline, phase
                time.sleep(0.01)
        finally:
            child.kill()
            child.wait(timeout=5)
        assert backup.read_bytes() == b"old application bytes", phase
        if phase == "saved":
            assert not target.exists() and staged.read_bytes() == b"new application bytes"
        else:
            assert target.read_bytes() == b"new application bytes" and not staged.exists()
        assert (fixture / "launch-confirmed").exists() == (phase == "acknowledged")
        preserved = fixture / "preserved"
        preserved.mkdir()
        for path in (target, staged, backup):
            if path.exists():
                shutil.copy2(path, preserved / path.name)
        if target.exists():
            target.rename(fixture / "new-before-recovery")
        backup.rename(target)
        assert target.read_bytes() == b"old application bytes"
        if not staged.exists():
            staged.write_bytes(b"new application bytes")
        subprocess.run([str(executable), str(fixture), "none"], check=True)
        assert target.read_bytes() == b"new application bytes" and not backup.exists()
        print(phase + ": real process kill in replacement transaction, preserved old bytes, manual recovery and retry passed")
