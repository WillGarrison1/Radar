#include "StormCache.hpp"

#include <iostream>

StormCache::StormCache() : pending({}), queue({}), worker([&]
                                                          { this->queue.WorkerLoop(); })
{
}

StormCache::~StormCache()
{
    this->queue.Stop();
    this->worker.join();
}

void StormCache::Process(SampleTimePoint timePoint)
{
    if (this->pending.contains(timePoint.time_since_epoch().count()))
    {
        return; // already being processed or downloaded
    }

    pending.emplace(timePoint.time_since_epoch().count());
    queue.Push([this, timePoint]()
               {
                auto start = std::chrono::steady_clock::now();
                auto result = CreateScan(std::move(timePoint)); 
                auto end = std::chrono::steady_clock::now();
                std::cout << "\nProcess took: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
                
                std::lock_guard lock(this->cacheMutex);
                this->cache[timePoint] = result;
                this->pending.erase(timePoint.time_since_epoch().count()); });
    return;
}