#pragma once

#include <map>
#include <Radar/StormProcessor.hpp>
#include "Icosphere.hpp"

class IcostormProcessor
{
public:
    IcostormProcessor();
    ~IcostormProcessor();

private:
    std::map<SampleTimePoint, Icosphere> icospheres;
    
};