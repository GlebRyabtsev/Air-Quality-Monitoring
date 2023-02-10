//
// Created by Gleb Ryabtsev on 06.03.22.
//

#include "main.h"
#include "HandshakePacket.h"
#include "ascii85.h"

constexpr char HandshakePacket::eventName[];

HandshakePacket::HandshakePacket(const char* dataEncoded) : Packet(eventName)
{
    size_t inLength = std::strlen(dataEncoded);
    data.resize(ascii85MaxDecodedLength(inLength));
    size_t realLength = decode_ascii85(reinterpret_cast<const uint8_t*>(dataEncoded), 
                                       inLength, data.begin().get_ptr(), 
                                       1000000);  // dirty hack
    data.resize(realLength);
}

HandshakePacket::HandshakePacket() = default;