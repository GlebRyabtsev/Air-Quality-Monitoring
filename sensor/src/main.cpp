#include "Particle.h"
#include <sys/stat.h>
/*
 * Project KIST_Sensor_Argon_Fw
 * Description:
 * Author:
 * Date:
 */

// todo: disable automatic cleaning and clean fan once per week
// todo: turn off SPS30s on shutdown

#include "MeasurementCollector.h"
#include "Packets/Packet.h"
#include "PacketPublisher.h"
#include "PacketQueue.h"
#include "PacketStorageManager.h"
#include "HandshakeHandler.h"

#include <variant>

#define ENABLE_CLEAR_FLASH_FUNCTION

SYSTEM_THREAD(ENABLED);

void syncTime();

#ifdef ENABLE_CLEAR_FLASH_FUNCTION
int clearFlash(const String& arg);
#endif

int handshake(const char *arg);

SerialLogHandler logHandler(LOG_LEVEL_INFO);
Timer timeSyncTimer = Timer(24 * 60 * 60 * 1000, syncTime);

PacketQueue packetPublishingQueue;

PacketQueue packetStorageQueue;

SystemConfig sysconfig;

SystemState sysstate;

SdFat32 sd;

ErrorHandler *eh;
MeasurementCollector *mc;
PacketStorageManager *psm;
PacketPublisher *pp;
HandshakeHandler *hh;

void setup()
{
    eh = new ErrorHandler{packetPublishingQueue, packetStorageQueue, sysconfig, sysstate};
    mc = new MeasurementCollector{packetPublishingQueue, packetStorageQueue, sysconfig, sysstate};
    psm = new PacketStorageManager{packetStorageQueue, sd, sysconfig, sysstate, *eh};
    pp = new PacketPublisher{packetPublishingQueue, sysconfig, sysstate};
    hh = new HandshakeHandler{*psm, packetPublishingQueue, sysstate, *eh};

    delay(3000); // necessary for logging to work in the init functions

    System.enableFeature(FEATURE_RESET_INFO);
    if(System.resetReason() == RESET_REASON_PANIC) {
        while(!Particle.connected()) {
            delay(1000);
        }
        Particle.publish("error", "Reset because of system panic occurred.");
        while(1) delay(1000);
    }

    sysconfig.deviceId = std::string(Particle.deviceID().c_str());

    packetPublishingQueue.init(sysconfig.PACKET_QUEUE_CAPACITY);
    packetStorageQueue.init(sysconfig.PACKET_QUEUE_CAPACITY);

    // sysstate.serialLogEnabled = true;

    Serial.begin(115200);

#ifdef ENABLE_CLEAR_FLASH_FUNCTION
    Particle.function("clearFlash", clearFlash);
#endif
    Particle.function("handshake", handshake);

    Particle.syncTime();

    eh->init();
    psm->initStorage();

    //auto res = psm.getPacket(PacketStorageManager::PacketDescriptor{.location=PacketStorageManager::PacketDescriptor::FLASH_LOCATION, .packetTimestamp=1652555097});

    // if(std::holds_alternative<DataPointPacket>(res)) {
    //     Log.info("It works!");
    // }


    mc->start();
    psm->start();
    pp->start();

    hh->start();


    // static_vector<PacketStorageManager::PacketDescriptor, SystemConfig::MAX_REQUESTED_PACKETS_PER_HANDSHAKE> packetDescriptors{};
    // static_vector<interval_t, 101> intervals{};
    // intervals.push_back({1652800330, 1652800694});
    // intervals.push_back({1652804167, 1652804531});
    // intervals.push_back({1652805561, 1652809444});
    // intervals.push_back({1652809778, 1652810984});
    // intervals.push_back({1652811318, 1652812529});
    // intervals.push_back({1652812863, 1652813128});
    // intervals.push_back({1652813812, 1652815630});
    // // intervals.push_back({1652800330, 1652800694});

    // // 
    // //std::vector<PacketStorageManager::PacketDescriptor> output{};
    // // static_vector<PacketStorageManager::PacketDescriptor, SystemConfig::MAX_REQUESTED_PACKETS_PER_HANDSHAKE> outputStat{};
    // // psm->findPackets(intervals, outputStat);
    // // for(const PacketStorageManager::PacketDescriptor& d : outputStat) {
    // //     Log.info("%d, %d", d.location, d.packetTimestamp);
    // // }
    // // Log.info("TRYING AGAIN");
    // // outputStat.clear();
    // // psm->findPackets(intervals, outputStat);
    // // for(const PacketStorageManager::PacketDescriptor& d : outputStat) {
    // //     Log.info("%d, %d", d.location, d.packetTimestamp);
    // // }
}

void loop()
{
    delay(1000);
}

void syncTime()
{
    Particle.syncTime(); // should run async
}

void clearDir(const String& path, bool selfDestruct) {
    DIR* dir = opendir(path.c_str());
    dirent* dirEntry = readdir(dir);
    while(dirEntry != nullptr) {
        String name{dirEntry->d_name};
        String pathToEntry = path + "/" + name;
        auto *statBuf = new struct stat;
        stat(pathToEntry.c_str(), statBuf);
        if(statBuf->st_mode & S_IFDIR) {
            if(!name.equals(".") && !name.equals("..")) {
                // is a directory
                clearDir(pathToEntry, true);
                rmdir(pathToEntry.c_str());
            }
        } else {
            // is a file
            unlink(pathToEntry.c_str());
        }
        dirEntry = readdir(dir);
    }
    closedir(dir);
    if(selfDestruct) {
        rmdir(path.c_str());
    }
}

int clearFlash(const String& arg)
{
    Log.info("Clear flash called");
    clearDir("/Packets", false);
    Log.info("Clear flash done");
    return 0;
}

int handshake(const char* arg) {
    return hh->putHandshake(arg);    
}