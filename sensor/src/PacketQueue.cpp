#include "PacketQueue.h"

void PacketQueue::init(size_t size, bool autoEmpty)
{
    this->autoEmpty = autoEmpty;
    os_queue_create(&queue, sizeof(Packet), size, nullptr);
}

bool PacketQueue::push(const Packet &packet)
{
    Packet basePacket = packet; // convert any Packet child to Packet base class to ensure correct
                                // byte-copying.
    if (os_queue_put(queue, &basePacket, 0, nullptr) != 0)
    {
        if (autoEmpty)
        {
            Packet wasteBin;
            while (os_queue_take(queue, &wasteBin, 0, nullptr))
                ;
            os_queue_put(queue, &basePacket, CONCURRENT_WAIT_FOREVER, nullptr);
            return true;
        } else {

            return false;   
        }
    }
    else
    {
        return true;
    }
}

bool PacketQueue::take(Packet *packet, system_tick_t del)
{
    return os_queue_take(queue, packet, del, nullptr) == 0;
}