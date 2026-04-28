#include "StormProcessor.hpp"

#include <cmath>
#include <functional>
#include <iostream>
#include <ranges>
#include <span>
#include <thread>

enum class GateType
{
    None,
    Reflectivity,
    Velocity
};

StormProcessor::StormProcessor()
{
    float seconds = 0.5;
    while (nexradAPI.ListSamples().size() == 0 && seconds < 30)
    {
        std::cout << "Waiting for radar archiveII records" << std::endl;
        nexradAPI.Update();
        std::this_thread::sleep_for(std::chrono::seconds(static_cast<uint32_t>(seconds)));
        seconds *= 1.5;
    }
}

StormProcessor::~StormProcessor()
{
}

void StormProcessor::Refresh()
{
    nexradAPI.Update();
}

void StormProcessor::Update()
{
}

std::vector<SampleTimePoint> StormProcessor::GetTimePoints()
{
    std::vector<SampleTimePoint> list;
    for (auto &sample : nexradAPI.ListSamples())
    {
        list.push_back(sample.time);
    }

    return list;
}

void StormProcessor::ProcessDataBlock(Radial &radial, std::vector<Gate> &gates, const uint8_t *block)
{
    MomentDataBlock moment;
    std::memcpy(&moment, block, sizeof(moment));
    moment.num_gates = ToSysOrderS(moment.num_gates);
    moment.range_to_first = ToSysOrderS(moment.range_to_first);
    moment.gate_size = ToSysOrderS(moment.gate_size);
    moment.word_size = moment.word_size;
    moment.scale = ToSysOrderF(moment.scale);
    moment.offset = ToSysOrderF(moment.offset);

    radial.firstGate = moment.range_to_first;
    radial.gateSize = moment.gate_size;

    if (moment.num_gates > gates.size())
    {
        gates.resize(moment.num_gates);
    }

    const uint8_t *gateData = block + sizeof(MomentDataBlock);

    std::function<int8_t &(Gate &)> func;
    if (std::string(moment.name, 3) == "REF")
    {
        func =
            [](Gate &g) -> int8_t &
        { return g.reflectivity; };
    }
    else if (std::string(moment.name, 3) == "VEL")
    {
        func =
            [](Gate &g) -> int8_t &
        { return g.velocity; };
    }
    else
    {
        return; // no support for other data types than velocity and reflectivity
    }

    auto gate = gates | std::views::transform(func);

    for (int i = 0; i < moment.num_gates; i++)
    {
        uint16_t raw;
        if (moment.word_size == 8)
        {
            raw = gateData[i];
        }
        else
        {
            raw = ToSysOrderS(reinterpret_cast<const uint16_t *>(gateData)[i]);
        }

        if (raw <= 1)
        {
            gate[i] = std::numeric_limits<float>::quiet_NaN(); // below threshold or range folded
            continue;
        }
        gate[i] = (raw - moment.offset) / moment.scale;
    }
}

void StormProcessor::ApplyMessage(RadarScan &scan, Record &record, Message &message)
{
    if (message.header.type != 31)
        return;

    auto data = std::span(record.data).subspan(message.index, message.header.size - sizeof(MessageHeader));

    Message31Header header;
    std::memcpy(&header, data.data(), sizeof(header));
    header.azimuth_angle = ToSysOrderF(header.azimuth_angle);
    header.elevation_angle = ToSysOrderF(header.elevation_angle);
    header.block_count = ToSysOrderS(header.block_count);

    uint16_t elevationI = static_cast<uint16_t>(std::round(header.elevation_angle * 2)) * 5;

    auto &radial = scan.radials[elevationI].emplace_back();
    radial.trueAzimuth = header.azimuth_angle;
    radial.trueElevation = header.elevation_angle;

    scan.radialWidth = header.azimuth_resolution == 1 ? 0.5 : 1;

    if (header.compression != 0)
    {
        std::cout << "No support for compressed messages yet!" << std::endl;
        return;
    }

    // block offsets array sits immediately after the header
    const uint32_t *blockOffsets = reinterpret_cast<const uint32_t *>(data.data() + sizeof(Message31Header));
    std::vector<Gate> gates;
    for (int i = 0; i < header.block_count; i++)
    {
        uint32_t offset = ToSysOrderL(blockOffsets[i]);
        const uint8_t *block = data.data() + offset;

        if (block[0] != 'D')
            continue; // skip non-data blocks (volume, elev, radial)

        ProcessDataBlock(radial, gates, block);
    }

    for (uint16_t i = 0; i < gates.size(); i++)
    {
        if (gates[i].reflectivity > 10) // filter gates to reflectivity threshold
        {
            radial.gates.emplace_back(gates[i].reflectivity, gates[i].velocity, i);
        }
    }
}

RadarScan StormProcessor::CreateScan(SampleTimePoint timePoint)
{

    if (nexradAPI.ListSamples().empty())
    {
        std::cerr << "No samples!" << std::endl;
        return {};
    }

    SampleMetaData meta{.key = ""};
    for (auto &sampleMeta : nexradAPI.ListSamples())
    {
        if (sampleMeta.time == timePoint)
        {
            meta = sampleMeta;
            break;
        }
    }
    if (meta.key.empty())
    {
        std::cout << "Could not find sample metadata" << std::endl;
        return {};
    }

    auto archive = nexradAPI.GetSample(meta);

    RadarScan scan;
    for (auto &record : archive.records)
    {
        for (auto &message : record.messages)
        {
            ApplyMessage(scan, record, message);
        }
    }
    return scan;
}