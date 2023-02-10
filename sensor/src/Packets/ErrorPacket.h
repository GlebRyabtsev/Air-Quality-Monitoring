#ifndef ERRORPACKET_H
#define ERRORPACKET_H

#include "TextPacket.h"

/**
 * Packet used for reporting errors.
 */
class ErrorPacket : public TextPacket
{
public:
    /**
     * Construct from error string. Timestamp is added automatically.
     * @param error
     */
    explicit ErrorPacket(const char* error);

    static constexpr char eventName[] = "error";
};

#endif