#ifndef TRACKERMESSAGEHANDLING_H
#define TRACKERMESSAGEHANDLING_H

#pragma once

#include <memory> // For std::unique_ptr
#include <mutex>  // For std::once_flag and std::call_once
#include "../NetworkUnit/ServerCom/Messages.h"
#include <unordered_map>
#include "../NetworkUnit/ServerCom/TrackerDataStorage/TrackerDataStorage.h"

class TrackerMessageHandling
{
public:
    // Deleted methods to ensure a single instance
    TrackerMessageHandling(const TrackerMessageHandling &) = delete;
    TrackerMessageHandling &operator=(const TrackerMessageHandling &) = delete;

    // Access the single instance
    static TrackerMessageHandling &getInstance();

    ResultMessage handle(const ClientRequestUserList &request) const;
    ResultMessage handle(const ClientRequestStore &request) const;

private:
    // Constructor and Destructor are private
    TrackerMessageHandling();

    TrackerDataStorage &trackerDataStorage;

    // Static instance
    static std::unique_ptr<TrackerMessageHandling> instance;
    static std::once_flag initInstanceFlag;
};

#endif