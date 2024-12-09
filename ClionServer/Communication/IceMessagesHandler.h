#ifndef ICEMESSAGESHANDLER_H
#define ICEMESSAGESHANDLER_H

#pragma once

#include <memory> // For std::unique_ptr
#include <mutex>  // For std::once_flag and std::call_once
#include "../NetworkUnit/ServerCom/Messages.h"
#include <unordered_map>

class IceMessagesHandler
{
public:
    // Deleted methods to ensure a single instance
    IceMessagesHandler(const IceMessagesHandler &) = delete;
    IceMessagesHandler &operator=(const IceMessagesHandler &) = delete;

    // Access the single instance
    static IceMessagesHandler &getInstance();

    ResultMessage handle(const ClientRequestGetUserICEInfo &request);
    ResultMessage handle(const ClientResponseAuthorizedICEConnection &request);

private:
    // Constructor and Destructor are private
    IceMessagesHandler() : waitIds(0) {}

    std::unordered_map<uint16_t, ID> waitsForResponse;
    uint16_t waitIds;
    // Static instance
    static std::unique_ptr<IceMessagesHandler> instance;
    static std::once_flag initInstanceFlag;
};

#endif // ICEMESSAGESHANDLER_H
