#include "Connection.h"

Connection::Connection(Address peer) : peerAdress(peer) {}

PacketData Connection::getPacketToSend()
{
    auto ret = PacketsToSend.front();
    PacketsToSend.pop();
    return ret;
}

bool Connection::getIsAlive()
{
    return isAlive;
}
