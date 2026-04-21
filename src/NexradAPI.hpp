#pragma once

#include "WeatherData.hpp"
#include <chrono>
#include <string>
#include <tinyxml2.h>
#include <vector>

using SampleTimePoint = std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>;

struct SampleMetaData
{
    SampleTimePoint time;
    std::string key;
    size_t size;
};

struct ArchiveIIHeader
{
    char filename[9];
    char extensionNumber[3];
    uint32_t date;
    uint32_t time;
    char radar[4];
};

struct CompressedRecord
{
    int32_t compresssedSize;
    std::vector<uint8_t> data;
};

struct ArchiveII
{
    ArchiveIIHeader header;
    std::vector<CompressedRecord> records;
};

class NexradAPI
{
public:
    NexradAPI();
    ~NexradAPI();

    /**
     * @brief refreshes the list of available radar samples
     *
     */
    void Update();

    /**
     * @brief returns a list of all the possible radar samples that can be downloaded
     *
     * @return std::vector<SampleMetaData>&
     */
    std::vector<SampleMetaData> &ListSamples();

    ArchiveII GetSample(SampleMetaData meta);

private:
    SampleMetaData ParseSampleMeta(tinyxml2::XMLElement *element);

    tinyxml2::XMLDocument radarSampleMetadataXML;
    std::vector<SampleMetaData> radarSamplesMeta;
};