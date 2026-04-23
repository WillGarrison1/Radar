#pragma once

#include <memory>

#include "NexradAPI.hpp"

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
    NexradAPI nexradAPI;
};