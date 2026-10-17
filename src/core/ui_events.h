#pragma once
#include <atomic>
#include <mutex>

namespace ui_events {
inline std::atomic<bool> dirty{true};
inline std::mutex wake_mutex;
inline void (*wake)() = nullptr;

inline void set_wake(void (*callback)()) {
    std::lock_guard<std::mutex> lock(wake_mutex);
    wake = callback;
}

inline void request_redraw() {
    if (dirty.exchange(true)) return;
    std::lock_guard<std::mutex> lock(wake_mutex);
    if (wake) wake();
}
}
