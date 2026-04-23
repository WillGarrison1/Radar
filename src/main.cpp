#include <iostream>

#include "StormProcessor.hpp"
#include <thread>

int main(int argc, char **argv)
{
    StormProcessor processor;
    processor.Process(processor.GetTimePoints()[1]);
    return 0;
}