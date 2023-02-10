#include "PacketStorageManager.h"

PacketStorageManager::PacketStorageManager(PacketQueue& packetStorageQueue, const SdFat32& sd,
                                           const SystemConfig& config,
                                           const SystemState& sysstate, ErrorHandler& eh)
    : packetStorageQueue(packetStorageQueue), sd(sd), sysconfig(config), sysstate(sysstate),
      eh(eh)
{
}

void PacketStorageManager::start()
{
    thread = Thread{"PacketStorageManager", [this] { run(); }};
}

void PacketStorageManager::initStorage()
{    
    os_mutex_create(&storageMutex);

    if (sysstate.flashActive)
     // Directory structure: /Packets/<first data point unix timestamp>.<event name>.pkt
    {
        bool s = initFlashStorage(); // todo: add success check
        if(s) {
            Log.info("Flash init success");
        } else {
            Log.error("Flash init error");
            eh.flashError();
        }
    }

    if (sysstate.sdActive) {
        bool s = initSDCardStorage();
        if(s) {
            Log.info("SD init success");
        } else {
            Log.error("SD init error");
            eh.sdError();
        }
    }
}

bool PacketStorageManager::initSDCardStorage()
{
    os_mutex_lock(storageMutex);

    subFolderTimestampsIndex.clear();  // reset the index

    subFolderTimestampsIndex.reserve(365);  // todo: reserve more?
    
    SD_TRY(sd.begin(SdSpiConfig(SD_CARD_CS_PIN, DEDICATED_SPI, 250000, &softSpi)));  Log.info("One");  // todo: move CS pin to SystemConfig


    File32 dir;
    File32 file;  // todo: rename to subfolder for clarity
    SD_TRY(sd.chdir("/")); // go to root  
    // create board-specific directory if it does not exist
    if (!sd.exists(sysconfig.deviceId.c_str()))
    {
        SD_TRY(sd.mkdir(Particle.deviceID()))
    }

    SD_TRY(dir.open(Particle.deviceID()));
    file = dir.openNextFile(O_RDONLY);

    while(file) {  // todo: unhandled sd card error: opennextfile
        char name[64];
        size_t nameSize = file.getName(name, 64);
        time32_t timestamp = std::atoi(name);
        if(timestamp > 1600000000) {
            // valid sub-folder, add timestamp to the index
            subFolderTimestampsIndex.push_back(timestamp);
        }
        SD_TRY(file.close())
        file = dir.openNextFile(O_RDONLY);
    }
    SD_TRY(dir.close());

    std::sort(subFolderTimestampsIndex.begin(), subFolderTimestampsIndex.end());
    Log.info("SD Init completed, added %d sub-folders to the index.",
                 subFolderTimestampsIndex.size());

    os_mutex_unlock(storageMutex);

    return true;
}

bool PacketStorageManager::initFlashStorage()
{
    os_mutex_lock(storageMutex);

    DIR* packetsDir;
    if (mkdir("/Packets", 0777) != 0 && errno != EEXIST) FLASH_ERROR();
    packetsDir = opendir("/Packets");
    if (packetsDir == nullptr) FLASH_ERROR();

    // Build index of all packets currently stored in flash.
    dirent* dirEntry = readdir(packetsDir);
    while (dirEntry != nullptr)
    {
        f_string name(dirEntry->d_name);
        time32_t timestamp = std::atoi(name.c_str());
        if(checkPacketName(name)) {
            if (timestamp > 1600000000) {
                flashPacketTimestampsIndex.push_back(timestamp);
            } else {
                Log.warn("Invalid packet found in flash: /Packets/%s", name.c_str());
                dirEntry = readdir(packetsDir);
            }
        }
        dirEntry = readdir(packetsDir);
    }
    if (errno) FLASH_ERROR();
    if (closedir(packetsDir) != 0) FLASH_ERROR();
    if (flashPacketTimestampsIndex.size() > sysconfig.FLASH_MAX_PACKETS) FLASH_ERROR();

    std::sort(flashPacketTimestampsIndex.begin(), flashPacketTimestampsIndex.end());
    Log.info("%d packets found in flash.", flashPacketTimestampsIndex.size());

    os_mutex_unlock(storageMutex);
    return true;
}

void PacketStorageManager::run()
{
    Packet packet;
    while (true)
    {
        packetStorageQueue.take(&packet, CONCURRENT_WAIT_FOREVER);

        if(std::strcmp(packet.getEventName(), DataPointPacket::eventName)) {
            // Packet is not DataPointPacket
            Log.error("Packet Storage Manager received invalid packet from the packet storage queue.");
        }
        
        if(sysstate.flashActive) {
            Log.info("Flash active, trying to save the packet");
            bool s = savePacketToFlash(packet);
            if(!s) eh.flashError();
        } else {
            Log.info("Flash not active, not saving");
        }
        if(sysstate.sdActive) {
            Log.info("SD active, trying to save the packet.");
            bool s = savePacketToSD(packet);
            Log.info("Saved to the SD card!");
            if(!s) eh.sdError();
        } else {
            Log.info("SD is not active, not saving.");
        }
    }
}

