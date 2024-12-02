#include "MessageHandler.h"

// Static member initialization
std::unique_ptr<MessageHandler> MessageHandler::instance = nullptr;
std::once_flag MessageHandler::initInstanceFlag;

// Private constructor
MessageHandler::MessageHandler() : iceMessagesHandler(IceMessagesHandler::getInstance()),
                                   trackerMessagesHandler(TrackerMessageHandling::getInstance())
{
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
        res = iceMessagesHandler.handle(ClientRequestGetUserICEInfo(msg));
        break;
    case ClientRequestCodes::NoMessageReceived:
        // Handle NoMessageReceived logic
        break;
    case ClientRequestCodes::Store:
        res = trackerMessagesHandler.handle(ClientRequestStore(msg));
        break;
    case ClientRequestCodes::UserListReq:
        res = trackerMessagesHandler.handle(ClientRequestUserList(msg));
        break;
    case ClientRequestCodes::DebuggingStringMessage:
        // Handle DebuggingStringMessageToSend logic
        break;
    case ClientResponseCodes::AuthorizedICEConnection:
        res = iceMessagesHandler.handle(ClientResponseAuthorizedICEConnection(msg));
        break;
    default:
        // Handle unknown codes or errors
        break;
    }
    return res;
}
