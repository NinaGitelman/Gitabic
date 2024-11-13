#pragma once

#include "../SocketHandler/SocketHandler.h"
class SocketHandler;

struct Message
{
    uint8_t code;
    uint32_t size;
    vector<uint8_t> data;
    Address other;
};

class Connection
{
protected:
    Address peerAdress;
    queue<Message> messafeRecvQueue;
    vector<uint8_t> receivedData;
    mutex recvMutex;

    queue<vector<uint8_t>> PacketsToSend;
    mutex sendMutex;

    bool isAlive;
    friend class SocketHandler;

    Connection(const Address &peer);
    virtual void onReceive(vector<uint8_t> data) = 0;
    virtual vector<uint8_t> getPacketToSend();

public:
    bool getIsAlive() const;
    virtual bool close() = 0;
    virtual void send(Message msg) = 0;
};