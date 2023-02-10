#include "ErrorHandler.h"

#include "Packets/ErrorPacket.h"

ErrorHandler::ErrorHandler(PacketQueue& packetPublishingQueue, PacketQueue& packetStorageQueue,
                           const SystemConfig& sysconfig, SystemState& sysstate)
    : publishWaitingPacketsTimer(sysconfig.ERROR_MESSAGES_SEND_ATTEMPT_PERIOD*1000,
                                 [this] { publishWaitingPackets(); }),
      packetPublishingQueue(packetPublishingQueue), packetStorageQueue(packetStorageQueue),
      sysstate(sysstate), sysconfig(sysconfig)
{
}

void ErrorHandler::sdError()
{
    if (sysstate.sdActive)
    {
        sysstate.sdActive = false;
        sendErrorMessages("SD card error.");
    }
}

void ErrorHandler::flashError()
{
    if (sysstate.flashActive)
    {
        sysstate.flashActive = false;
        sendErrorMessages("Flash error.");
    }
}

void ErrorHandler::sensorError(uint8_t sensor)
{
    switch (sensor)
    {
    case 0:
        if (!sysstate.sensor1Error)
        {
            sysstate.sensor1Error = true;
            sendErrorMessages("Sensor 1 error");
        }
        break;
    case 1:
        if (!sysstate.sensor2Error)
        {
            sysstate.sensor2Error = true;
            sendErrorMessages("Sensor 2 error");
        }
        break;
    default:
        break;
    }
}

void ErrorHandler::sendErrorMessages(const std::string& msg)
{
    if (sysstate.serialLogEnabled)
        Log.error(msg.c_str());

    ErrorPacket ep(msg.c_str());
    if (sysstate.cloudReportingEnabled)
    {
        if (Particle.disconnected())
        {
            waitingPackets.push(ep);
        }
        else
        {
            packetPublishingQueue.push(ep);
        }
    }
}

void ErrorHandler::publishWaitingPackets()
{
    if(Particle.disconnected()) return;
    Packet packet;
    bool s = waitingPackets.take(&packet, 0);
    while (s)
    {
        packetPublishingQueue.push(packet);
        s = waitingPackets.take(&packet, 0);
    }
}

void ErrorHandler::init()
{
    waitingPackets.init(sysconfig.PACKET_QUEUE_CAPACITY);
    publishWaitingPacketsTimer.start();
}
