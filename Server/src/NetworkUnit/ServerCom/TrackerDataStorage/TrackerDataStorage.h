#ifndef TRACKERDATASTORAGE_H
#define TRACKERDATASTORAGE_H

#pragma once

#include "../Messages.h"
#include <condition_variable>
#include <chrono>
#include <unordered_map>
#include <memory>
#include <thread>

using std::condition_variable;
using std::mutex;
using std::pair;
using std::thread;
using std::time_t;
using std::unique_lock;

#define TEN_MINUTES 10 * 60

class TrackerDataStorage
{
public:
    /// @brief Retrieves the Singleton instance
    static TrackerDataStorage &getInstance();

    /// @brief Destructor
    ~TrackerDataStorage();

    /// @brief Saves the data with an expiration time
    /// @param key The key to store
    /// @param value The associated value
    void saveData(const ID &key, const ID &value);
    vector<ID> getRegisteredData(const ID &fileId);

private:
    // Private constructor for Singleton pattern
    TrackerDataStorage();

    /// @brief Removes old data from storage
    void removeOldData();

    /// @brief Thread for automatic data cleanup
    std::thread removeOldDataThread;

    /// @brief Storage for data with expiration time as the key
    std::unordered_map<std::time_t, std::pair<ID, ID>> savedData;

    /// @brief Mutex for thread-safe access
    std::mutex mut;

    /// @brief Controls the activity of the cleanup thread
    bool isActive;

    /// @brief The Singleton instance
    static std::unique_ptr<TrackerDataStorage> instance;

    /// @brief Mutex for instance creation
    static mutex instanceMutex;
};

#endif
