#pragma once

#include <map>

#include "NexradAPI.hpp"
#include <condition_variable>
#include <mutex>
#include <functional>
#include <queue>

struct Gate
{
    float reflectivity;
    float velocity;
};

struct Radial
{
    std::vector<Gate> gates;
    uint16_t gateSize;
    uint16_t firstGate;
    float trueAzimuth;   // actual non-rounded azimuth
    float trueElevation; // actual non-rounded elevation
};

struct VolumeScan
{
    /*
        radial = radials[elevation][azimuth];
    */
    std::map<uint16_t, std::vector<Radial>> radials;
};

class TaskQueue
{
public:
    void Push(std::function<void()> task)
    {
        {
            std::lock_guard lock(mutex);
            queue.push(std::move(task));
        }
        cv.notify_one();
    }

    void WorkerLoop()
    {
        while (true)
        {
            std::function<void()> task;
            {
                std::unique_lock lock(mutex);
                cv.wait(lock, [this]
                        { return !queue.empty() || stopped; });
                if (stopped && queue.empty())
                    return;
                task = std::move(queue.front());
                queue.pop();
            }
            task();
        }
    }

    void Stop()
    {
        {
            std::lock_guard lock(mutex);
            stopped = true;
        }
        cv.notify_all();
    }

private:
    std::queue<std::function<void()>> queue;
    std::mutex mutex;
    std::condition_variable cv;
    bool stopped = false;
};

class StormProcessor
{
public:
    StormProcessor();
    ~StormProcessor();

    void Refresh();
    void Update();

    std::vector<SampleTimePoint> GetTimePoints();
    void Process(SampleTimePoint timestep);

private:
    VolumeScan _Process(ArchiveII archive);

    std::mutex cacheMutex;
    std::map<SampleTimePoint, VolumeScan> cache;
    NexradAPI nexradAPI;
    std::thread worker;
    TaskQueue queue;
};