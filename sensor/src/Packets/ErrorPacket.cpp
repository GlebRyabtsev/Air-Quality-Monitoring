//
// Created by Gleb Ryabtsev on 07.03.22.
//


#include "ErrorPacket.h"

constexpr char ErrorPacket::eventName[];

ErrorPacket::ErrorPacket(const char* error) : TextPacket(error, Time.now(), eventName)
{
}