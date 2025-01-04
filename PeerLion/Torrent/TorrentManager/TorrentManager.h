
#ifndef TORRENTMANAGER_H
#define TORRENTMANAGER_H

#include "../TorrentFileHandler/TorrentFileHandler.h"
#include "../PeersConnectionManager/PeersConnectionManager.h"

class TorrentFileHandler;
class PeersConnectionManager;


// class to manage all the torrent files (TorrentFileHandler)
class TorrentManager
{
private:
    // Singleton - Delete copy and assignment to ensure singleton
    TorrentManager(const TorrentManager &) = delete;
    explicit TorrentManager();
    TorrentManager &operator=(const TorrentManager &) = delete;

    static mutex mutexInstance;
    static std::unique_ptr<TorrentManager> instance;



    unordered_map<FileID, std::shared_ptr<TorrentFileHandler>> fileHandlers;

public:
    // singleton
    static TorrentManager &getInstance();



};



#endif //TORRENTMANAGER_H
