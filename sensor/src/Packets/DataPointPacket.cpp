#include "DataPointPacket.h"

constexpr char DataPointPacket::eventName[];

DataPointPacket::DataPointPacket() : Packet(eventName) {}

DataPointPacket::~DataPointPacket() = default;

bool DataPointPacket::append(DatapointDouble& dpd, time32_t timestamp)
{
    if (isFull()) return false;

    const uint8_t* bytesPtr;
    bytesPtr = reinterpret_cast<const uint8_t*>(&timestamp);
    data.insert(data.end(), bytesPtr, bytesPtr+sizeof(timestamp));

    for (const float valF : dpd) {
        uint32_t valI = measurementMultiplier * valF + 0.5;
        bytesPtr = reinterpret_cast<const uint8_t*>(&valI);
        data.insert(data.end(), bytesPtr, bytesPtr+3);
    }
    return true;
}

bool DataPointPacket::isFull()
{
    return (MAX_SIZE_BYTES - data.size()) < SystemConfig::DATAPOINT_SIZE;
}

