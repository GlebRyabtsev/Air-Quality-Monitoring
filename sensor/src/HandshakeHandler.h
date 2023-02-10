#ifndef HANDSHAKEHANDLER_H
#define HANDSHAKEHANDLER_H

#include "main.h"
#include "PacketStorageManager.h"
#include "PacketQueue.h"
#include "packets/HandshakePacket.h"

class HandshakeHandler
{
public:
    HandshakeHandler(PacketStorageManager &psm,  PacketQueue &packetPublishingQueue, 
                     SystemState &sysstate,  ErrorHandler &eh);

    void start();

    bool putHandshake(const char* encodedData);

private:
    [[noreturn]] void run();

    Thread thread;

    os_mutex_t handshakeMutex{};
    HandshakePacket handshake;
    bool handshakeAvailable = false;

    // Shared resources
    PacketStorageManager &psm;
    PacketQueue &packetPublishingQueue;
    SystemState &sysstate;
    ErrorHandler &eh;
};

#endif