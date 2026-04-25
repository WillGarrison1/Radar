#pragma once

#include <map>
#include <condition_variable>
#include <mutex>
#include <functional>
#include <queue>
#include <unordered_set>

#include <TaskQueue.hpp>
#include "NexradAPI.hpp"

struct Gate
{
    int8_t reflectivity;
    int8_t velocity;
    uint16_t gateNum;
};

struct Radial
{
    std::vector<Gate> gates;
    uint16_t gateSize;
    uint16_t firstGate;
    float trueAzimuth;   // actual non-rounded azimuth
    float trueElevation; // actual non-rounded elevation
};

struct RadarScan
{
    /*
        radial = radials[elevation][azimuth];
    */
    float radialWidth;
    std::map<uint16_t, std::vector<Radial>> radials;
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

class StormProcessor
{
public:
    StormProcessor();
    ~StormProcessor();

    void Refresh();
    void Update();

    std::vector<SampleTimePoint> GetTimePoints();
    void Process(SampleTimePoint timestep);

    inline std::map<SampleTimePoint, RadarScan> &GetCached()
    {
        return cache;
    }

    inline std::vector<SampleTimePoint> GetPending()
    {
        std::vector<SampleTimePoint> vec;
        for (uint64_t point : pending)
        {
            SampleTimePoint tp(std::chrono::duration_cast<SampleTimePoint::duration>(std::chrono::nanoseconds{point}));
            vec.push_back(tp);
        }
        return vec;
    }

private:
    RadarScan CreateScan(ArchiveII archive);
    void ApplyMessage(RadarScan &scan, Record &record, Message &message);
    void ProcessDataBlock(Radial &radial, std::vector<Gate> &gates, const uint8_t *block);

    std::mutex cacheMutex;
    std::map<SampleTimePoint, RadarScan> cache;
    std::unordered_set<uint64_t> pending;
    NexradAPI nexradAPI;
    TaskQueue queue;
    std::thread worker;
};