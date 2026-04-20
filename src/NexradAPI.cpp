#include "NexradAPI.hpp"
#include <iostream>

#include <bxzstr.hpp>
#include <chrono>
#include <cpr/cpr.h>
#include <fstream>
#include <ranges>
#include <sstream>
#include <string>
#include <tinyxml2.h>

static std::string radarName = "KLSX";

NexradAPI::NexradAPI()
{
    Update();
}

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

};

struct ArchiveII
{
    ArchiveIIHeader header;
    CompressedRecord metadataRecord;
};

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

    std::cout << dateString << std::endl;
    std::string url = "https://unidata-nexrad-level2.s3.amazonaws.com/?prefix=" + dateString + "/" + radarName + "/";
    std::cout << url << std::endl;
    auto result = cpr::Get(cpr::Url{url});
    std::cout << "Status code: " << result.status_code << std::endl;

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
        radarSamplesMeta.push_back(ParseSampleMeta(radarSampleXML));
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
    std::tm tm{};

    int firstUnderscore = sample.key.find("_");
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
                  std::chrono::month{tm.tm_mon + 1} /
                  std::chrono::day{tm.tm_mday}} +
              std::chrono::hours{tm.tm_hour} + std::chrono::minutes{tm.tm_min} + std::chrono::seconds{tm.tm_sec};
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
        tp.time_since_epoch());

    auto tp_seconds =
        std::chrono::time_point_cast<std::chrono::seconds>(tp);
    sample.time = tp_seconds;

    return sample;
}

std::vector<SampleMetaData> &NexradAPI::ListSamples()
{
    return radarSamplesMeta;
}

std::string NexradAPI::GetSample(SampleMetaData meta)
{
    static std::string url = "https://unidata-nexrad-level2.s3.amazonaws.com/" + meta.key;
    auto result = cpr::Get(cpr::Url{url});
    if (result.status_code != 200)
    {
        throw std::runtime_error("Failed to get file!");
    }

    std::istringstream ss(result.text);
    bxz::istream decompressed(ss.rdbuf(), bxz::z);

    std::string decompressedData;
    decompressed >> decompressedData;

    std::ofstream outFile("out");
    outFile << decompressedData;
    outFile.close();

    return decompressedData;
}