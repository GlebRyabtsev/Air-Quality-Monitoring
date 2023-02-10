#ifndef PACKETPUBLISHINGTHREAD_H
#define PACKETPUBLISHINGTHREAD_H

#include "PacketQueue.h"
#include "main.h"

class PacketPublisher
{
public:
    /**
     * Construct a new Packet Publisher object with the provided references to the shared resources.
     * Warning: Packet Publisher should not be constructed as a global variable.
     *
     * @param packetPublishingQueue Packet Publishing Queue
     * @param sysconfig System Config
     * @param sysstate System State
     */
    PacketPublisher(PacketQueue& packetPublishingQueue, const SystemConfig& sysconfig,
                    const SystemState& sysstate);

    void start();

private:
    /**
     * Run function of the Packet Publisher thread.
     *
     * Grabs new packets from Packet Publishing Queue, and publishes them as events to the Particle cloud.
     * Drops packets if handshake timeout has occurred.
     */
    [[noreturn]] void run();

    Thread thread;

    //  SHARED RESOURCES
    PacketQueue& packetPublishingQueue;
    const SystemState& sysstate;
    const SystemConfig& sysconfig;
};

#endif