bool PacketStorageManager::savePacketToSD(const Packet& packet)
{
    // Wait for storage to become available
    os_mutex_lock(storageMutex);
    // Produce filename and get binary data from the Packet instance
    f_string filename = packet.makeFilename();

    uint16_t dataSize;
    const uint8_t* data = packet.getBytes(&dataSize);

    // Write to the sd card
    // Structure: /<device id>/<start of interval timestamp>/<timestamp>.<event name>.pkt
    File32 file;
    time32_t pt = packet.getTimestamp();
    time32_t usedSubFolderTimestamp = 0; // where we are going to write the file
    if(!subFolderTimestampsIndex.empty()) {
        time32_t d = Time.now() - subFolderTimestampsIndex.back();
        if (d < 0) {
            Log.error("Timestamp of the latest SD card subfolder is in the future");
            for(int i = 0; i < subFolderTimestampsIndex.size() - 1; ++i) {
                auto t1 = subFolderTimestampsIndex[i];
                auto t2 = subFolderTimestampsIndex[i+1];
                if(t1 <= pt && pt <= t2) {
                    usedSubFolderTimestamp = t1;
                    break;
                }
            }
        } else if (d < sysconfig.SD_CARD_SUBFOLDER_TIMESPAN) {
            // we can use the latest sub-foler
            Log.warn("Saving packet to the latest subfolder.");
            usedSubFolderTimestamp = subFolderTimestampsIndex.back();
        } else {
            // the latest sub-folder is too old, so we're creating a new one
            Log.warn("Latest sub-folder too old, creating a new one.");
            usedSubFolderTimestamp = pt;
            subFolderTimestampsIndex.push_back(usedSubFolderTimestamp);
        }

    } else {
        // no sub-folders yet
        Log.info("No subfolders found, creating new one.");
        usedSubFolderTimestamp = pt;
        subFolderTimestampsIndex.push_back(usedSubFolderTimestamp);
    }

    char subfolderPath[64];
    std::snprintf(subfolderPath, sizeof(subfolderPath), "/%s/%d", sysconfig.deviceId.c_str(), usedSubFolderTimestamp);

    if(!sd.exists(subfolderPath)) {
        SD_TRY(sd.mkdir(subfolderPath))
    }
    SD_TRY(sd.chdir(subfolderPath))
    SD_TRY(file.open(filename.c_str(), O_RDWR | O_CREAT));
    SD_TRY(file.write(data, dataSize));
    SD_TRY(file.close());
    SD_TRY(sd.chdir("/"));

    os_mutex_unlock(storageMutex);
    return true;
}

bool PacketStorageManager::savePacketToFlash(const Packet& packet) {
    assert(!std::strcmp(packet.getEventName(), DataPointPacket::eventName));

    os_mutex_lock(storageMutex);
    if(flashPacketTimestampsIndex.size() == sysconfig.FLASH_MAX_PACKETS) {
        //The memory is full. We erase the earliest packet file and remove
        // the corresponding timestamp from the index vector.
        char path[64];
        snprintf(path, sizeof(path), "/Packets/%d.pkt", flashPacketTimestampsIndex[0]);
        if (unlink(path) != 0) FLASH_ERROR();
        flashPacketTimestampsIndex.erase(flashPacketTimestampsIndex.begin());
    } else if (flashPacketTimestampsIndex.size() > sysconfig.FLASH_MAX_PACKETS) {
        FLASH_ERROR();
    }
    
    // Write new packet file and add its timestamp to the index vector.
    char path[64];
    snprintf(path, sizeof(path), "/Packets/%s", packet.makeFilename().c_str());
    int f = open(path, O_RDWR | O_CREAT);
    uint16_t dataSize;
    const uint8_t* data = packet.getBytes(&dataSize);
    if (f == -1) FLASH_ERROR();
    if (write(f, data, dataSize) == -1) FLASH_ERROR();
    if (close(f) == -1) FLASH_ERROR();

    flashPacketTimestampsIndex.push_back(packet.getTimestamp());

    os_mutex_unlock(storageMutex);
    return true;
}

bool PacketStorageManager::checkPacketName(const f_string& name) const {
    size_t pointPos = name.find('.');
    if(pointPos == std::string::npos) {
        return false;
    }
    return name.substr(pointPos+1) == "pkt";
}