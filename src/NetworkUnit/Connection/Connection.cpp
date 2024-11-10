#include "Connection.h"

Connection::Connection(Address peer) : peerAdress(peer) {}

vector<uint8_t> Connection::getPacketToSend()
{
    auto ret = PacketsToSend.front();
    PacketsToSend.pop();
    return ret;
}

bool Connection::getIsAlive()
{
    return isAlive;
}
