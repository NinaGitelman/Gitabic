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

namespace std
{
    template <>
    struct hash<ID>
    {
        size_t operator()(const ID &id) const
        {
            size_t res = 0;
            for (size_t i = 0; i < id.size(); i++)
            {
                res ^= hash<uint8_t>()(id[i]) << i;
            }
            return res;
        }
    };
}

class DataRepublish
{
public:
    DataRepublish(const TCPSocket &socket);
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
    TCPSocket socket;
    bool isActive;
};

#endif