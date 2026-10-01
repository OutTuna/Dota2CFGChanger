import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
json_include = Path(sys.argv[1]) if len(sys.argv) > 1 else root / "build/_deps/json-src/include"
if not (json_include / "nlohmann/json.hpp").exists():
    raise SystemExit("Usage: python3 tests/core_checks.py <nlohmann-json-include-directory>")
locales = [json.loads((root / "resources/locales" / (code + ".json")).read_text()) for code in ("en", "ru", "uk")]
for locale in locales:
    assert set(locale) == set(locales[0])
    assert all(isinstance(value, str) and value for value in locale.values())
    for key, value in locale.items():
        assert value.count("{value}") == locales[0][key].count("{value}")
with tempfile.TemporaryDirectory(prefix="dotamanager-core-") as directory:
    temp = Path(directory)
    subprocess.run(["cmake", "-DLOCALES_DIR=" + str(root / "resources/locales"), "-DOUTPUT_DIR=" + str(temp), "-P", str(root / "cmake/EmbedLocales.cmake")], check=True)
    (temp / "faults.h").write_text(r"""
#include <filesystem>
#include <system_error>
void test_rename(const std::filesystem::path&, const std::filesystem::path&);
void test_rename(const std::filesystem::path&, const std::filesystem::path&, std::error_code&);
std::uintmax_t test_remove(const std::filesystem::path&, std::error_code&);
void test_copy(const std::filesystem::path&, const std::filesystem::path&, std::filesystem::copy_options);
""")
    source = (root / "src/core/file_ops.cpp").read_text()
    source = '#include "faults.h"\n' + source.replace("fs::rename(", "test_rename(").replace("fs::remove_all(", "test_remove(").replace("fs::copy(", "test_copy(")
    (temp / "file_ops_test.cpp").write_text(source)
    (temp / "checks.cpp").write_text(r"""
#include "file_ops.h"
#include "localization.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <system_error>
namespace fs = std::filesystem;
bool install_failure = false, restore_failure = false, save_failure = false;
bool copy_failure = false, cleanup_failure = false;
bool suffix(const fs::path& p, const char* s) { return p.u8string().find(s) != std::string::npos; }
void test_rename(const fs::path& from, const fs::path& to) {
    if ((install_failure && suffix(from, ".dotamanager_tmp")) || (save_failure && suffix(to, ".dotamanager_backup")))
        throw fs::filesystem_error("Injected rename failure", from, to, std::make_error_code(std::errc::permission_denied));
    fs::rename(from, to);
}
void test_rename(const fs::path& from, const fs::path& to, std::error_code& ec) {
    if (restore_failure) { ec = std::make_error_code(std::errc::permission_denied); return; }
    fs::rename(from, to, ec);
}
std::uintmax_t test_remove(const fs::path& path, std::error_code& ec) {
    if (cleanup_failure && suffix(path, ".dotamanager_backup")) {
        ec = std::make_error_code(std::errc::permission_denied); return 0;
    }
    return fs::remove_all(path, ec);
}
void test_copy(const fs::path& from, const fs::path& to, fs::copy_options options) {
    if (copy_failure) {
        fs::create_directory(to); std::ofstream(to / "partial") << "partial";
        throw fs::filesystem_error("Injected copy failure", from, to, std::make_error_code(std::errc::no_space_on_device));
    }
    fs::copy(from, to, options);
}
std::string read(const fs::path& path) { std::ifstream f(path); return {std::istreambuf_iterator<char>(f), {}}; }
void put(const fs::path& path, const char* text) { fs::create_directories(path.parent_path()); std::ofstream(path) << text; }
int main(int argc, char** argv) {
    auto root = fs::u8path(argv[1]);
    const auto home = root / "home";
    assert(normalize_user_path("~/папка/../конфіг") == (home / fs::u8path("конфіг")).u8string());
    assert(normalize_user_path("").empty());
    auto flatpak = home / ".var/app/com.valvesoftware.Steam/.local/share/Steam/userdata";
    fs::create_directories(flatpak);
    assert(default_steam_userdata() == flatpak.u8string());
    auto native = home / ".local/share/Steam/userdata";
    fs::create_directories(native);
    assert(default_steam_userdata() == native.u8string());
    auto xdg = root / "data/Steam/userdata";
    fs::create_directories(xdg);
    assert(default_steam_userdata() == xdg.u8string());
    auto src = root / fs::u8path("джерело/570"), dst = root / fs::u8path("назначение/570");
    put(src / "cfg/options.txt", "new"); put(dst / "cfg/options.txt", "old");
    auto temp = fs::u8path(dst.u8string() + ".dotamanager_tmp");
    auto backup = fs::u8path(dst.u8string() + ".dotamanager_backup");
    auto reset = [&] { fs::remove_all(dst); fs::remove_all(temp); fs::remove_all(backup); put(dst / "cfg/options.txt", "old"); };
    auto expect_failure = [&] { bool failed = false; try { replace_config_directory(src, dst); } catch (...) { failed = true; } assert(failed); };
    assert(replace_config_directory(src, dst).empty());
    assert(read(dst / "cfg/options.txt") == "new" && !fs::exists(temp) && !fs::exists(backup));
    reset(); copy_failure = true; expect_failure(); copy_failure = false;
    assert(read(dst / "cfg/options.txt") == "old" && !fs::exists(temp));
    save_failure = true; expect_failure(); save_failure = false;
    assert(read(dst / "cfg/options.txt") == "old" && !fs::exists(temp));
    install_failure = true; expect_failure();
    assert(read(dst / "cfg/options.txt") == "old" && !fs::exists(temp) && !fs::exists(backup));
    restore_failure = true;
    bool recovery = false;
    try { replace_config_directory(src, dst); } catch (const ConfigFileError& e) { recovery = e.key == "restore_failed" && e.detail == backup.u8string(); }
    assert(recovery && read(backup / "cfg/options.txt") == "old" && fs::exists(temp));
    install_failure = restore_failure = false;
    expect_failure(); assert(read(backup / "cfg/options.txt") == "old");
    reset(); cleanup_failure = true;
    assert(replace_config_directory(src, dst) == backup);
    assert(read(dst / "cfg/options.txt") == "new" && read(backup / "cfg/options.txt") == "old");
    cleanup_failure = false; reset();
    fs::create_directory(temp); put(temp / "preserve", "keep"); expect_failure();
    assert(read(temp / "preserve") == "keep" && read(dst / "cfg/options.txt") == "old");
    reset(); fs::remove_all(dst);
    assert(replace_config_directory(src, dst).empty() && read(dst / "cfg/options.txt") == "new");
    for (const auto& target : {src, src / "nested"}) {
        bool failed = false; try { replace_config_directory(src, target); } catch (const ConfigFileError& e) { failed = e.key == "overlap"; }
        assert(failed && read(src / "cfg/options.txt") == "new");
    }
    reset(); fs::create_symlink(src / "cfg/options.txt", src / "link"); expect_failure();
    assert(read(dst / "cfg/options.txt") == "old"); fs::remove(src / "link");
    fs::create_symlink(root / "missing", backup); expect_failure();
    assert(fs::is_symlink(fs::symlink_status(backup))); fs::remove(backup);
    assert(std::string(language_code()) == "en" && std::string(tr("ready")) == "Ready");
    set_language("ru"); assert(std::string(tr("copy")) == "Копировать конфиг");
    set_language("uk"); assert(std::string(tr("copy")) == "Копіювати конфіг");
    assert(tr_value("status", "100%") == "Статус: 100%");
    set_language("unknown"); assert(std::string(language_code()) == "en");
    assert(std::string(tr("missing_key")) == "missing_key");
    std::cout << "Paths, copy failures, rollback, recovery and translations passed\n";
}
""")
    compiler = os.environ.get("CXX", "c++")
    executable = temp / "checks"
    subprocess.run([compiler, "-std=c++17", "-I" + str(root / "src/core"), "-I" + str(root / "src/platform"), "-I" + str(root / "src/network"), "-I" + str(temp), "-I" + str(json_include), str(temp / "checks.cpp"), str(temp / "file_ops_test.cpp"), str(root / "src/core/localization.cpp"), "-o", str(executable)], check=True)
    env = dict(os.environ, HOME=str(temp / "home"), XDG_DATA_HOME=str(temp / "data"))
    subprocess.run([str(executable), str(temp)], env=env, check=True)
    app = (root / "src/core/app.cpp").read_text()
    functions = app[app.index("static void set_status("):app.index("static void fetch_profile_info")]
    settings_source = r"""
#include "app.h"
#include "file_ops.h"
#include "localization.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <algorithm>
#include <cassert>
#include <iostream>
namespace fs = std::filesystem;
using json = nlohmann::json;
std::string settings_directory;
std::string config_dir() { return settings_directory; }
""" + functions + r"""
int main(int argc, char** argv) {
    settings_directory = (fs::u8path(argv[1]) / "settings").u8string();
    fs::create_directories(settings_directory);
    load_settings(); assert(src_path[0] != '~' && std::string(language_code()) == "en");
    set_language("uk");
    assert(set_config_path(src_path, "~/дані"));
    assert(set_config_path(dst_path, (fs::u8path(argv[1]) / fs::u8path("папка з пробілами")).u8string()));
    g_theme = 3; save_settings();
    std::string source = src_path, destination = dst_path;
    set_language("en"); g_theme = 0; src_path[0] = 0; dst_path[0] = 0;
    load_settings();
    assert(std::string(language_code()) == "uk");
    assert(std::string(src_path) == source && std::string(dst_path) == destination && g_theme == 3);
    assert(!set_config_path(src_path, std::string(PATH_BUF_SIZE + 10, 'x')) && std::string(src_path) == source);
    selected_src = 0; src_list.push_back("123");
    assert(set_config_path(src_path, (fs::u8path(argv[1]) / "new_source").u8string()));
    assert(selected_src == -1 && src_list.empty());
    std::ofstream f(settings_path()); f << "{invalid"; f.close(); load_settings();
    assert(src_path[0] != 0);
    std::cout << "Settings persistence, UTF-8 paths and path length checks passed\n";
}
"""
    (temp / "settings.cpp").write_text(settings_source)
    settings_executable = temp / "settings_checks"
    subprocess.run([compiler, "-std=c++17", "-I" + str(root / "src/core"), "-I" + str(root / "src/platform"), "-I" + str(root / "src/network"), "-I" + str(temp), "-I" + str(json_include), str(temp / "settings.cpp"), str(root / "src/core/file_ops.cpp"), str(root / "src/core/localization.cpp"), "-o", str(settings_executable)], check=True)
    subprocess.run([str(settings_executable), str(temp)], env=env, check=True)
