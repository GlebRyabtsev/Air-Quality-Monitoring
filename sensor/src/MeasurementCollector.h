#ifndef MEASUREMENTCOLLECTOR_H
#define MEASUREMENTCOLLECTOR_H

#include "PacketQueue.h"
#include "Packets/DataPointPacket.h"
#include <SPS30.h>
#include <main.h>

#include <queue>

class MeasurementCollector
{
public:

    /**
     * Construct a new Measurement Collector object with the provided references to the shared resources.
     * Warning: Measurement Collector should not be constructed as a global variable.
     *
     * @param packetPublishingQueue Packet Publishing Queue
     * @param packetStorageQueue Packet Storage Queue
     * @param sysconfig System Configuration
     * @param sysstate System State
     */
    MeasurementCollector(PacketQueue& packetPublishingQueue, PacketQueue& packetStorageQueue,
                         const SystemConfig& sysconfig, SystemState& sysstate);

    /**
     * Start the measurement collector thread
     */
    void start();

private:
    /**
     * The run function of the thread. Starts the SPS30 sensors, then acquires measurements in intervals
     * specified in the System Config by calling recordMeasurement(). Current Packet is filled with data points,
     * then pushCurrentPacket() is invoked.
     *
     * The function also handles data points acquired when the system time is not set.
     */
    [[noreturn]] void run();

    /**
     * Measurements are pushed into the Averaging Vector, and when it is full, the computeAverage() function is called.
     */
    void recordMeasurement();

    /**
     * Pushes the Current Packet into the Packet Storage Queue and Packet Publishing Queue, and resets it.
     */
    void pushCurrentPacket();

    /**
     * Computes the average from the Averaging Vector.
     * @return Data point with averaged values
     */
    DatapointDouble computeAverage();

    Thread thread;

    std::vector<DatapointDouble> averagingVector{};
    DataPointPacket currentPacket{};
    SPS30 sensor1{sysconfig.SPS30_SDA_1, sysconfig.SPS30_SCL_1};
    SPS30 sensor2{sysconfig.SPS30_SDA_2, sysconfig.SPS30_SCL_2};

    // References to the shared resources
    PacketQueue& packetPublishingQueue;
    PacketQueue& packetStorageQueue;
    const SystemConfig& sysconfig;
    SystemState& sysstate;
};

#endif
