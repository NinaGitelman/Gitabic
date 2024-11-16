#include "DataRepublish.h"

DataRepublish::DataRepublish(TCPSocket *tcpSocket) : tcpSocket(tcpSocket)
{
    isActive = true;
    republishOldDataThread = thread(&DataRepublish::republishOldData, this);
    republishOldDataThread.detach();
}

DataRepublish::~DataRepublish()
{
    isActive = false;
    republishOldDataThread.join();
}

void DataRepublish::saveData(ID fileId, EncryptedID myId)
{
    std::lock_guard<mutex> guard(mut);
    savedData[fileId] = pair<EncryptedID, std::time_t>(myId, time(nullptr) + TEN_MINUTES);
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
        if (this->savedData.size() < 1)
        {
            lock.unlock();
            std::this_thread::sleep_for(std::chrono::seconds(TEN_MINUTES - 1));
            continue;
        }
        time_t closest = current + TEN_MINUTES;
        for (auto &it : savedData)
        {
            if (it.second.second < closest)
            {
                closest = it.second.second;
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(closest - current));
    }
}

bool DataRepublish::publish(ID fileId, EncryptedID myId)
{
    StoreRequest request(fileId, myId);
    tcpSocket->sendRequest(request);
    auto isRelevant = [](uint8_t code)
    {
        switch (code)
        {
        case ResponseCodes::StoreFailure:
        case ResponseCodes::StoreSuccess:
            return true;
        }
        return false;
    };
    return tcpSocket->recieve(isRelevant).code == ResponseCodes::StoreSuccess;
}
