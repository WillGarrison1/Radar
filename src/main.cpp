#include <iostream>
#include <format>
#include <thread>

#include "RadarRenderer.hpp"
#include "StormProcessor.hpp"

int main(int argc, char **argv)
{
    StormProcessor processor;
    std::cout << "Found " << processor.GetTimePoints().size() << " records" << std::endl;
    auto time = processor.GetTimePoints().back();
    std::string s = std::format("{:%Y/%m/%d-%H:%M}", time);
    std::cout << "Getting record at time " << s << std::endl;
    auto m = processor.Process(time);
    std::cout << "Num elevations: " << m.radials.size() << "\nElevations:";
    uint32_t size = 0;
    for (auto &azimuths : m.radials)
    {
        std::cout << " " << azimuths.first / 10.0f << ",";
        size += azimuths.second.size();
    }
    std::cout << "\n"
              << s + " Num Radials: " << size << std::endl;

    RadarRenderer renderer;
    while (!renderer.ShouldQuit())
    {
        renderer.Update(m);
    }

    return 0;
}