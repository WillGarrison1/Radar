#include <iostream>

#include "NexradAPI.hpp"

int main(int argc, char **argv)
{
    NexradAPI parser;
    parser.GetSample(parser.ListSamples()[0]);

    return 0;
}