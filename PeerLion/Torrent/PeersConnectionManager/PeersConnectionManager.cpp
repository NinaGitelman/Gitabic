//
// Created by user on 12/19/24.
//

#include "PeersConnectionManager.h"

std::mutex PeersConnectionManager::mutexInstance;
std::unique_ptr<PeersConnectionManager> PeersConnectionManager::instance;

PeersConnectionManager& PeersConnectionManager::getInstance(std::shared_ptr<TCPSocket> socket)
{
    std::lock_guard<std::mutex> lock(mutexInstance);

    if (!instance)
    {
        instance = std::unique_ptr<PeersConnectionManager>(
            new PeersConnectionManager(socket)
        );
    } else if (socket)
    {
        // Optionally handle attempts to reinitialize with a different socket
        throw std::runtime_error("PeersConnectionManager already initialized");
    }
    return *instance;
}

PeersConnectionManager::PeersConnectionManager(std::shared_ptr<TCPSocket> socket): _serverSocket(socket)
{
}

// for debugging...
static void printDataAsASCII(vector<uint8_t> data)
{
    for (const auto& byte : data)
    {
        if (std::isprint(byte))
        {
            std::cout << static_cast<char>(byte); // Printable characters
        }
        else
        {
            std::cout << '.'; // Replace non-printable characters with '.'
        }
    }
    std::cout << std::endl;
}

// TODO divide this into smaller functions maybe...
bool PeersConnectionManager::addFileForPeer(FileID fileID, PeerID& peer)
{
    bool addedFile = false;
    std::cout << "Debug Peers Connection Manager add peer" << std::endl;

    std::unique_lock<std::mutex> peersConectionLock(_mutexPeerConnections);

    // first check if i am connected already from before
    if(_peerConnections.find(peer) == _peerConnections.end())
    {
        // create ice connection and send to server the ice data
        std::shared_ptr<ICEConnection> peerConnection = std::make_shared<ICEConnection>(true);

        // gets local ice data and sends to server on the request
        std::vector<uint8_t> myIceData = peerConnection->getLocalICEData();
        ClientRequestGetUserICEInfo requestIce = ClientRequestGetUserICEInfo(peer, myIceData);
        _serverSocket->sendRequest(requestIce);

        std::cout << "send request of get my ice data\n\n";
        std::function<bool(uint8_t)> isRelevant = [](uint8_t code)
        {
            return code == ServerResponseCodes::UserAuthorizedICEData;
        };

        // send request to server to connect to the other peer and for its ice data
        ServerResponseUserAuthorizedICEData response = _serverSocket->receive(isRelevant);

        std::cout << "peer data: " ;
        printDataAsASCII(response.iceCandidateInfo);
        // create thread that will add the peer

        // TODO - problem with the thread. find way to know it is done even without the join and get return of okay or not if connected
        //  TODO - maybe change the thread to return something instead.... wait for is connected or sth...
        // TODO -  make wrapper to just call connect to peer and wait for response
        /// TODO - later sprint make it prettier
         bool connected = peerConnection->connectToPeer(response.iceCandidateInfo);

         if(connected)
         {
            addedFile = connected;
            std::cout << "SUCESSFULLY CONNECTED to peer in add file for peer" << std::endl;

             _peerConnections.insert(std::pair<PeerID,std::shared_ptr<ICEConnection>>(peer, std::move(peerConnection)));

            peersConectionLock.unlock();

            {
                // create registered peer files for this peer
                std::unique_lock<std::mutex> lock(_mutexRegisteredPeersFiles);

                _registeredPeersFiles.insert(std::pair<PeerID, unordered_set<FileID>>(peer, unordered_set{fileID}));
            }
        }
        else
        {
            std::cout << "failed connecting to peer in add file for peer" << std::endl;
            addedFile = false;
        }
    }
    // if the file is not present in the peer files list, add it
    else if(_registeredPeersFiles[peer].find(fileID) == _registeredPeersFiles[peer].end() )
    {
        _registeredPeersFiles[peer].insert(fileID);
        addedFile = true;
    }
    return addedFile;

}


void PeersConnectionManager::removeFileFromPeer( FileID fileID, PeerID& peer)
{
    std::unique_lock<std::mutex> registeredPeersLock(_mutexRegisteredPeersFiles);

    auto itPeerFiles = _registeredPeersFiles.find(peer);

    if (itPeerFiles  != _registeredPeersFiles.end() ) // if finds the peer
    {
        auto itFileIdInSet = itPeerFiles->second.find(fileID);

        if(itFileIdInSet != itPeerFiles->second.end()) // if finds the fileId in the peer files set
        {

            if(itPeerFiles->second.size() == 1 ) // the only file id left -> disconnect
            {
                 _registeredPeersFiles.erase(itPeerFiles);
                registeredPeersLock.unlock();

                    {
                        std::unique_lock<std::mutex> lock(_mutexRegisteredPeersFiles);

                        // delete the connection and the peer
                        auto itPeerConnection = _peerConnections.find(peer);
                        if(itPeerConnection != _peerConnections.end())
                        {
                          // disconnect the ice connection
                          (itPeerConnection->second)->disconnect();

                          // take it out from the map


                          _peerConnections.erase(itPeerConnection);
                    }
                }

            }
            else // simply delete this file id
            {
                itPeerFiles->second.erase(itFileIdInSet);

            }
        }
        else
        {
            throw std::runtime_error("File id not in peer");
        }
    }
    else
    {
        throw std::runtime_error("Peer not found");
    }
}

void PeersConnectionManager::sendMessage(PeerID& peer, MessageBaseToSend* message)
{
    std::unique_lock<std::mutex> registeredPeersLock(_mutexPeerConnections);

    auto itPeerConnection = _peerConnections.find(peer);

    if (itPeerConnection  != _peerConnections.end() ) // if finds the peer
    {
      (itPeerConnection->second)->sendMessage(message);
    }
    else
    {
         throw std::runtime_error("Peer not found");
    }
}

void routePackets()
{


}