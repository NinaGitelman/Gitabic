#pragma once

#include <algorithm>
#include <unordered_set>
#include "../../ICEConnection/ICEConnection.h"
#include "../FileHandler/TorrentFileHandler.h"
#include "../../NetworkUnit/ServerComm/TCPSocket/TCPSocket.h"
#include <unordered_map>

// technically the same as addres but im leaving it under a different name in case we want to add more things...

using PeerID = ID; // to be pretty :)
using FileID = ID; // to be pretty :)

using std::unordered_map;
using std::unordered_set;

class PeersConnectionManager
{
public:
    // If the peer already exists, add the fileID

    /// @brief Function adds the given fileID to the connected peer list
    /// if the peer alreadt exists, adds only the file ID to its list
    /// if not, creates its ice connectionsa dn the vector of fileIds with the file id for it
    /// @param peer  the peerID
    /// @param fileID the fileID of the file to get from it
    void addFileForPeer(FileID fileID, PeerID& peer);


    /// @brief function removes given file form given peer
    /// if there is only this file, it will disconnect the peer
    /// @param fileID the fileId to remove
    /// @param peer the peer to remove from
    void removeFileFromPeer(FileID fileID, PeerID& peer);

    void sendMessage(PeerID& peer, MessageBaseToSend* message);

    static PeersConnectionManager& getInstance(std::shared_ptr<TCPSocket> socket = nullptr);

private:
    explicit PeersConnectionManager(std::shared_ptr<TCPSocket> socket = nullptr);
    static mutex mutexInstance;
    static std::unique_ptr<PeersConnectionManager> instance;

    // Delete copy and assignment to ensure singleton
    PeersConnectionManager(const PeersConnectionManager&) = delete;
    PeersConnectionManager& operator=(const PeersConnectionManager&) = delete;

    std::shared_ptr<TCPSocket> _serverSocket;

    std::mutex _mutexRegisteredPeersFiles;
    unordered_map<PeerID, unordered_set<FileID>> _registeredPeersFiles;

    std::mutex _mutexPeerConnections;
    unordered_map<PeerID, std::shared_ptr<ICEConnection>> _peerConnections;

    unordered_map<FileID, TorrentFileHandler> _fileHandlers;
};
