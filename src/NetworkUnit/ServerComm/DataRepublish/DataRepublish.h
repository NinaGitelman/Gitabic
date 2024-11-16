#ifndef DATAREPUBLISH_H
#define DATAREPUBLISH_H

#pragma once
#include "../TCPSocket/TCPSocket.h"
#include <condition_variable>
#include <chrono>

using std::condition_variable;
using std::pair;
using std::time_t;
using std::unique_lock;

#define TEN_MINUTES 60 * 10
#define TIME_RETRY 10

class DataRepublish
{
public:
    DataRepublish(TCPSocket *tcpSocket);
    ~DataRepublish();
    void saveData(ID fileId, EncryptedID myId);
    void stopRepublish(const ID &fileId);

private:
    void republishOldData();
    bool publish(ID fileId, EncryptedID myId);

    unordered_map<ID, pair<EncryptedID, std::time_t>> savedData;
    mutex mut;
    condition_variable cv;
    thread republishOldDataThread;
    TCPSocket *tcpSocket;
    bool isActive;
};

#endif