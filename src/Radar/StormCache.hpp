#pragma once

#include <map>
#include <thread>
#include <unordered_set>
#include <mutex>

#include "StormProcessor.hpp"

class StormCache : public StormProcessor
{
public:
    StormCache();
    ~StormCache();

    inline std::map<SampleTimePoint, RadarScan> &GetCached()
    {
        return cache;
    }

    inline std::vector<SampleTimePoint> GetPending()
    {
        std::vector<SampleTimePoint> vec;
        for (uint64_t point : pending)
        {
            SampleTimePoint tp(std::chrono::duration_cast<SampleTimePoint::duration>(std::chrono::nanoseconds{point}));
            vec.push_back(tp);
        }
        return vec;
    }

    void Process(SampleTimePoint timestep);

private:
    std::map<SampleTimePoint, RadarScan> cache;
    std::unordered_set<uint64_t> pending;

    TaskQueue queue;
    std::thread worker;
    std::mutex cacheMutex;
};