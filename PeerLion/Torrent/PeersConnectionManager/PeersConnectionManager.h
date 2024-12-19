#pragma once

#include "../../ICEConnection/ICEConnection.h"
#include "../FileHandler/TorrentFileHandler.h"
#include "../../NetworkUnit/ServerComm/TCPSocket/TCPSocket.h"
#include <unordered_map>

// technically the same as addres but im leaving it under a different name in case we want to add more things...

using PeerTorrent = ID; // to be pretty :)
using FileID = ID; // to be pretty :)

using std::unordered_map;

class PeersConnectionManager
{
public:

    void addPeer(PeerTorrent& peer, FileID fileID);

    void removePeer(PeerTorrent& peer, FileID fileID);

    void sendMessage(PeerTorrent& peer, vector<uint8_t>& message);

    PeersConnectionManager& getInstance(std::unique_ptr<TCPSocket> socket = nullptr);

private:

    explicit PeersConnectionManager(std::unique_ptr<TCPSocket> socket = nullptr);

    static mutex mutexInstance;
    static std::unique_ptr<PeersConnectionManager> instance;

    // Delete copy and assignment to ensure singleton
    PeersConnectionManager(const PeersConnectionManager&) = delete;
    PeersConnectionManager& operator=(const PeersConnectionManager&) = delete;

    std::unique_ptr<TCPSocket> _serverSocket;

    unordered_map<PeerTorrent, vector<FileID>> _registeredPeersFiles;
    unordered_map<PeerTorrent, ICEConnection> _peerConnections;
    unordered_map<FileID, TorrentFileHandler> _fileHandlers;

};