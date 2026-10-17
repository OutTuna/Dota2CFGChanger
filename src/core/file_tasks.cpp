#include "file_tasks.h"
#include "app.h"
#include <thread>
#include <utility>

namespace {
std::thread worker;
std::atomic<bool> busy{false};
std::atomic<bool> stopping{false};
std::atomic<bool> ready{false};
std::mutex mutex;
FileTaskCompletion completion;
FileTaskCompletion failed(std::string detail) {
    return [detail = std::move(detail)] {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        status_msg = "error";
        status_detail = detail;
        app_status_changed();
    };
}
}

bool file_tasks_start(std::function<FileTaskCompletion()> task) {
    if (stopping.load() || g_scanning.load() || busy.exchange(true)) return false;
    try {
        if (worker.joinable()) worker.join();
        worker = std::thread([task = std::move(task)] {
            FileTaskCompletion result;
            try { result = task(); }
            catch (const std::exception& error) {
                result = failed(error.what());
            } catch (...) { result = failed("Background file operation failed"); }
            {
                std::lock_guard<std::mutex> lock(mutex);
                completion = std::move(result);
                ready = true;
            }
            ui_events::request_redraw();
        });
        return true;
    } catch (const std::exception& error) {
        busy = false;
        failed(error.what())();
        return false;
    }
}

bool file_tasks_busy() { return busy.load(); }

void file_tasks_poll() {
    if (!ready.exchange(false)) return;
    FileTaskCompletion result;
    {
        std::lock_guard<std::mutex> lock(mutex);
        result = std::move(completion);
    }
    busy = false;
    if (result) result();
    ui_events::request_redraw();
}

void file_tasks_request_stop() { stopping = true; }

void file_tasks_shutdown() {
    file_tasks_request_stop();
    if (worker.joinable()) worker.join();
    file_tasks_poll();
}
