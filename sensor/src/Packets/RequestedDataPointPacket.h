#ifndef KIST_SENSOR_ARGON_FW_LOCAL_REQUESTEDDATAPOINTPACKET_H
#define KIST_SENSOR_ARGON_FW_LOCAL_REQUESTEDDATAPOINTPACKET_H

#include "Packet.h"
#include "Packets/HandshakePacket.h"

/**
 * Packet used to send data points requested in a handshake.
 * Structure:
 * Bytes   |Function
 * --------|-------------------
 * 0-3     |Timestamp
 * 4       |Total packets in the response or zero if not known. Must be specified in at least one packet.
 * 5       |Number of this packet.
 * 6-9     |Handshake Timestamp
 * 10-end  |Requested packets in the standard format
 */
class RequestedDataPointPacket : public Packet
{
public:
    /**
     * @brief Construct as a response to a specific handshake packet
     * @param hp Handshake packet to reference in the header of this Requested Data Point Packet
     * @param packetNumber Number of this packet in the response to the handshake packet, starting with 0
     * @param totalPackets Total number of packets in the response
     */
    RequestedDataPointPacket(const HandshakePacket& hp, uint8_t packetNumber, uint8_t totalPackets);

    ~RequestedDataPointPacket() override;

    /**
     * @brief Get the back insert iterator for the payload of the packet.
     * Used to write to the packet without copying.
     * @return std::back_insert_iterator
     */
    std::back_insert_iterator<decltype(data)> getDataBackInserter();

    static constexpr size_t HEADER_SIZE = 10;

    static constexpr char eventName[] = "rdp";
};

#endif // KIST_SENSOR_ARGON_FW_LOCAL_REQUESTEDDATAPOINTPACKET_H
