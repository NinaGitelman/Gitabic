#include "TrackerMessageHandling.h"

// Define static members
std::unique_ptr<TrackerMessageHandling> TrackerMessageHandling::instance = nullptr;
std::once_flag TrackerMessageHandling::initInstanceFlag;

// Access the single instance
TrackerMessageHandling &TrackerMessageHandling::getInstance()
{
    std::call_once(initInstanceFlag, []()
                   { instance.reset(new TrackerMessageHandling()); });
    return *instance;
}

ResultMessage TrackerMessageHandling::handle(const ClientRequestUserList &request) const {
    ResultMessage res;
    res.id = request.from;
    res.msg = std::make_shared<ServerResponseUserList>(request.fileId, trackerDataStorage.getRegisteredData(request.fileId, request.from));
    return res;
}

ResultMessage TrackerMessageHandling::handle(const ClientRequestStore &request) const {
    ResultMessage res;
    res.id = request.from;
    trackerDataStorage.saveData(request.fileId, request.myId);
    res.msg = std::make_shared<MessageBaseToSend>(ServerResponseCodes::StoreSuccess);
    return res;
}

TrackerMessageHandling::TrackerMessageHandling() : trackerDataStorage(TrackerDataStorage::getInstance()) {}
