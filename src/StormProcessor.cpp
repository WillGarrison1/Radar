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

// Message 31 is the message with radial data
#pragma pack(1)
struct Message31Header
{
    char radar_id[4];
    uint32_t collection_time;
    uint16_t date;
    uint16_t azimuth_num;
    float azimuth_angle;
    uint8_t compression;
    uint8_t spare;
    uint16_t radial_length;     // length of uncompressed radial in bytes
    uint8_t azimuth_resolution; // 1 = 0.5 degrees 2 = 1 degrees
    uint8_t radial_status;
    uint8_t elevation_num;
    uint8_t cut_sector;
    float elevation_angle;
    uint8_t radial_blanking;
    uint8_t azimuth_mode;
    uint16_t block_count;
};

struct MomentDataBlock
{
    char block_type; // always 'D'
    char name[3];    // "REF", "VEL", "SW ", "ZDR", "PHI", "RHO"
    uint32_t reserved;
    uint16_t num_gates;
    uint16_t range_to_first;
    uint16_t gate_size;
    uint16_t tover;
    uint16_t snr_threshold;
    uint8_t flags;
    uint8_t word_size; // 8 or 16
    float scale;
    float offset;
    // gate data follows immediately
};
#pragma pack()

StormProcessor::StormProcessor() : pending({}), queue({}), worker([&]
                                                                  { this->queue.WorkerLoop(); })
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
    this->queue.Stop();
    this->worker.join();
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

void StormProcessor::Process(SampleTimePoint timePoint)
{
    if (this->pending.contains(timePoint.time_since_epoch().count()))
    {
        return; // already being processed or downloaded
    }

    if (nexradAPI.ListSamples().empty())
    {
        std::cerr << "No samples!" << std::endl;
        return;
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
        return;
    }

    pending.emplace(timePoint.time_since_epoch().count());
    queue.Push([this, meta]()
               {   
                auto start = std::chrono::steady_clock::now();
                auto archive = nexradAPI.GetSample(meta);
                auto end = std::chrono::steady_clock::now();
                std::cout << "Download took: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
                start = std::chrono::steady_clock::now();
                auto result = _Process(std::move(archive)); 
                end = std::chrono::steady_clock::now();
                std::cout << "\nProcess took: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
                std::lock_guard lock(this->cacheMutex);
                this->cache[meta.time] = result;
                this->pending.erase(meta.time.time_since_epoch().count()); });
    return;
}

VolumeScan StormProcessor::_Process(ArchiveII archive)
{
    VolumeScan scan;
    for (auto &record : archive.records)
        for (auto &message : record.messages)
        {
            if (message.header.type != 31)
                continue;
            auto data = std::span(record.data).subspan(message.index, message.header.size - sizeof(MessageHeader));
            Message31Header header;
            std::memcpy(&header, data.data(), sizeof(header));
            header.azimuth_angle = ToSysOrderF(header.azimuth_angle);
            header.elevation_angle = ToSysOrderF(header.elevation_angle);
            header.block_count = ToSysOrderS(header.block_count);
            uint16_t elevationI = static_cast<uint16_t>(std::round(header.elevation_angle * 2)) * 5;
            auto &radial = scan.radials[elevationI].emplace_back();

            if (header.compression != 0)
            {
                std::cout << "No support for compressed messages yet!" << std::endl;
                continue;
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
                radial.trueAzimuth = header.azimuth_angle;
                radial.trueElevation = header.elevation_angle;

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
                    continue; // no support for other data types than velocity and reflectivity
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
            // gates now contains e.g. dBZ values for "REF", m/s for "VEL", etc.
            for (uint16_t i = 0; i < gates.size(); i++)
            {
                if (gates[i].reflectivity > 0) // filter gates to positive reflectivity
                    radial.gates[i] = gates[i];
            }
        }
    return scan;
}