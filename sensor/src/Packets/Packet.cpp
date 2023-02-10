#include "Packet.h"
#include "application.h"

#include <cmath>

Packet::Packet(const char* eventName)
{
    assert(eventName != nullptr);
    std::strcpy(this->eventNamePrivate, eventName);
}

Packet::Packet() = default;

Packet::~Packet() = default;

const uint8_t* Packet::getBytes(uint16_t* l) const
{
    *l = data.size();
    return data.begin().get_ptr();
}

time32_t Packet::getTimestamp() const
{
    assert(data.size() >= 4);
    return *reinterpret_cast<const time32_t*>(&data[0]);
}

String Packet::getEventNameString() const
{
    return {eventNamePrivate};
}

const char* Packet::getEventName() const
{
    return eventNamePrivate;
}

f_string Packet::makeFilename() const {
    char name[32];
    std::snprintf(name, sizeof(name), "%d.pkt", getTimestamp());
    return {name};
}

void Packet::reset()
{
    data.clear();
}
