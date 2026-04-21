#include <iostream>

#include "NexradAPI.hpp"
#include <thread>

int main(int argc, char **argv)
{
    NexradAPI parser;
    while (parser.ListSamples().size() < 8)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        parser.Update();
    }

    parser.GetSample(parser.ListSamples()[0]);

    return 0;
}