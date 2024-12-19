//
// Created by user on 12/19/24.
//

#include "PeersConnectionManager.h"

PeersConnectionManager& PeersConnectionManager::getInstance(std::unique_ptr<TCPSocket> socket)
{
    std::lock_guard<std::mutex> lock(mutexInstance);

    if (!instance)
    {
        instance = std::unique_ptr<PeersConnectionManager>(
            new PeersConnectionManager(std::move(socket))
        );
    } else if (socket)
    {
        // Optionally handle attempts to reinitialize with a different socket
        throw std::runtime_error("PeersConnectionManager already initialized");
    }
    return *instance;
}

PeersConnectionManager::PeersConnectionManager(std::unique_ptr<TCPSocket> socket): _serverSocket(std::move(socket))

{


}

void PeersConnectionManager::addPeer(PeerTorrent& peer, FileID fileID)
{
    // handle p2p connection
    // first check if i am connected already from before
    ICEConnection iceConnection(1);

}


void PeersConnectionManager::removePeer(PeerTorrent& peer, FileID fileID)
{

}

void PeersConnectionManager::sendMessage(PeerTorrent& peer, vector<uint8_t>& message)
{

}
