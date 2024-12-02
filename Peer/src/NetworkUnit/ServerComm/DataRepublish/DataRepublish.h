#ifndef DATAREPUBLISH_H
#define DATAREPUBLISH_H

#pragma once
#include "../TCPSocket/TCPSocket.h"
#include <condition_variable>
#include <chrono>
#include <unordered_map>
#include <memory>

#define TEN_MINUTES 60 * 10
#define TIME_RETRY 10

using std::condition_variable;
using std::mutex;
using std::pair;
using std::time_t;
using std::unique_lock;
using std::unordered_map;

class DataRepublish
{
public:
    /// @brief Gets the Singleton instance of DataRepublish
    /// @param tcpSocket the socket to publish in
    /// @return The single instance of DataRepublish
    static DataRepublish &getInstance(TCPSocket *tcpSocket);

    /// @brief Stops the thread and deletes the instance
    ~DataRepublish();

    /// @brief Start publishing the data
    /// @param fileId The file ID you have
    /// @param myId Your encrypted ID
    void saveData(ID fileId, ID myId);

    /// @brief Stops publishing data
    /// @param fileId The file to stop publish
    void stopRepublish(const ID &fileId);

private:
    // Private constructor
    DataRepublish(TCPSocket *tcpSocket);

    /// @brief Runs in a thread and republish each data once every ten minutes
    void republishOldData();

    /// @brief Sends the data through the socket
    /// @param fileId The file you have
    /// @param myId Your encrypted ID
    /// @return Is the store successful.
    bool publish(ID fileId, ID myId);

    /// @brief The saved data and next time needs to be published
    unordered_map<ID, pair<ID, std::time_t>> savedData;
    mutex mut;

    /// @brief Thread that runs republishOldData method
    thread republishOldDataThread;

    /// @brief The socket to the server
    TCPSocket *tcpSocket;

    /// @brief Controls whether the thread should keep running
    bool isActive;

    /// @brief The single instance of DataRepublish
    static std::unique_ptr<DataRepublish> instance;

    /// @brief Mutex for thread-safe instance initialization
    static mutex instanceMutex;
};

#endif
