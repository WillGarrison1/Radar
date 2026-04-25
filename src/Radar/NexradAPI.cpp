#include "NexradAPI.hpp"

#include <bxzstr.hpp>
#include <chrono>
#include <cpr/cpr.h>
#include <fstream>
#include <iostream>
#include <ranges>
#include <sstream>
#include <string>
#include <tinyxml2.h>

#include "Benchmark.hpp"

static std::string radarName = "KLSX";

NexradAPI::NexradAPI()
{
    for (int i = 0; i < this->sessions.size(); i++)
    {
        sessions[i].SetHeader(cpr::Header{{"Accept-Encoding", "identity"}});
        sessions[i].SetTimeout(cpr::Timeout{20000});
    }
    Update();
}

NexradAPI::~NexradAPI() {}

void NexradAPI::Update()
{
    auto now = std::chrono::system_clock::now();
    auto date = std::chrono::year_month_day(std::chrono::floor<std::chrono::days>(now));

    std::string dateString = std::to_string(static_cast<int32_t>(date.year()));

    std::string month = std::to_string(static_cast<uint32_t>(date.month()));
    month = std::string(2 - month.length(), '0') + month;

    std::string day = std::to_string(static_cast<uint32_t>(date.day()));
    day = std::string(2 - day.length(), '0') + day;

    dateString += "/" + month + "/" + day;

    std::string url = "https://unidata-nexrad-level2.s3.amazonaws.com/?prefix=" + dateString + "/" + radarName + "/";
    auto &session = sessions[0];
    session.SetUrl(cpr::Url{url});

    auto result = session.Get();
    if (result.status_code != 200)
    {
        throw std::runtime_error("Failed to get weather radar metadata");
    }

    auto errorCode = this->radarSampleMetadataXML.Parse(result.text.c_str(), result.downloaded_bytes);
    if (errorCode)
    {
        throw std::runtime_error("Failed to parse radar metadata!");
    }

    auto sampleBucket = this->radarSampleMetadataXML.FirstChildElement("ListBucketResult");
    if (!sampleBucket)
    {
        throw std::runtime_error("Failed to get radar sampleBucket");
    }

    radarSamplesMeta.clear();

    std::cout << "Num " + radarName + " Radar Samples: " << sampleBucket->ChildElementCount("Contents") << std::endl;
    auto radarSampleXML = sampleBucket->FirstChildElement("Contents");
    while (radarSampleXML)
    {
        auto meta = ParseSampleMeta(radarSampleXML);
        if (!meta.key.empty())
            radarSamplesMeta.push_back(meta);
        radarSampleXML = radarSampleXML->NextSiblingElement();
    }
}

SampleMetaData NexradAPI::ParseSampleMeta(tinyxml2::XMLElement *meta)
{
    if (std::string(meta->Value()) != "Contents")
    {
        std::cerr << "Invalid radar metedata" << std::endl;
        return {};
    }

    SampleMetaData sample;
    auto key = meta->FirstChildElement("Key");
    auto size = meta->FirstChildElement("Size");
    if (!key)
    {
        std::cerr << "Could not find sample key!" << std::endl;
        return {};
    }
    if (!size)
    {
        std::cerr << "Could not find sample size!" << std::endl;
        return {};
    }

    sample.key = key->GetText();
    sample.size = std::stoull(size->GetText());

    if (*(sample.key.end() - 1) != '6' && *(sample.key.end() - 1) != '8')
    {
        std::cout << "No support for non-standard archiveII files" << std::endl;
        return {};
    }

    std::tm tm{};

    size_t firstUnderscore = sample.key.find("_");
    if (firstUnderscore == std::string::npos)
    {
        throw std::runtime_error("Invalid format");
    }

    std::string cleaned = sample.key.substr(0, 10) + sample.key.substr(firstUnderscore + 1, 6);
    std::istringstream ss(cleaned);

    ss >> std::get_time(&tm, "%Y/%m/%d%H%M%S");

    if (ss.fail())
    {
        throw std::runtime_error("Failed to extract timestamp from sample");
    }

    auto tp = std::chrono::sys_days{
                  std::chrono::year{tm.tm_year + 1900} /
                  std::chrono::month{static_cast<uint32_t>(tm.tm_mon + 1)} /
                  std::chrono::day{static_cast<uint32_t>(tm.tm_mday)}} +
              std::chrono::hours{tm.tm_hour} + std::chrono::minutes{tm.tm_min} + std::chrono::seconds{tm.tm_sec};

    auto tp_seconds =
        std::chrono::time_point_cast<std::chrono::seconds>(tp);
    sample.time = tp_seconds;

    return sample;
}

