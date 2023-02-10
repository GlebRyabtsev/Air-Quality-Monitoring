//
// Created by Gleb Ryabtsev on 06.03.22.
//

#ifndef KIST_SENSOR_ARGON_FW_LOCAL_HANDSHAKEPACKET_H
#define KIST_SENSOR_ARGON_FW_LOCAL_HANDSHAKEPACKET_H

#include "Packet.h"

/**
 * Packet used to represent received handshakes.
 * Structure:
 * Bytes | Function
 * ------|---------------------------------
 * 0-3   | timestmap
 * 4-end | intervals (8 bytes per interval)
 */
class HandshakePacket : public Packet
{
public:

    static constexpr char eventName[] = "hs";
    static constexpr uint16_t MAX_INTERVALS = (SystemConfig::PACKET_MAX_SIZE_BYTES - 4) / (2 * 4);

    /**
     * Construct from an ascii85-encoded string.
     * @param data character string
     */
    explicit HandshakePacket(const char* data);

    /**
     * Construct an empty HandshakePacket
     */
    HandshakePacket();

    /**
     * Get intervals from the packet
     * 
     * @tparam Container Container of value type interval_t
     * @param outIt Back insert iterator to a container of 
     *              interval_t elements, where to write the intervals
     */
    template<class Container>
    void getIntervals(std::back_insert_iterator<Container> outIt);

};

template<class Container>
void HandshakePacket::getIntervals(std::back_insert_iterator<Container> outIntervalIt) {
    for(auto dataIt = data.begin() + 4; dataIt <= data.end() - 8; dataIt += 8) {
        auto start = *reinterpret_cast<time32_t*>(dataIt.get_ptr());
        auto end = *reinterpret_cast<time32_t*>(dataIt.get_ptr() + 4);
        outIntervalIt = {start, end};
    }
}

#endif // KIST_SENSOR_ARGON_FW_LOCAL_HANDSHAKEPACKET_H
