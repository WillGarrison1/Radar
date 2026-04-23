#pragma once

#include <unordered_map>

#include "NexradAPI.hpp"

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
    float trueAzimuth; // actual non-rounded azimuth
    float trueElevation; // actual non-rounded elevation
};

struct VolumeScan
{
    /*
        radial = radials[round(elevation*10)][rounded(azimuth*10)];
    */
    std::unordered_map<uint16_t, std::unordered_map<uint16_t, Radial>> radials;
};

class StormProcessor
{
public:
    StormProcessor();
    ~StormProcessor();

    void Refresh();
    void Update();

    std::vector<SampleTimePoint> GetTimePoints();
    VolumeScan Process(SampleTimePoint timestep);

private:
    NexradAPI nexradAPI;
};