std::vector<SampleMetaData> &NexradAPI::ListSamples()
{
    return radarSamplesMeta;
}

size_t NexradAPI::ParseRecord(Record &record, const char *data)
{
    record.compresssedSize = ToSysOrderL(*reinterpret_cast<const uint32_t *>(data));
    data += 4;

    size_t absSize = std::abs(record.compresssedSize); // apparently size can be negative, in this case just take the `abs` of it
    size_t actualSize = absSize + 4;

    std::string_view recordData(data, absSize);
    std::istringstream sstream((std::string)recordData);

    bxz::istream decompressor(sstream.rdbuf(), bxz::bz2);

    if (sstream.fail())
    {
        std::cerr << "Failed to decompress!!" << std::endl;
        return actualSize;
    }

    record.data.clear();
    record.data.reserve(100 * 1024);

    char chunk[16384];
    while (decompressor.read(chunk, sizeof(chunk)) || decompressor.gcount() > 0)
    {
        record.data.insert(record.data.end(), chunk, chunk + decompressor.gcount());
    }

    // parse message headers
    const uint8_t *messageCurrent = record.data.data();
    const uint8_t *recordEnd = record.data.data() + record.data.size();
    while (messageCurrent + 12 + sizeof(MessageHeader) <= recordEnd)
    {
        auto &message = record.messages.emplace_back();
        message.header = *reinterpret_cast<const MessageHeader *>(messageCurrent + 12);
        message.index = static_cast<size_t>(messageCurrent - record.data.data() + sizeof(MessageHeader) + 12);

        auto &header = message.header;
        header.date = ToSysOrderS(header.date);
        header.num_segs = ToSysOrderS(header.num_segs);
        header.seq_num = ToSysOrderS(header.seq_num);
        header.size = ToSysOrderS(header.size) * 2;
        header.timeMS = ToSysOrderL(header.timeMS);

        if (header.type == 31 || header.type == 29)
        {
            messageCurrent += header.size + 12;
        }
        else
        {
            messageCurrent += 2432;
        }
    }
    return actualSize;
}

ArchiveII NexradAPI::GetSample(SampleMetaData meta)
{
    std::string url = "https://unidata-nexrad-level2.s3.amazonaws.com/" + meta.key;

    constexpr uint32_t numChunks = 2;
    size_t chunkSize = (meta.size + numChunks - 1) / numChunks;

    std::vector<cpr::Response> chunks(numChunks);
    std::vector<std::thread> workers;

    for (uint32_t i = 0; i < numChunks; i++)
    {
        workers.emplace_back([&, i]
                             {
                auto &session = sessions[i];
                size_t start = i * chunkSize;
                size_t end = std::min((i + 1) * chunkSize - 1, meta.size - 1);
                session.SetUrl(cpr::Url{url});
                session.SetHeader(cpr::Header{{"Range", std::format("bytes={}-{}",start,end)}});
                chunks[i] = std::move(session.Get()); });
    }
    for (auto &t : workers)
        t.join();

    std::string dataText;
    dataText.reserve(meta.size);
    for (uint32_t i = 0; i < numChunks; i++)
    {
        if ((chunks[i].status_code != 200 && chunks[i].status_code != 206) || chunks[i].error)
        {
            std::cerr << "Failed to download archiveII! Status-code: " << chunks[i].status_code << std::endl;
            return {};
        }
        dataText += chunks[i].text;
    }

    ArchiveII archive;

    const char *data = dataText.c_str();

    std::memcpy(&archive.header, data, sizeof(archive.header));

    archive.header.date = ToSysOrderL(archive.header.date);
    archive.header.time = ToSysOrderL(archive.header.time);

    const char *current = data + sizeof(ArchiveIIHeader);

    while (current - data < dataText.length())
    {
        Record &record = archive.records.emplace_back();
        size_t size = ParseRecord(record, current);
        current += size;
    }

    return archive;
}