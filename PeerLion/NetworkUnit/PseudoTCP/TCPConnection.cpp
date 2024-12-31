#include "TCPConnection.h"

TCPConnection::TCPConnection(const Address &peer) : peerAdress(peer) {}

vector<uint8_t> TCPConnection::getPacketToSend()
{
    auto ret = PacketsToSend.front();
    PacketsToSend.pop();
    return ret;
}

bool TCPConnection::getIsAlive() const
{
    return isAlive;
}
