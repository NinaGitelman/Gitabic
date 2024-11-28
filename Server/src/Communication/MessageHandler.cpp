#include "MessageHandler.h"

MessageHandler::MessageHandler()
{
}

MessageHandler::~MessageHandler()
{
}

ResultMessage MessageHandler::handle(MessageBaseReceived msg)
{
    ResultMessage res;

    switch (msg.code)
    {
    case ClientRequestCodes::GetUserICEInfo:
        break;
    case ClientRequestCodes::NoMessageReceived:
        break;
    case ClientRequestCodes::Store:
        break;
    case ClientRequestCodes::UserListReq:
        break;
    case ClientRequestCodes::DebuggingStringMessageToSend:
        break;
    case ClientResponseCodes::AuthorizedICEConnection:
        break;
    }
    return res;
}
