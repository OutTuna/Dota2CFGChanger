#pragma once
#include <functional>

using FileTaskCompletion = std::function<void()>;
bool file_tasks_start(std::function<FileTaskCompletion()> task);
bool file_tasks_busy();
void file_tasks_poll();
void file_tasks_request_stop();
void file_tasks_shutdown();
