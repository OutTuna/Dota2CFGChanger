#include "app.h"
#include <cassert>
#include <future>
#include <chrono>
#include <thread>
#include <iostream>

int main() {
    std::promise<void> entered, release;
    auto started = entered.get_future();
    auto proceed = release.get_future().share();
    const auto ui_thread = std::this_thread::get_id();
    bool completed = false;
    assert(file_tasks_start([&]() -> FileTaskCompletion {
        assert(std::this_thread::get_id() != ui_thread);
        entered.set_value();
        assert(proceed.wait_for(std::chrono::seconds(3)) == std::future_status::ready);
        return [&] { assert(std::this_thread::get_id() == ui_thread); completed = true; };
    }));
    assert(started.wait_for(std::chrono::seconds(3)) == std::future_status::ready);
    assert(file_tasks_busy() && !completed);
    assert(!file_tasks_start([] { return FileTaskCompletion{}; }));
    release.set_value();
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (file_tasks_busy()) {
        file_tasks_poll();
        assert(std::chrono::steady_clock::now() < deadline);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(completed);
    assert(file_tasks_start([]() -> FileTaskCompletion { throw std::runtime_error("test failure"); }));
    while (file_tasks_busy()) {
        file_tasks_poll();
        assert(std::chrono::steady_clock::now() < deadline);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(status_msg == "error" && status_detail == "test failure");
    completed = false;
    assert(file_tasks_start([&] { return [&] { completed = true; }; }));
    file_tasks_shutdown();
    assert(completed && !file_tasks_busy());
    assert(!file_tasks_start([] { return FileTaskCompletion{}; }));
    std::cout << "File tasks return immediately, serialize work and publish on the UI thread\n";
}
