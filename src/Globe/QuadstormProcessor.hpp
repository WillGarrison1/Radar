#pragma once

#include <map>
#include <Radar/StormProcessor.hpp>
#include "Quadsphere.hpp"

class QuadstormProcessor
{
public:
    QuadstormProcessor();
    ~QuadstormProcessor();

private:
    std::map<SampleTimePoint, Quadsphere> icospheres;
    
};