#include "DataRepublish.h"

DataRepublish::DataRepublish(const TCPSocket &socket) : socket(socket)
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
    savedData[fileId] = pair<EncryptedID, std::time_t>(myId, time(nullptr));
}

void DataRepublish::stopRepublish(const ID &fileId)
{
    savedData.erase(fileId);
}

void DataRepublish::republishOldData()
{
    while (isActive)
    {
        std::unique_lock<mutex> lock(mut);
        lock.lock();
        if (this->savedData.size() < 1)
        {
            lock.unlock();
            std::this_thread::sleep_for(std::chrono::seconds(TEN_MINUTES - 1));
            continue;
        }
    }
}

bool DataRepublish::publish(ID fileId, EncryptedID myId)
{
    StoreRequest request(fileId, myId);
    socket.sendRequest(request);
    return socket.recieve().code == ResponseCodes::StoreSuccess;
}
