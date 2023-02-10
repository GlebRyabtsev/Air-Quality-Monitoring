#include "HandshakeHandler.h"

#include "packets/RequestedDataPointPacket.h"

HandshakeHandler::HandshakeHandler(PacketStorageManager &psm, PacketQueue &packetPublishingQueue, 
                                    SystemState &sysstate, ErrorHandler &eh) 
    : psm(psm), packetPublishingQueue(packetPublishingQueue), sysstate(sysstate),  eh(eh)
{
}

void HandshakeHandler::start()
{
    os_mutex_create(&handshakeMutex);
    thread = Thread("HandshakeHandler", [this] { run(); });
}

bool HandshakeHandler::putHandshake(const char* encodedData)
{
    if(handshakeAvailable) {
        Log.warn("Received handshake while processing another one, returning error");
        return false;
    }
    os_mutex_lock(handshakeMutex);
    handshake = HandshakePacket(encodedData);
    os_mutex_unlock(handshakeMutex);
    Log.info("Received handshake with timestmap %d", handshake.getTimestamp());
    handshakeAvailable = true;
    return true;
}

[[noreturn]] void HandshakeHandler::run() 
{
    while(true) {
        while(!handshakeAvailable) {
            delay(100);
        }
        os_mutex_lock(handshakeMutex);
        static_vector<interval_t, HandshakePacket::MAX_INTERVALS> intervals{};
        handshake.getIntervals(std::back_inserter(intervals));
        Log.info("Received handshake with %d intervals, timestamp %d.", intervals.size(), handshake.getTimestamp());
        static_vector<PacketStorageManager::PacketDescriptor, SystemConfig::MAX_REQUESTED_PACKETS_PER_HANDSHAKE> packets{};
        psm.findPackets(intervals, packets);
        Log.info("Filled vector, size %d", packets.size());
        os_mutex_unlock(handshakeMutex);
        handshakeAvailable = false;
    }
}