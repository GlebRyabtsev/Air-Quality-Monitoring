#include "TextPacket.h"

TextPacket::TextPacket(const char* text, time32_t timestamp, const char* eventName)
    : Packet(eventName)
{
    auto bytesPtr = reinterpret_cast<const uint8_t*>(&timestamp);
    data.insert(data.end(), bytesPtr, bytesPtr + sizeof(timestamp));

    size_t textLength = std::strlen(text);
    const uint8_t* textBytes = reinterpret_cast<const uint8_t*>(text);
    data.insert(data.end(), textBytes, textBytes + textLength);
}

TextPacket::~TextPacket() = default;