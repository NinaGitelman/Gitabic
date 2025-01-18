#include "IceMessagesHandler.h"

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
    ResultMessage res;
    res.id = request.RequestedUserId;
    res.msg = std::make_shared<ServerRequestAuthorizeICEConnection>(request.iceCandidateInfo, waitIds, request.from);
    waitsForResponse[waitIds++] = request.from;
    return res;
}

ResultMessage IceMessagesHandler::handle(const ClientResponseAuthorizedICEConnection &request) {
    ResultMessage res;
    res.id = waitsForResponse[request.requestId];
    waitsForResponse.erase(request.requestId);
    res.msg = std::make_shared<ServerResponseUserAuthorizedICEData>(request.iceCandidateInfo);
    return res;
}

ResultMessage IceMessagesHandler::handle(const ClientResponseAlreadyConnected &request) {
    ResultMessage res;
    res.id = waitsForResponse[request.requestID];
    waitsForResponse.erase(request.requestID);
    res.msg = std::make_shared<MessageBaseToSend>(ServerResponseCodes::UserAlreadyConnected);
    return res;
}

ResultMessage IceMessagesHandler::handle(const ClientResponseFullCapacity &request) {
    ResultMessage res;
    res.id = waitsForResponse[request.requestID];
    waitsForResponse.erase(request.requestID);
    res.msg = std::make_shared<MessageBaseToSend>(ServerResponseCodes::UserFullCapacity);
    return res;
}
