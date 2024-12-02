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
    /// @brief constructs and starts a thread for republishing
    /// @param tcpSocket the socket to publish in
    DataRepublish(TCPSocket *tcpSocket);
    /// @brief Stops the thread
    ~DataRepublish();
    /// @brief Start publishing the data
    /// @param fileId The file ID you have
    /// @param myId your encrypted ID
    void saveData(ID fileId, ID myId);
    /// @brief Stops publishing data
    /// @param fileId The file to stop publish
    void stopRepublish(const ID &fileId);

private:
    /// @brief Runs in a thread and republish each data once every ten minutes
    void republishOldData();
    /// @brief Sends the data through the socket
    /// @param fileId The file you have
    /// @param myId Your encrypted ID
    /// @return Is the store successful.
    bool publish(ID fileId, ID myId);

    /// @brief The saved data and next time nee to be published
    unordered_map<ID, pair<ID, std::time_t>> savedData;
    mutex mut;
    /// @brief Thread that runs republishOldData method
    thread republishOldDataThread;
    /// @brief The socket to the server
    TCPSocket *tcpSocket;
    bool isActive;
};

#endif