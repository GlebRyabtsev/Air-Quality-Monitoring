#ifndef PACKETSTORAGETHREAD_H
#define PACKETSTORAGETHREAD_H

#include "main.h"

#include "ErrorHandler.h"
#include "packets/RequestedDataPointPacket.h"
#include "packets/HandshakePacket.h"
#include "packets/DataPointPacket.h"

#include <SdFat.h>
#include <dirent.h>
#include <fcntl.h>

#include <algorithm>
#include <variant>

#define SD_TRY(expr) if (!(expr)) {os_mutex_unlock(storageMutex); return false;}
#define FLASH_ERROR() {os_mutex_unlock(storageMutex); return false;}

#define SOFT_MISO_PIN D11 // todo: move
#define SOFT_MOSI_PIN D12
#define SOFT_SCK_PIN D13
#define SD_CARD_CS_PIN A5
class PacketStorageManager
{
public:

    /**
     * Construct a new Packet Storage Manager object with the provided references to the shared resources.
     * Warning: Packet Storage Manager should not be constructed as a global variable
     *
     * @param packetStorageQueue Packet Storage Queue
     * @param sd Sd file system
     * @param config System Configuration
     * @param sysstate System State
     * @param eh Error Handler
     */
    PacketStorageManager(PacketQueue& packetStorageQueue, const SdFat32& sd,
                         const SystemConfig& config, const SystemState& sysstate,
                         ErrorHandler& eh); // todo: make sd private variable

    /**
     * Initialize the SD card and the flash storage.
     */
    void initStorage();

    /**
     * Start the Packet Storage Manager thread
     */
    void start();

    /**
     * Searches packets in the specified intervals in the storage. Checks SD card first, then looks in
     * the flash if there are any gaps.
     * 
     * Note: this function does access SD card, but not flash. For the flash, search is performed
     * in the flashPacketTimestampsIndex vector.
     * 
     * @tparam Container A container with value type PacketDescriptor supporting std::back_inserter, to which the descriptors
     * of the found packets are written.
     * @tparam s_intervals Size of the static vector with intervals
     * @param intervals Intervals in which to search packets; begin and end are exclusive
     * @param output The output container
     * @return true Success
     * @return false SD card error
     */
    template<class Container, size_t s_intervals>
    bool findPackets(static_vector<interval_t, s_intervals> &intervals, Container& output);

    /**
     * Stores location of the packet in the storage
     */
    struct PacketDescriptor {
        static constexpr time32_t FLASH_LOCATION = -1;
        time32_t location;  // either SD sub-folder timestamp or FLASH_LOCATION
        time32_t packetTimestamp; // timestamp, as in the file name of the packet
    };

    /**
     * Retrieves a Data Point Packet (as raw byte data) from storage
     * @tparam Container type of the output container with values of type (uint8_t), should support std::back_insert_iterator<>
     * @param d Packet descriptor
     * @param outputIt back insert iterator of the output container
     * @return true on success, false on failure
     */
    template<class Container>
    bool getPacket(const PacketDescriptor& d, std::back_insert_iterator<Container> outputIt);

private:
    /**
     * Initialize SD card by trying to establish SPI connection and building sub-folder index.
     * @return true on success, false on failure
     */
    bool initSDCardStorage();

    /**
     * Initialize the flash storage by creating the index of currently stored packets
     * @return
     */
    bool initFlashStorage();

    /**
     * Attempt attempt to save a packet to the appropriate location on the SD card.
     * @param packet The packet to be saved, must be a Data Point Packet
     * @return true on success, false on failure
     */
    bool savePacketToSD(const Packet& packet);

    /**
     * Attempt to save a packet to the flash
     * @param packet The packet to be saved, must be a Data Point Packet
     * @return True on success, false on failure
     */
    bool savePacketToFlash(const Packet& packet);

    /**
     * Search for packets on the SD card
     *
     * @tparam Container Output container with values of type PacketDescriptor, should support std::back_insert_iterator
     * @tparam s_intervals Max size of the intervals static vector
     * @param intervals Static vector containing the time intervals in which to search packets
     * @param outputIt std::back_insert_iterator of the output container
     * @return True on success, false on failure
     */
    template<class Container, size_t s_intervals>
    bool findPacketsOnSDCard(static_vector<interval_t, s_intervals> intervals, 
                             std::back_insert_iterator<Container> outputIt) const;

