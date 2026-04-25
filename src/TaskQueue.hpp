#pragma once

#include <queue>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>

class TaskQueue
{
public:
    TaskQueue() = default;
    ~TaskQueue() = default;

    void Push(std::function<void()> task);
    void WorkerLoop();
    void Stop();

private:
    bool WorkerWaitForTask(std::function<void()>& task);

    std::queue<std::function<void()>> queue;
    std::mutex mutex;
    std::condition_variable cv;
    bool stopped = false;
};
