//
// Created by user on 1/4/25.
//

#include "TorrentManager.h"


std::mutex TorrentManager::mutexInstance;
std::unique_ptr<TorrentManager> TorrentManager::instance;

TorrentManager& TorrentManager::getInstance()
{
    std::lock_guard<std::mutex> lock(mutexInstance);

    if (!instance)
    {
        instance = std::unique_ptr<TorrentManager>();
    }

    return *instance;
}
