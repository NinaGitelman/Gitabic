#include "ConnectionManager.h"

ConnectionManager& ConnectionManager::getInstance()
{
    std::lock_guard<mutex> lock(mutexInstance);
    if (!instance)
    {
        instance = std::unique_ptr<ConnectionManager>(new ConnectionManager());
    }
    return *instance;
}

void ConnectionManager::addPeer(PeerTorrent& peer, FileID fileID)
{
    // handle p2p connection
    // first check if i am connected already from before
    ICEConnection iceConnection(1);

}


void ConnectionManager::removePeer(PeerTorrent& peer, FileID fileID)
{

}

void ConnectionManager::sendMessage(PeerTorrent& peer, vector<uint8_t>& message)
{

}
