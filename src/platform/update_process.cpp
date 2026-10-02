#include "update_process.h"
#include <stdexcept>
#include <chrono>
#include <thread>
#include <cstdlib>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#else
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <spawn.h>
#include <cerrno>
extern char** environ;
#endif

namespace fs = std::filesystem;
#ifdef _WIN32
namespace {
std::wstring widen(const std::string& value) { return fs::u8path(value).wstring(); }
std::wstring quote(const std::wstring& value) {
    std::wstring result = L"\"";
    unsigned slashes = 0;
    for (wchar_t c : value) {
        if (c == L'\\') { ++slashes; continue; }
        result.append(c == L'"' ? slashes * 2 + 1 : slashes, L'\\');
        slashes = 0;
        result += c;
    }
    result.append(slashes * 2, L'\\');
    return result + L"\"";
}
}
#endif

fs::path running_update_target() {
#ifdef _WIN32
    std::vector<wchar_t> path(32768);
    DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (!length || length >= path.size()) throw std::runtime_error("Cannot locate running executable");
    return fs::canonical(fs::path(path.data()));
#elif defined(__linux__)
    const char* appimage = std::getenv("APPIMAGE");
    if (!appimage || !*appimage) throw std::runtime_error("Automatic installation requires an AppImage; use the release page for this build");
    auto path = fs::canonical(fs::u8path(appimage));
    if (!fs::is_regular_file(path)) throw std::runtime_error("Cannot locate running AppImage");
    return path;
#else
    throw std::runtime_error("Automatic installation is supported on Windows and Linux AppImage builds");
#endif
}

std::uint64_t current_process_id() {
#ifdef _WIN32
    return GetCurrentProcessId();
#else
    return static_cast<std::uint64_t>(getpid());
#endif
}

UpdateProcess watch_update_parent(std::uint64_t id) {
    if (!id || id == current_process_id()) throw std::runtime_error("Invalid updater parent process");
#ifdef _WIN32
    HANDLE handle = OpenProcess(SYNCHRONIZE, FALSE, static_cast<DWORD>(id));
    if (!handle) throw std::runtime_error("Cannot wait for the running application");
    return {id, reinterpret_cast<std::uintptr_t>(handle)};
#else
    if (kill(static_cast<pid_t>(id), 0) != 0) throw std::runtime_error("Cannot find the running application");
    return {id, 0};
#endif
}

UpdateProcess launch_update_process(const fs::path& executable, const std::vector<std::string>& arguments) {
#ifdef _WIN32
    std::wstring command = quote(executable.wstring());
    for (const auto& arg : arguments) command += L" " + quote(widen(arg));
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    auto cwd = executable.parent_path().wstring();
    if (!CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, FALSE,
        CREATE_NO_WINDOW, nullptr, cwd.c_str(), &startup, &process))
        throw std::runtime_error("Cannot start updater or application (Windows error " + std::to_string(GetLastError()) + ")");
    CloseHandle(process.hThread);
    return {process.dwProcessId, reinterpret_cast<std::uintptr_t>(process.hProcess)};
#else
    std::vector<std::string> values{executable.string()};
    values.insert(values.end(), arguments.begin(), arguments.end());
    std::vector<char*> argv;
    for (auto& value : values) argv.push_back(value.data());
    argv.push_back(nullptr);
    std::vector<std::string> environment;
    for (char** entry = environ; *entry; ++entry) {
        std::string value = *entry;
        auto name = value.substr(0, value.find('='));
        if (name != "APPIMAGE" && name != "APPDIR" && name != "OWD" && name != "ARGV0" &&
            name != "LD_LIBRARY_PATH" && name != "APPIMAGE_EXTRACT_AND_RUN")
            environment.push_back(std::move(value));
    }
    environment.push_back("APPIMAGE_EXTRACT_AND_RUN=1");
    std::vector<char*> env;
    for (auto& value : environment) env.push_back(value.data());
    env.push_back(nullptr);
    pid_t pid = 0;
    posix_spawnattr_t attributes;
    int error = posix_spawnattr_init(&attributes);
    if (error) throw std::runtime_error("Cannot initialize updater process");
    error = posix_spawnattr_setflags(&attributes, POSIX_SPAWN_SETPGROUP);
    if (!error) error = posix_spawnattr_setpgroup(&attributes, 0);
    if (!error) error = posix_spawn(&pid, executable.c_str(), nullptr, &attributes, argv.data(), env.data());
    posix_spawnattr_destroy(&attributes);
    if (error) throw std::runtime_error("Cannot start updater or application (error " + std::to_string(error) + ")");
    return {static_cast<std::uint64_t>(pid), 1};
#endif
}

bool update_process_exited(const UpdateProcess& process, unsigned milliseconds) {
#ifdef _WIN32
    DWORD result = WaitForSingleObject(reinterpret_cast<HANDLE>(process.handle), milliseconds);
    if (result == WAIT_FAILED) throw std::runtime_error("Cannot check updater process");
    return result == WAIT_OBJECT_0;
#else
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds);
    do {
        if (process.handle) {
            int status = 0;
            pid_t result = waitpid(static_cast<pid_t>(process.id), &status, WNOHANG);
            if (result > 0 || (result < 0 && errno == ECHILD)) return true;
        }
        if (kill(static_cast<pid_t>(process.id), 0) != 0 && errno == ESRCH) return true;
        if (!milliseconds) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    } while (std::chrono::steady_clock::now() < deadline);
    return false;
#endif
}

void stop_update_process(const UpdateProcess& process) {
#ifdef _WIN32
    TerminateProcess(reinterpret_cast<HANDLE>(process.handle), 1);
#else
    kill(-static_cast<pid_t>(process.id), SIGKILL);
#endif
    update_process_exited(process, 5000);
}

void release_update_process(UpdateProcess& process) {
#ifdef _WIN32
    if (process.handle) CloseHandle(reinterpret_cast<HANDLE>(process.handle));
#endif
    process = {};
}

std::vector<std::string> update_arguments(int argc, char** argv) {
#ifdef _WIN32
    int count = 0;
    LPWSTR* values = CommandLineToArgvW(GetCommandLineW(), &count);
    if (!values) throw std::runtime_error("Cannot read command line");
    std::vector<std::string> result;
    for (int i = 0; i < count; ++i) result.push_back(fs::path(values[i]).u8string());
    LocalFree(values);
    return result;
#else
    return {argv, argv + argc};
#endif
}

void show_update_install_error(const std::string& detail) {
#ifdef _WIN32
    MessageBoxW(nullptr, widen(detail).c_str(), L"Dota 2 CFG Changer update", MB_OK | MB_ICONERROR);
#else
    try {
        auto process = launch_update_process("/usr/bin/zenity", {"--error", "--text=" + detail});
        release_update_process(process);
    } catch (...) {}
#endif
}
