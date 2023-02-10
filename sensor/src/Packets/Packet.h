#ifndef PACKET_H
#define PACKET_H

#include "main.h"

/**
 * Abstract packet to be sent via Particle Cloud Events.
 * Provides common functions used by PacketPublisher and PacketStorageManager.
 */
class Packet
{
public:
    /**
     * @brief Construct a new Packet object
     * 
     * @param eventName event name to be used
     */
    Packet(const char* eventName);

    /**
     * @brief Construct a new Packet object without specifying the event name
     * 
     */
    Packet();

    virtual ~Packet();

    /**
     * Get binary data.
     * @return Packet in binary form
     */
    const uint8_t* getBytes(uint16_t* l) const;

    /**
     * Discard the data and make the object behave as new.
     */
    virtual void reset();

    /**
     * Get timestamp.
     * @return Unix timestamp
     */
    virtual time32_t getTimestamp() const;

    String getEventNameString() const;  // todo: delete

    /**
     * Get the name of the Particle Cloud Event which should be used to send the packet.
     * @return Event name
     */
    const char* getEventName() const;

    static_string<64> makeFilename() const;

protected:
    // not encoded (binary) data of the packet
    static_vector<uint8_t, SystemConfig::PACKET_MAX_SIZE_BYTES> data{};
private:
    // we need to store event name in every packet object, because we slice DataPointPacket,
    // ErrorPacket, etc. into the Packet base class when we push it into packetPublishingQueue
    // and packetStorageQueue.
    char eventNamePrivate[10] = "";

};

#endif
