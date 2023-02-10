#ifndef PACKETQUEUE_H
#define PACKETQUEUE_H

#include "Packets/Packet.h"
#include "Particle.h"

#include <queue>

class PacketQueue
{
public:
    void init(size_t size, bool autoEmpty = true);

    bool push(const Packet& packet);
    // returns false if queue is full.

    bool take(Packet* packet, system_tick_t del);

    bool autoEmpty = true;
private:
    os_queue_t queue{};
};

#endif
