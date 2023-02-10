#include "Packets/Packet.h"
#include "PacketPublisher.h"
#include "ascii85.h"

PacketPublisher::PacketPublisher(PacketQueue& packetPublishingQueue, const SystemConfig& sysconfig,
                                 const SystemState& sysstate)
    : packetPublishingQueue(packetPublishingQueue), sysstate(sysstate), sysconfig(sysconfig)
{
}

void PacketPublisher::start()
{
    thread = Thread{"PacketPublisher", [this] { run(); }};
}

[[noreturn]] void PacketPublisher::run()
{
    Packet packet;
    while (true)
    {
        packetPublishingQueue.take(&packet, CONCURRENT_WAIT_FOREVER);
        if(Time.now() - sysstate.lastHandshakeTimestamp < sysconfig.HANDSHAKE_MAX_PERIOD) {
            uint16_t dataSize;
            const uint8_t* data = packet.getBytes(&dataSize);
            const size_t PACKET_MAX_SIZE_UTF8 = SystemConfig::PACKET_MAX_SIZE_BYTES * 5 / 4;
            uint8_t encodedData[PACKET_MAX_SIZE_UTF8];
            auto encodedLength = encode_ascii85(data, dataSize,
                                                encodedData, PACKET_MAX_SIZE_UTF8);
            String encodedDataString(reinterpret_cast<char*>(encodedData), encodedLength);

            Particle.publish(packet.getEventNameString(), encodedDataString,
                             NO_ACK); // can block for as long as 10 minutes
        } else {
            Log.warn("PacketPublisher: Packet dropped because of handshake timeout.");
        }

        delay(1000); // prevents internal buffer overflow if sending a large number of packets in rapid succession
    }
}