    /**
     * Search for packets in a specific folder on the SD card
     *
     * @tparam Container Output container with values of type PacketDescriptor, should support std::back_insert_iterator
     * @tparam s_intervals Max size of the intervals static vector
     * @param folderTimestamp Timestamp of the folder in which to perform the search
     * @param intervals Static vector containing the time intervals in which to search packets
     * @param outputIt std::back_insert_iterator of the output container
     * @return True on success, false on failure
     */
    template<class Container, size_t s_intervals>
    bool findPacketsInFolder(time32_t folderTimestamp, 
                                const static_vector<interval_t, s_intervals> &intervals, 
                                std::back_insert_iterator<Container> outputIt) const;

    /**
     * Search for packets in the flash
     *
     * @tparam Container Output container with values of type PacketDescriptor, should support std::back_insert_iterator
     * @param interval Time interval in which to search the packet
     * @param outputIt std::back_insert_iterator of the output container
     * @return Does not return bool because the search is performed in the index vector, no file system operations.
     */
    template<class Container>
    void findPacketsInFlash(interval_t interval, std::back_insert_iterator<Container> outputIt);

    /**
     * Returns true if the packet name follows the naming convention <UNIX timestamp>.pkt
     */
    bool checkPacketName(const f_string& name) const;

    /**
     * Given a sorted container of timestamps, find timestamps that lie within the specified time interval.
     * @tparam It Timestamp iterator
     * @param interval Interval in which to search timestamps (begin and end exclusive)
     * @param begin Begin of the timestamps container
     * @param end End of the timestamps container
     * @return Iterators of the original container pointing to the begin (incl) and end (excl) of the found range.
     * Begin iterator is set to the "end" parameter if nothing was found.
     */
    template<class It>
    std::pair<It, It> findInterval(const interval_t& interval, It begin, It end) const;

    /**
     * Run function of the Packet Storage Manager Thread. Receives packets from the Packet Storage Queue
     * and attempts to save them to the flash and the sd card.
     */
    [[noreturn]] void run();
    os_mutex_t storageMutex{};

    static_vector<time32_t, 2048> flashPacketTimestampsIndex{};  // timestamps of all packets in the flash

    static_vector<time32_t, 1024> subFolderTimestampsIndex{}; // timestamps of sub-folders on the sd card
    // both index vectors are kept sorted to simplify search

    SdFat32 sd;

    SoftSpiDriver<SOFT_MISO_PIN, SOFT_MOSI_PIN, SOFT_SCK_PIN> softSpi;

    Thread thread;

    // SHARED RESOURCES REFERENCES
    PacketQueue& packetStorageQueue;
    const SystemConfig& sysconfig;
    const SystemState& sysstate;
    ErrorHandler& eh;
};


template<class Container, size_t s_intervals>
bool PacketStorageManager::findPackets(static_vector<interval_t, s_intervals> &intervals, 
                                       Container &output)
{
    Serial.printf("Free RAM %d\n", System.freeMemory());
    //Serial.printf("Find packets called, intervals:\n");
    for(const auto &intv : intervals) {
        Serial.printf("[%d, %d]\n", intv.first, intv.second);
    }
    Serial.printf("Output max size: %d, SD active: %d, flash active: %d\n", output.max_size(), 1, sysstate.flashActive);
    bool s = findPacketsOnSDCard(intervals, std::back_inserter(output));
    if(!s) {
        return false;
    }
    if(flashPacketTimestampsIndex.empty() || !sysstate.flashActive) {
        // can't search in flash, so we're done
        return true;
    }
    // sort the output vector and the intervals
    std::sort(output.begin(), output.end(), [](const PacketDescriptor& lhs, const PacketDescriptor& rhs){
        return lhs.packetTimestamp < rhs.packetTimestamp;
    });

    std::sort(intervals.begin(), intervals.end(), [](const interval_t& lhs, const interval_t& rhs) {
        return lhs.first < rhs.first;
    });

    // look for missing data in the flash
    const time32_t gapTreshold = 12; //DataPointPacket::TIMESPAN * 3/2;
    auto pktIt = output.begin();
    auto intervalIt = intervals.begin();
    if(!output.empty()) {
        for(; intervalIt < intervals.end(); ++intervalIt) {
            if(intervalIt->second < flashPacketTimestampsIndex[0]) {
                // the entire interval precedes the earliest packet in the flash, so even if
                // sth is missing, we have nothing in the flash for that period
                continue;
            }

            // The intervals must not overlap, although they can share (exclusive) borders
            // If that condition is met, and the sd card search algorithms have worked correctly,
            // then the current packet should be either inside the current interval, or lie after
            // it (the latter case means that nothing was found in the interval)
            assert(intervalIt->first < pktIt->packetTimestamp);

            if(intervalIt->second < pktIt->packetTimestamp) {
                // no packets were found in the current interval so we search the whole interval in the flash
                if(intervalIt->second - intervalIt->first > gapTreshold) {
                    LOG_W("Calling findPacketsInFlash (241)")
                    findPacketsInFlash(*intervalIt, std::back_inserter(output));
                    continue;
                }
            }

            // Gap between current packet and the interval begin
            time32_t intervalStartGap = pktIt->packetTimestamp - intervalIt->first;
            if(intervalStartGap > gapTreshold) {
                LOG_W("Calling findPacketsInFlash (250)")
                findPacketsInFlash({intervalIt->first, pktIt->packetTimestamp}, std::back_inserter(output));
            }

            if(std::next(pktIt) != output.end()) {
                // We are not at the last packet in the output vector. 
                // Search for gaps between packets inside the interval
                while(std::next(pktIt)->packetTimestamp < intervalIt->second) {
                    time32_t gap = std::next(pktIt)->packetTimestamp - pktIt->packetTimestamp;
                    if(gap > gapTreshold) {
                        LOG_W("Calling findPacketsInFlash (260)")
                        findPacketsInFlash({pktIt->packetTimestamp, std::next(pktIt)->packetTimestamp}, 
                                            std::back_inserter(output));
                    }
                    if(std::next(pktIt) == output.end()) {
                        // reached the last packet in the output vector
                        break;
                    }
                    ++pktIt;
                }
            }
            // Now pktIt points to the last packet inside the interval
            // We check the gap betwen the last packet inside the interval and the end of the interval
            time32_t intervalEndGap = intervalIt->second - pktIt->packetTimestamp;
            if(intervalEndGap > gapTreshold) {
                LOG_W("Calling findPacketsInFlash (275)")
                findPacketsInFlash({pktIt->packetTimestamp, intervalIt->second}, std::back_inserter(output));
            }
            ++pktIt;
            if(pktIt == output.end()) {
                ++intervalIt;
                // if packets were found for the last interval in the intervals vector, then intervalIt will point
                // to intervals.end() at this point. Otherwise it will point to the first interval 
                break;
            }

        }
    }
    
    assert(pktIt == output.end());

    // Request remaining intervals (for which no packets were found on the SD card)
    for(; intervalIt < intervals.end(); ++intervalIt) {
        findPacketsInFlash(*intervalIt, std::back_inserter(output));
    }

    return true;
}

