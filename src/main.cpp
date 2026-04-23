#include <iostream>

#include "StormProcessor.hpp"
#include <format>
#include <thread>

int main(int argc, char **argv)
{
    StormProcessor processor;
    std::cout << "Found " << processor.GetTimePoints().size() << " records" << std::endl;
    for (auto &time : processor.GetTimePoints())
    {
        std::string s = std::format("{:%Y/%m/%d-%H:%M}", time);
        std::cout << "Getting record at time " << s << std::endl;
        auto m = processor.Process(time);
        uint32_t size = 0;
        for (auto &azimuths : m.radials)
        {
            size += azimuths.second.size();
        }
        std::cout << s + " Num Radials: " << size << std::endl;
    }
    return 0;
}