#pragma once

#include <iostream>
#include <array>
#include <queue>
#include <vector>
#include <mutex>

#include "NetworkUnit/SocketHandler/SocketHandler.h"
class SocketHandler;

using std::array;
using std::mutex;
using std::queue;
using std::vector;

struct Address
{
    std::array<uint8_t, 4> ip;
    uint16_t port;

    bool operator==(Address &other)
    {
        return other.port == this->port &&
               other.ip == this->ip;
    }
};

struct Message // TODO: implement
{
};
struct PacketData // TODO: implement
{
};

class Connection
{
protected:
    Address peerAdress;
    queue<Message> messafeRecvQueue;
    vector<uint8_t> receivedData;
    mutex recvMutex;

    queue<PacketData> PacketsToSend;
    mutex sendMutex;

    bool isAlive;
    friend class SocketHandler;

    Connection(Address peer);
    virtual void onReceive(vector<uint8_t> data) = 0;
    virtual PacketData getPacketToSend();

public:
    bool getIsAlive();
    virtual bool close() = 0;
    virtual void send(Message msg) = 0;
};