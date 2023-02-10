//
// Created by Gleb Ryabtsev on 06.03.22.
//

#include "RequestedDataPointPacket.h"

constexpr char RequestedDataPointPacket::eventName[];

RequestedDataPointPacket::RequestedDataPointPacket(const HandshakePacket& hp, uint8_t packetNumber, uint8_t totalPackets)
    : Packet(eventName)
{
    const uint8_t* bytesPtr;

    time32_t timestamp = Time.now();
    bytesPtr = reinterpret_cast<const uint8_t*>(&timestamp);
    data.insert(data.end(), bytesPtr, bytesPtr + sizeof(timestamp));

    data.push_back(totalPackets);

    data.push_back(packetNumber);

    time32_t handshakeTimestamp = hp.getTimestamp();
    bytesPtr = reinterpret_cast<const uint8_t*>(&handshakeTimestamp);
    data.insert(data.end(), bytesPtr, bytesPtr + sizeof(timestamp));
}

RequestedDataPointPacket::~RequestedDataPointPacket()
= default;

std::back_insert_iterator<decltype(RequestedDataPointPacket::data)> RequestedDataPointPacket::getDataBackInserter() {
    return std::back_inserter(data);
}