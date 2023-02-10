#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#include "main.h"

#include "Packets/ErrorPacket.h"
#include "Packets/Packet.h"
#include "PacketQueue.h"

class ErrorHandler
{
public:

    /**
     * Construct Error Handler object with the provided references to the shared resources.
     * Warning: Error Handler should not be constructed as a global variable.
     *
     * @param packetPublishingQueue Packet Publishing Queue
     * @param packetStorageQueue Packet Storage Queue
     * @param sysconfig System Configuration
     * @param sysstate System State
     */
    ErrorHandler(PacketQueue& packetPublishingQueue, PacketQueue& packetStorageQueue,
                 const SystemConfig& sysconfig, SystemState& sysstate);

    /**
     * Initialize error handler
     */
    void init();

    /**
     * Throw an sd card error and disable SD card in sysstate
     */
    void sdError();

    /**
     * Throw a flash error and disable flash in system state
     */
    void flashError();

    /**
     * Throw a sensor error.
     * @param sensor sensor id (0 or 1)
     */
    void sensorError(uint8_t sensor);

private:
    /**
     * Send error messages to the log and (if enabled) publish to the cloud.
     * @param msg Message text
     */
    void sendErrorMessages(const std::string& msg);

    /**
     * Publish error packets that accumulated while there was no network connection.
     */
    void publishWaitingPackets();

    Timer publishWaitingPacketsTimer;

    PacketQueue waitingPackets; // stores error packets until cloud is connected

    os_mutex_t mutex{};

    // References to the shared resources
    PacketQueue &packetPublishingQueue, &packetStorageQueue;

    SystemState& sysstate;

    const SystemConfig& sysconfig;

};

#endif