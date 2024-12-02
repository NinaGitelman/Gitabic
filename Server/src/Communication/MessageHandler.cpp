#include "MessageHandler.h"

// Static member initialization
std::unique_ptr<MessageHandler> MessageHandler::instance = nullptr;
std::once_flag MessageHandler::initInstanceFlag;

// Private constructor
MessageHandler::MessageHandler()
{
    iceMessagesHandler = &IceMessagesHandler::getInstance();
}

// Access the singleton instance
MessageHandler &MessageHandler::getInstance()
{
    std::call_once(initInstanceFlag, []()
                   { instance.reset(new MessageHandler()); });
    return *instance;
}

// Functionality
ResultMessage MessageHandler::handle(MessageBaseReceived msg)
{
    ResultMessage res;

    switch (msg.code)
    {
    case ClientRequestCodes::GetUserICEInfo:
        return iceMessagesHandler->handle(ClientRequestGetUserICEInfo(msg));
    case ClientRequestCodes::NoMessageReceived:
        // Handle NoMessageReceived logic
        break;
    case ClientRequestCodes::Store:
        // Handle Store logic
        break;
    case ClientRequestCodes::UserListReq:
        // Handle UserListReq logic
        break;
    case ClientRequestCodes::DebuggingStringMessage:
        // Handle DebuggingStringMessageToSend logic
        break;
    case ClientResponseCodes::AuthorizedICEConnection:
        return iceMessagesHandler->handle(ClientResponseAuthorizedICEConnection(msg));
    default:
        // Handle unknown codes or errors
        break;
    }
    return res;
}
