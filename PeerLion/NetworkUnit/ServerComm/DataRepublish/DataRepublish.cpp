#include "DataRepublish.h"

#include <utility>

// Define the static members
std::unique_ptr<DataRepublish> DataRepublish::instance = nullptr;
mutex DataRepublish::instanceMutex;

DataRepublish &DataRepublish::getInstance(const std::shared_ptr<TCPSocket>& tcpSocket)
{
    std::lock_guard<mutex> lock(instanceMutex);
    if (!instance)
    {
        instance = std::unique_ptr<DataRepublish>(new DataRepublish(tcpSocket));
    }
    return *instance;
}

DataRepublish::DataRepublish(const std::shared_ptr<TCPSocket>& tcpSocket) : tcpSocket(tcpSocket)
{
    isActive = true;
    republishOldDataThread = thread(&DataRepublish::republishOldData, this);
    republishOldDataThread.detach();
}

DataRepublish::~DataRepublish()
{
    isActive = false;
    if (republishOldDataThread.joinable())
    {
        republishOldDataThread.join();
    }
}

void DataRepublish::saveData(ID fileId, ID myId)
{
    std::lock_guard<mutex> guard(mut);
    savedData[fileId] = pair<ID, std::time_t>(myId, time(nullptr) + TEN_MINUTES);
    publish(fileId, myId);
}

void DataRepublish::stopRepublish(const ID &fileId)
{
    std::lock_guard<mutex> guard(mut);
    savedData.erase(fileId);
}

void DataRepublish::republishOldData()
{
    while (isActive)
    {
        time_t current = time(nullptr);

        for (auto &it : savedData)
        {
            if (it.second.second < current)
            {
                if (publish(it.first, it.second.first))
                {
                    it.second.second = current + TEN_MINUTES;
                }
                else
                {
                    it.second.second = current + TIME_RETRY;
                }
            }
        }

        std::unique_lock<mutex> lock(mut);
        if (savedData.empty())
        {
            lock.unlock();
            std::this_thread::sleep_for(std::chrono::seconds(TEN_MINUTES - 1));
            continue;
        }
        lock.unlock();

        time_t closest = current + TEN_MINUTES;
        for (const auto &it : savedData)
        {
            if (it.second.second < closest)
            {
                closest = it.second.second;
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(closest - current));
    }
}

bool DataRepublish::publish(ID fileId, ID myId)
{
    StoreRequest request(fileId, myId);
    tcpSocket->sendRequest(request);

    auto isRelevant = [](uint8_t code)
    {
        return code == ServerResponseCodes::StoreFailure ||
               code == ServerResponseCodes::StoreSuccess;
    };

    return tcpSocket->receive(isRelevant).code == ServerResponseCodes::StoreSuccess;
}
