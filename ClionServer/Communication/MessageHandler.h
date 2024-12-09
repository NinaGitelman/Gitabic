#ifndef MESSAGEHANDLER_H
#define MESSAGEHANDLER_H

#pragma once

#include "IceMessagesHandler.h"
#include "TrackerMessageHandling.h"
class MessageHandler
{
public:
    // Deleted copy constructor and assignment operator to enforce singleton
    MessageHandler(const MessageHandler &) = delete;
    MessageHandler &operator=(const MessageHandler &) = delete;

    // Access the singleton instance
    static MessageHandler &getInstance();

    // Functionality
    ResultMessage handle(MessageBaseReceived msg);

private:
    // Private constructor and destructor
    MessageHandler();

    // Static instance and flag
    static std::unique_ptr<MessageHandler> instance;
    static std::once_flag initInstanceFlag;
    IceMessagesHandler &iceMessagesHandler;
    TrackerMessageHandling &trackerMessagesHandler;
};

#endif // MESSAGEHANDLER_H
