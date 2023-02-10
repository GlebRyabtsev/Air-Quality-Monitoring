#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#undef max // arduino min, max, abs conflict with ones in std lib
#undef min
#undef abs

#include <array>
#include <vector>

#include <boost/container/static_vector.hpp>

#define BOOST_STATIC_STRING_THROW(ex) { Log.error("Static string error"); delay(500); System.reset();}
#define BOOST_STATIC_STRING_STANDALONE
#include <boost/static_string/static_string.hpp>

#include "SdFat.h"

#define LOG_W(s) Log.info(s); delay(200);

// Typedefs
typedef std::array<double, 10> DatapointDouble;    // for real values
typedef std::array<uint32_t, 10> DatapointInteger; // for integer representation
typedef std::pair<time32_t, time32_t> interval_t;  // for time intervals
template<typename T, size_t capacity>
using static_vector = boost::container::static_vector<T, capacity>;
template<size_t length>
using static_string = boost::static_string<length>;
typedef static_string<64> f_string;  // to store filenames

/**
 * Get the max decoded length of an ascii85-encoded byte array
 * @param encodedLength Length of the encoded data
 * @return Maximum length of the decoded data
 */
constexpr size_t ascii85MaxDecodedLength(size_t encodedLength) {  // todo: use everywhere
    return encodedLength / 5 * 4 + 3;
}

/**
 * Calculate the maximum length of ascii85-encoded data
 * @param decodedLength Length of the decodedbyte array
 * @return Maximum length of the resulting ascii string
 */
constexpr size_t ascii85MaxEncodedLength(size_t decodedLength) {
    return  ((decodedLength + 3) / 4) * 5;
}

/**
 * System Config contains system parameters that are set at compile-time
 */
struct SystemConfig
{
    // MEASURING AND STORING
    // max capacity of packetPublishingQueue and packetStorageQueue
    static constexpr uint8_t PACKET_QUEUE_CAPACITY = 10;       
    // maximum number of packets stored in the flash
    static constexpr uint16_t FLASH_MAX_PACKETS = 1024;
    // how many measurement data points are used to compute one average
    static constexpr uint16_t N_DATA_POINTS_AVERAGING = 2;
    // period (s) with which measurements are read from the sensors
    static constexpr uint16_t SPS30_MEASUREMENT_PERIOD = 1;
    // max time between two handshakes. If this time is exceeded, the system will stop publishing packets until
    // the next handshake arrives.
    static constexpr time32_t HANDSHAKE_MAX_PERIOD = 100*3600;
    // how often new sub-folders are created on the sd-card
    static constexpr time32_t SD_CARD_SUBFOLDER_TIMESPAN = 3600;
    // SPS30 COMMUNICATION
    static constexpr uint8_t SPS30_SDA_1 = SDA;
    static constexpr uint8_t SPS30_SCL_1 = SCL;
    static constexpr uint8_t SPS30_SDA_2 = D2;
    static constexpr uint8_t SPS30_SCL_2 = D3;
    
    // how often error messages are sent
    static constexpr uint16_t ERROR_MESSAGES_SEND_ATTEMPT_PERIOD = 1*60;

    // Device ID of this board
    std::string deviceId;

    // how many bytes one datapoint takes
    static constexpr int DATAPOINT_SIZE = 10 * 3 + 4;

    // Maximum size of one outgoing packet before encoding
    static constexpr int PACKET_MAX_SIZE_BYTES = 150 / 5 * 4;

    // Maximum number of requested packets that can be sent in response to one handshake (don't increase!)
    static constexpr uint8_t MAX_REQUESTED_PACKETS_PER_HANDSHAKE = 250;
};

/**
 * System state stores the configuration variables that may change at runtime
 */
struct SystemState
{
    // Local storage configuration. Flash or SD functionality is disabled by ErrorHandler if an
    // error occurs
    bool sdActive = true, flashActive = true;

    // Error reporting configuration
    bool serialLogEnabled = false, cloudReportingEnabled = true;

    // SPS30 error flags. Set true when error occurs.
    bool sensor1Error = false, sensor2Error = false;

    // Determines if the sensor should be disabled if an error occurs
    bool disableSPS30OnError = false;

    time32_t lastHandshakeTimestamp = 0;
};

#endif
