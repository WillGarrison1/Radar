#include "TaskQueue.hpp"

void TaskQueue::Push(std::function<void()> task)
{
    {
        std::lock_guard lock(mutex);
        queue.push(std::move(task));
    }
    cv.notify_one();
}

void TaskQueue::Stop()
{
    {
        std::lock_guard lock(mutex);
        stopped = true;
    }
    cv.notify_all();
}

bool TaskQueue::WorkerWaitForTask(std::function<void()> &task)
{
    std::unique_lock lock(mutex);
    cv.wait(lock, [this]
            { return !queue.empty() || stopped; });

    if (stopped)
        return true;

    task = std::move(queue.front());
    queue.pop();
    return false;
}

void TaskQueue::WorkerLoop()
{
    while (true)
    {
        std::function<void()> task;
        bool quit = WorkerWaitForTask(task);

        if (quit)
            break;

        task();
    }
}