template<class Container, size_t s_intervals>
bool PacketStorageManager::findPacketsOnSDCard(static_vector<interval_t, s_intervals> intervals, 
                                          std::back_insert_iterator<Container> outputIt) const {
    static_vector<interval_t, s_intervals> relevantIntervals{}; // intervals which may include the PREVIOUS subfolder
    for(size_t i = 1; i < subFolderTimestampsIndex.size(); ++i) {
        relevantIntervals.clear();
        const time32_t currentFolder = subFolderTimestampsIndex[i];
        const time32_t previousFolder = subFolderTimestampsIndex[i-1];

        // iterate over intervals and find intervals relevant for the current sub folder
        // since the subFolderTimestampsVector is guaranteed to be sorted, if we reach
        // the end of an interval, we can just remove it from the intervals vector, so that
        // we don't check against it again. To enable removing elements while iterating
        // over a vector, we increment the iterator manually.

        auto intervalIt = intervals.begin();
        while(intervalIt < intervals.end()) {
            if(intervalIt->first < currentFolder) {
                // the CURRENT sub folder's timestamp is within the interval, so we say
                // that the interval is relevant for the PREVIOUS subfolder (because the previous 
                // folder might, too, contain packets within the interval)
                relevantIntervals.push_back(*intervalIt);

                if(intervalIt->second < currentFolder) {
                    // the current folder's timestamp is after the current interval, so the
                    // interval is no longer relevant, and we remove it.
                    intervalIt = intervals.erase(intervalIt);
                    continue;
                }
            }
            ++intervalIt;
        }

        // find packets in the previous folder which lie in the relevant intervals, inserting
        // them into the result vector.

        if(!relevantIntervals.empty()) Log.info("Relevant intervals for %d:", previousFolder);
        for(auto& i : relevantIntervals) {
            Log.info("[%d; %d]", i.first, i.second);
        }

        if(!relevantIntervals.empty()) {
            bool sdOk = findPacketsInFolder(previousFolder, relevantIntervals, outputIt);
            if(!sdOk) return false;
            if(i == subFolderTimestampsIndex.size() - 1) {
                // current folder is the last one, so we process it as well
                // since the relevant intervals have been calculated for the previous folder,
                // and are not necessarily exhaustive for the current folder, we pass the entire
                // intervals vector
                sdOk = findPacketsInFolder(currentFolder, intervals, outputIt);
                if(!sdOk) return false;
            }
        }
        if(intervals.empty()) {
            break;  // all intervals covered
        }
    }
    return true;
}

