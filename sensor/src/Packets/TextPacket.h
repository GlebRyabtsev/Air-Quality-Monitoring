#ifndef TEXTPACKET_H
#define TEXTPACKET_H

#include "Packet.h"

// A TextPacket is used for text messages sent to the Particle cloud
// A text packet is structured as follows:
// [0-3] 32-bit Unix timestamp
// [4-...] Message string
// Since TextPackt is not intended to store accumulated data, it is
// designed to be immutable

/**
 * Packet used for text messages sent to the particle cloud.
 * Structure:
 * Bytes   |Function
 * --------|-------------------
 * 0-3     |Timestamp
 * 4-end   |Message string
 *
 * Since TextPacket is not intended to store accumulated data, it is designed to be immutable.
 */
class TextPacket : public Packet
{
public:
    /**
     * Construct from string and timestamp.
     * @param text Message string
     * @param timestamp Packet timestamp
     * @param eventName Event name to use
     */
    TextPacket(const char* text, time32_t timestamp, const char* eventName);

    ~TextPacket() override;

protected:
private:
};

#endif