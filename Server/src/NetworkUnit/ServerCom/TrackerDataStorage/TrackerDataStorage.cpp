#include "TrackerDataStorage.h"

// Define the static members
std::unique_ptr<TrackerDataStorage> TrackerDataStorage::instance = nullptr;
std::mutex TrackerDataStorage::instanceMutex;

TrackerDataStorage &TrackerDataStorage::getInstance()
{
    std::lock_guard<std::mutex> lock(instanceMutex);
    if (!instance)
    {
        instance = std::unique_ptr<TrackerDataStorage>(new TrackerDataStorage());
    }
    return *instance;
}

TrackerDataStorage::TrackerDataStorage()
{
    isActive = true;
    removeOldDataThread = std::thread(&TrackerDataStorage::removeOldData, this);
    removeOldDataThread.detach();
}

TrackerDataStorage::~TrackerDataStorage()
{
    isActive = false;
    if (removeOldDataThread.joinable())
    {
        removeOldDataThread.join();
    }
}

void TrackerDataStorage::saveData(const ID &key, const ID &value)
{
    std::lock_guard<std::mutex> guard(mut);
    savedData[time(nullptr) + TEN_MINUTES] = std::make_pair(key, value);
}

void TrackerDataStorage::removeOldData()
{
    while (isActive)
    {
        time_t current = time(nullptr);

        // Delete expired entries
        {
            std::lock_guard<std::mutex> lock(mut);
            std::erase_if(savedData, [current](const auto &pair)
                          { return pair.first < current; });
        }

        // Sleep until the next expiration time or default delay
        {
            std::unique_lock<std::mutex> lock(mut);
            if (savedData.empty())
            {
                lock.unlock();
                std::this_thread::sleep_for(std::chrono::seconds(TEN_MINUTES - 1));
                continue;
            }

            time_t closest = current + TEN_MINUTES;
            for (const auto &entry : savedData)
            {
                if (entry.first < closest)
                {
                    closest = entry.first;
                }
            }
            lock.unlock();
            std::this_thread::sleep_for(std::chrono::seconds(closest - current));
        }
    }
}
