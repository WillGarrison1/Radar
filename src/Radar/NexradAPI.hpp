#pragma once

#include <chrono>
#include <string>
#include <tinyxml2.h>
#include <vector>
#include <cpr/cpr.h>

using SampleTimePoint = std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>;

struct SampleMetaData
{
    SampleTimePoint time;
    std::string key;
    size_t size;
};

#pragma pack(1)
struct ArchiveIIHeader
{
    char filename[9];
    char extensionNumber[3];
    uint32_t date;
    uint32_t time;
    char radar[4];
};

struct MessageHeader
{
    uint16_t size;
    uint8_t channel;
    uint8_t type;
    uint16_t seq_num;
    uint16_t date;
    uint32_t timeMS;
    uint16_t num_segs;
    uint16_t seg_num;
};
#pragma pack()

struct Message
{
    MessageHeader header;
    size_t index;
};

struct Record
{
    int32_t compresssedSize;
    std::vector<uint8_t> data;
    std::vector<Message> messages;
};

struct ArchiveII
{
    ArchiveIIHeader header;
    std::vector<Record> records;
};

constexpr uint32_t ToSysOrderL(uint32_t data)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return ((data & 0xff) << 24) |
           ((data & 0xff00) << 8) |
           ((data & 0xff0000) >> 8) |
           ((data & 0xff000000) >> 24);
#else
    return data;
#endif
}

constexpr uint16_t ToSysOrderS(uint16_t data)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return ((data & 0xff) << 8) |
           ((data & 0xff00) >> 8);
#else
    return data;
#endif
}

constexpr float ToSysOrderF(float f)
{
    return std::bit_cast<float>(ToSysOrderL(std::bit_cast<uint32_t>(f)));
}

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
    size_t ParseRecord(Record &record, const char *data);

    tinyxml2::XMLDocument radarSampleMetadataXML;
    std::vector<SampleMetaData> radarSamplesMeta;

    std::array<cpr::Session, 4> sessions;
};