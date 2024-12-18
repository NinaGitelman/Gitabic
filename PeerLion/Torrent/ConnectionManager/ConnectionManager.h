//
// Created by user on 12/18/24.
//
#pragma once

#include "../../ICEConnection/ICEConnection.h"
#include "../FileHandler/TorrentFileHandler.h"
#include <unordered_map>

using PeerTorrent = ID; // to be pretty :)
using FileID = ID; // to be pretty :)

using std::unordered_map;

class ConnectionManager
{
  public:

	 static ConnectionManager &getInstance();
     ~ConnectionManager();

     void addPeer(PeerTorrent& peer, FileID fileID);

     void removePeer(PeerTorrent& peer, FileID fileID);

     void sendMessage(PeerTorrent& peer, vector<uint8_t>& message);

  private:

	ConnectionManager();

	static mutex mutexInstance;
    static std::unique_ptr<ConnectionManager> instance;

	unordered_map<PeerTorrent, vector<FileID>> _registeredPeersFiles;
	unordered_map<PeerTorrent, ICEConnection> _peerConnections;
	unordered_map<FileID, TorrentFileHandler> _fileHandlers;


};