template<class Container, size_t s_intervals>
bool PacketStorageManager::findPacketsInFolder(time32_t folderTimestamp, 
                                                  const static_vector<interval_t, s_intervals>& intervals, 
                                                  std::back_insert_iterator<Container> outputIt) const {
    os_mutex_lock(storageMutex);
    File32 dir;
    File32 file;
    // First, create an index of packet timestamps in the folder, so that we don't need to iterate over it for each intervals
    char subfolderPath[64];
    std::snprintf(subfolderPath, sizeof(subfolderPath), "/%s/%d", sysconfig.deviceId.c_str(), folderTimestamp);
    SD_TRY(dir.open(subfolderPath));
    static_vector<time32_t, SystemConfig::SD_CARD_SUBFOLDER_TIMESPAN / DataPointPacket::TIMESPAN + 1> packetTimestamps;
    file = dir.openNextFile(O_RDONLY);
    while(file) { // todo: unhandled exception
        char nameChar[64];
        size_t nameSize = file.getName(nameChar, 64);
        f_string name(nameChar, nameSize);
        if(checkPacketName(name)) {
            time32_t timestamp = std::atoi(name.c_str());
            if(timestamp != 0) {
                packetTimestamps.push_back(timestamp);
            } else {
                Log.warn("Found .pkt file with invalid filename: /%s/%d/%s", 
                         sysconfig.deviceId.c_str(), folderTimestamp, name.c_str());
                continue;
            }
        }

        file=dir.openNextFile(O_RDONLY);
    }
    SD_TRY(dir.close());
    SD_TRY(file.close());
    std::sort(packetTimestamps.begin(), packetTimestamps.end());
    
    // Then, for each interval, find packets within it.

    for(const auto& interval : intervals) {
        auto[packetsBegin, packetsEnd] = findInterval(interval, packetTimestamps.begin(), packetTimestamps.end());
        if(packetsBegin == packetTimestamps.end()) {
            continue; // not found
        }

        for(auto it = packetsBegin; it < packetsEnd; ++it) {
            outputIt = PacketDescriptor{.location = folderTimestamp, .packetTimestamp = *it};
        }
    }
    
    os_mutex_unlock(storageMutex);
    return true;
}

template<class Container>
void PacketStorageManager::findPacketsInFlash(interval_t interval, std::back_insert_iterator<Container> outputIt) {
    Log.info("Find packets in flash called, %d", System.freeMemory());
    auto[packetsBegin, packetsEnd] = findInterval(interval, flashPacketTimestampsIndex.begin(), flashPacketTimestampsIndex.end());
    for(auto it = packetsBegin; it < packetsEnd; ++it) {
        outputIt = PacketDescriptor{.location=PacketDescriptor::FLASH_LOCATION, .packetTimestamp=*it};
    }
}

template<class It>
std::pair<It, It> PacketStorageManager::findInterval(const interval_t& interval, It begin, It end) const {
    auto relevantPacketsBegin = std::find_if(begin, end, [interval](time32_t t) {
        return interval.first < t;  // first packet inside the interval
    });
    auto relevantPacketsEnd = std::find_if(begin, end, [interval](time32_t t) {
        return interval.second <= t;
    });
    return {relevantPacketsBegin, relevantPacketsEnd};
}

template<class Container>
bool PacketStorageManager::getPacket(const PacketDescriptor& d, std::back_insert_iterator<Container> outputIt) {
    os_mutex_lock(storageMutex);
    if(d.location == PacketDescriptor::FLASH_LOCATION) {
        // Packet in flash
        char path[64];
        std::snprintf(path, sizeof(path), "/Packets/ %d.pkt", d.packetTimestamp);  // todo: is this right?
        int f = open(path, O_RDONLY);
        uint8_t buf[1024];
        int size = read(f, buf, sizeof(buf));
        close(f);
        os_mutex_unlock(storageMutex);
        if(size > 0) {
            std::copy(buf, buf + size, outputIt);
            return true;
        } else {
            return false;
        }
    } else {
        // Packet on the SD card
        File32 packetFile;
        char path[64];
        std::snprintf(path, sizeof(path), "/%s/%d/%d.pkt", sysconfig.deviceId.c_str(), d.location, d.packetTimestamp);
        bool s = packetFile.open(path);
        if(s) {
            size_t size = packetFile.fileSize();
            uint8_t buf[1024];
            packetFile.read(buf, size);
            packetFile.close();
            os_mutex_unlock(storageMutex);
            std::copy(buf, buf + size, outputIt);
            return true;
        } else {
            os_mutex_unlock(storageMutex);
            return false;
        }
    }
}

#endif
