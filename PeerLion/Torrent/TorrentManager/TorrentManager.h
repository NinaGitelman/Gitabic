
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
    unordered_map<FileID, std::shared_ptr<TorrentFileHandler>> fileHandlers;

public:
};



#endif //TORRENTMANAGER_H
