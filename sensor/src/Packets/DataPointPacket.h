#ifndef MDP10PACKET_H
#define MDP10PACKET_H

#include "main.h"
#include "Packet.h"
#include "SdFat.h"

#include "RequestedDataPointPacket.h"

/**
 * Packet used to send data points obtained through regular measurements.
 * 
 * Structure:
 * Bytes   |Function
 * --------|-----------
 * 0-n*34  |data points
 */
class DataPointPacket : public Packet
{
public:
    /**
     * @brief Construct an empty DataPointPacket
     */
    DataPointPacket();

    ~DataPointPacket() override;
    
    /**
     * @brief Convert a data point into the binary format and append it to the payload.
     * 
     * Data point structure:
     * Bytes | Function
     * ------|--------------------------
     * 0-3   | timestamp
     * 4-33  | Values as 3-byte integers
     * 
     * @param dpd Data point
     * @param timestamp Timestamp
     * @return true on success
     */
    bool append(DatapointDouble& dpd, time32_t timestamp);

    /**
     * @brief Check if a further datapoint can be appended.
     * @return true if full
     */
    bool isFull();

    static constexpr char eventName[] = "dp";

    // DataPointPacket needs to fit into RequestedDataPointPacket
    static constexpr size_t MAX_SIZE_BYTES = SystemConfig::PACKET_MAX_SIZE_BYTES - RequestedDataPointPacket::HEADER_SIZE;

    static constexpr time32_t TIMESPAN = MAX_SIZE_BYTES / SystemConfig::DATAPOINT_SIZE *
                                         SystemConfig::SPS30_MEASUREMENT_PERIOD * SystemConfig::N_DATA_POINTS_AVERAGING;

private:
    static constexpr double measurementMultiplier = 100.0 * 32.0;
};

#endif
