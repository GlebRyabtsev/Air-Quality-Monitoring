#include "Packets/DataPointPacket.h"
#include <MeasurementCollector.h>

MeasurementCollector::MeasurementCollector(PacketQueue& packetPublishingQueue,
                                           PacketQueue& packetStorageQueue,
                                           const SystemConfig& sysconfig,
                                           SystemState& sysstate)
    : packetPublishingQueue(packetPublishingQueue), packetStorageQueue(packetStorageQueue),
      sysconfig(sysconfig), sysstate(sysstate)
{
}

void MeasurementCollector::start()
{
    thread = Thread{"MeasurementCollector", [this] { run(); }};
}

[[noreturn]] void MeasurementCollector::run()
{
    std::queue<DatapointDouble> timestamplessDataPoints{};

    // Init SPS30s
    sensor1.startMeasurement();
    sensor2.startMeasurement();

    while (true)
    {
        recordMeasurement();
        if (averagingVector.size() == sysconfig.N_DATA_POINTS_AVERAGING)
        {
            // We have collected enough data points to calculate the average
            auto avg = computeAverage();
            averagingVector.clear();

            if (Time.year() == 2000 || Particle.syncTimePending())
            {
                // Oh-oh: we don't know what time it is and can't stamp the
                // packet, so it goes into the queue of timestamp-less packets
                timestamplessDataPoints.push(avg);
            }
            else
            {
                // System time is correct, so we push the timestampless
                // datapoints first in FIFO order before pushing the current
                // datapoint (avg). We deduce timestamps based on position in the
                // queue. Beforehand, we set the lastHandshakeTimestamp variable to 
                // current time, so that the PacketPublisher can start sending packets
                // before the first handshake arrives from the server.
                sysstate.lastHandshakeTimestamp = Time.now();
                while (!timestamplessDataPoints.empty())
                {
                    auto dp = timestamplessDataPoints.front();
                    time32_t t = Time.now() - timestamplessDataPoints.size() *
                                                  sysconfig.SPS30_MEASUREMENT_PERIOD *
                                                  sysconfig.N_DATA_POINTS_AVERAGING;
                    currentPacket.append(dp, t);
                    timestamplessDataPoints.pop();
                    if (currentPacket.isFull())
                        pushCurrentPacket();
                }
                currentPacket.append(avg, Time.now());
                if (currentPacket.isFull())
                    pushCurrentPacket();
            }
        }

        delay(sysconfig.SPS30_MEASUREMENT_PERIOD * 1000 - 50);
    }
}

void MeasurementCollector::recordMeasurement()
{
    SPS30MeasuredValues val1, val2;
    while (!(sensor1.readDataReadyFlag() && sensor2.readDataReadyFlag()))
        delay(2);
    sensor1.readMeasuredValues(&val1);
    sensor2.readMeasuredValues(&val2);
    DatapointDouble mes{val1.NumberConcentration.pm005, val1.NumberConcentration.pm010,
                        val1.NumberConcentration.pm025, val1.NumberConcentration.pm040,
                        val1.NumberConcentration.pm100, val2.NumberConcentration.pm005,
                        val2.NumberConcentration.pm010, val2.NumberConcentration.pm025,
                        val2.NumberConcentration.pm040, val2.NumberConcentration.pm100};
    averagingVector.push_back(mes);
}

void MeasurementCollector::pushCurrentPacket()
{
    // Push current packet into the packetPublishingQueue and packetStorageQueue
    packetPublishingQueue.push(currentPacket);
    packetStorageQueue.push(currentPacket);
    currentPacket.reset();
}

DatapointDouble MeasurementCollector::computeAverage()
{
    // average all values in the averagingVector and return the result
    DatapointDouble avg;
    for (const DatapointDouble& dp : averagingVector)
    {
        for (uint8_t i = 0; i < dp.size(); i++)
        {
            avg[i] += dp[i];
        }
    }
    for (double& val : avg)
    {
        val /= sysconfig.N_DATA_POINTS_AVERAGING;
    }
    return avg;
}