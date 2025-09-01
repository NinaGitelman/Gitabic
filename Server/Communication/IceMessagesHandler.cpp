#include "IceMessagesHandler.h"

#include <ranges>

// Define static members
std::unique_ptr<IceMessagesHandler> IceMessagesHandler::instance = nullptr;
std::once_flag IceMessagesHandler::initInstanceFlag;

// Access the single instance
IceMessagesHandler &IceMessagesHandler::getInstance()
{
    std::call_once(initInstanceFlag, []()
                   { instance.reset(new IceMessagesHandler()); });
    return *instance;
}

ResultMessage IceMessagesHandler::handle(const ClientRequestGetUserICEInfo &request)
{
    const std::pair opositeRequest(request.RequestedUserId, request.from);
    const std::pair sameRequest( request.from, request.RequestedUserId);
    for (auto [reqID, requestIDs] : waitsForResponse) {
        if ((opositeRequest == requestIDs || sameRequest == requestIDs) &&
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - requestsTime[reqID]) < std::chrono::seconds(10)) {
            return ResultMessage{request.from, std::make_shared<MessageBaseToSend>(ServerResponseCodes::ExistsOpositeRequest)};
        }
    }
    ResultMessage res;
    res.id = request.RequestedUserId;
    res.msg = std::make_shared<ServerRequestAuthorizeICEConnection>(request.iceCandidateInfo, waitIds, request.from);
    requestsTime[waitIds] = std::chrono::system_clock::now();
    waitsForResponse[waitIds++] = std::pair(request.from, request.RequestedUserId);
    return res;
}

ResultMessage IceMessagesHandler::handle(const ClientResponseAuthorizedICEConnection &request) {
    ResultMessage res;
    res.id = waitsForResponse[request.requestId].first;
    waitsForResponse.erase(request.requestId);
    requestsTime.erase(request.requestId);
    res.msg = std::make_shared<ServerResponseUserAuthorizedICEData>(request.iceCandidateInfo);
    std::cout << "Ice request\n";
    return res;
}

ResultMessage IceMessagesHandler::handle(const ClientResponseAlreadyConnected &request) {
    ResultMessage res;
    res.id = waitsForResponse[request.requestID].first;
    waitsForResponse.erase(request.requestID);
    requestsTime.erase(request.requestID);
    res.msg = std::make_shared<MessageBaseToSend>(ServerResponseCodes::UserAlreadyConnected);
    std::cout << "Ice response\n";
    return res;
}

ResultMessage IceMessagesHandler::handle(const ClientResponseFullCapacity &request) {
    ResultMessage res;
    res.id = waitsForResponse[request.requestID].first;
    waitsForResponse.erase(request.requestID);
    requestsTime.erase(request.requestID);
    res.msg = std::make_shared<MessageBaseToSend>(ServerResponseCodes::UserFullCapacity);
    return res;
}
