//
// Created by user on 12/19/24.
//
/// TODO - change add file for peers and remove file from peer to use the peer's own mutex...
#include "PeersConnectionManager.h"

#include "../TorrentManager/TorrentManager.h"

std::mutex PeersConnectionManager::mutexInstance;
std::unique_ptr<PeersConnectionManager> PeersConnectionManager::instance;

PeersConnectionManager &PeersConnectionManager::getInstance(std::shared_ptr<TCPSocket> socket) {
    std::lock_guard<std::mutex> lock(mutexInstance);

    if (!instance) {
        instance = std::unique_ptr<PeersConnectionManager>(
            new PeersConnectionManager(socket)
        );
    }
    return *instance;
}

PeersConnectionManager::PeersConnectionManager(std::shared_ptr<TCPSocket> socket): _serverSocket(socket) {
    _isRunning = std::make_shared<std::atomic<bool> >(true);

    _routePacketThread = std::thread(&PeersConnectionManager::routePackets, this);
    _shareIceDataThread = std::thread(&PeersConnectionManager::shareIceData, this);

    _routePacketThread.detach();
    _shareIceDataThread.detach();
}

PeersConnectionManager::~PeersConnectionManager() {
    if (_isRunning) {
        *_isRunning = false; // Signal thread to stop
    }
    if (_routePacketThread.joinable()) {
        _routePacketThread.join(); // Wait for thread to finish
    }
    if (_shareIceDataThread.joinable()) {
        _shareIceDataThread.join();
    }

    // not really necessary but just tp be safe///
    std::lock_guard<std::mutex> lock(_mutexPeerConnections);

    // disconnect every connection when closing
    for (const auto &[peerID, connectionAndMutex]: _peerConnections) {
        connectionAndMutex.connection->disconnect();
    }
}


// for debugging...
static void printDataAsASCII(vector<uint8_t> data) {
    for (const auto &byte: data) {
        if (std::isprint(byte)) {
            std::cout << static_cast<char>(byte); // Printable characters
        } else {
            std::cout << '.'; // Replace non-printable characters with '.'
        }
    }
    std::cout << std::endl;
}

//a
// TODO divide this into smaller functions maybe...
// TODO - this gotta be a thread because it can block the whole program
bool PeersConnectionManager::addFileForPeer(const FileID &fileID, const PeerID &peer) {
    bool addedFile = false;
    std::cout << "Debug Peers Connection Manager add peer" << std::endl;

    std::unique_lock<std::mutex> peersConectionLock(_mutexPeerConnections);

    // first check if i am connected already from before
    if (_peerConnections.find(peer) == _peerConnections.end()) {
        ThreadSafeCout::cout("PeerManager: Adding peer " + SHA256::hashToString(peer) + "\n");

        // create ice connection and send to server the ice data
        std::shared_ptr<ICEConnection> peerConnection = std::make_shared<ICEConnection>(true);

        // gets local ice data and sends to server on the request
        std::vector<uint8_t> myIceData = peerConnection->getLocalICEData();
        ClientRequestGetUserICEInfo requestIce = ClientRequestGetUserICEInfo(peer, myIceData); {
            //   std::unique_lock<std::mutex> serverSocketLock(_mutexServerSocket);
            _serverSocket->sendRequest(requestIce);
        }
        std::cout << "send request of get my ice data\n\n";
        const std::function<bool(uint8_t)> isRelevant = [](const uint8_t code) {
            return code >= ServerResponseCodes::UserAuthorizedICEData && code <=
                   ServerResponseCodes::ExistsOpositeRequest;
        };

        // send request to server to connect to the other peer and for its ice data


        // std::unique_lock<std::mutex> serverSocketLock(_mutexServerSocket);
        MessageBaseReceived response = _serverSocket->receive(isRelevant);
        //     serverSocketLock.unlock();

        if (response.code == ServerResponseCodes::ExistsOpositeRequest) {
            return false;
        }
        if (response.code == ServerResponseCodes::UserAlreadyConnected) {
            // TODO URI CHECK - someone gotta catch this error and not just throw it
            throw std::runtime_error("Tried to connect to an already connected peer");
        }
        if (response.code == ServerResponseCodes::UserFullCapacity) {
            return false;
        }

        std::cout << "peer data: ";

        auto userIceData = ServerResponseUserAuthorizedICEData(response);
        printDataAsASCII(userIceData.iceCandidateInfo);
        // create thread that will add the peer

        bool connected = peerConnection->connectToPeer(userIceData.iceCandidateInfo);

        if (connected) {
            addedFile = connected;
            std::cout << "SUCESSFULLY CONNECTED to peer in add file for peer" << std::endl;

            _peerConnections[peer] = PeerConnectionAndMutex(peerConnection);

            peersConectionLock.unlock(); {
                // create registered peer files for this peer
                std::unique_lock<std::mutex> lock(_mutexRegisteredPeersFiles);

                _registeredPeersFiles.insert(std::pair<PeerID, unordered_set<FileID> >(peer, unordered_set{fileID}));
            }
        } else {
            std::cout << "failed connecting to peer in add file for peer" << std::endl;
            addedFile = false;
        }
    }
    // if the file is not present in the peer files list, add it
    else if (!_registeredPeersFiles[peer].contains(fileID)) {
        ThreadSafeCout::cout("PeerManager: Adding file for peer " + SHA256::hashToString(fileID) + "\n");

        _registeredPeersFiles[peer].insert(fileID);
        addedFile = true;
    }
    return addedFile;
}


void PeersConnectionManager::removeFileFromPeer(const FileID fileID, const PeerID &peer) {
    std::unique_lock<std::mutex> registeredPeersLock(_mutexRegisteredPeersFiles);

    auto itPeerFiles = _registeredPeersFiles.find(peer);

    if (itPeerFiles != _registeredPeersFiles.end()) // if finds the peer
    {
        auto itFileIdInSet = itPeerFiles->second.find(fileID);

        if (itFileIdInSet != itPeerFiles->second.end()) // if finds the fileId in the peer files set
        {
            if (itPeerFiles->second.size() == 1) // the only file id left -> disconnect
            {
                _registeredPeersFiles.erase(itPeerFiles);
                registeredPeersLock.unlock(); {
                    std::unique_lock<std::mutex> lock(_mutexRegisteredPeersFiles);

                    // delete the connection and the peer
                    auto itPeerConnection = _peerConnections.find(peer);
                    if (itPeerConnection != _peerConnections.end()) {
                        // disconnect the ice connection
                        (itPeerConnection->second).connection->disconnect();

                        // take it out from the map


                        _peerConnections.erase(itPeerConnection);
                    }
                }
            } else // simply delete this file id
            {
                itPeerFiles->second.erase(itFileIdInSet);
            }
        } else {
            throw std::runtime_error("File id not in peer");
        }
    } else {
        throw std::runtime_error("Peer not found");
    }
}

void PeersConnectionManager::sendMessage(const PeerID &peer, const std::shared_ptr<MessageBaseToSend> &message) {
    std::unique_lock<std::mutex> registeredPeersLock(_mutexPeerConnections);

    const auto itPeerConnection = _peerConnections.find(peer);
    registeredPeersLock.unlock();

    std::unique_lock<std::mutex> lock((itPeerConnection->second).mutex);

    if (itPeerConnection != _peerConnections.end()) // if finds the peer
    {
        if (itPeerConnection->second.connection->isConnected()) {
            (itPeerConnection->second).connection->sendMessage(message);
        } else {
            lock.unlock();
            registeredPeersLock.lock();
            _peerConnections.erase(itPeerConnection);
            throw std::runtime_error("Peer disconnected");
        }
    } else {
        throw std::runtime_error("Peer not found");
    }
}

void PeersConnectionManager::sendMessage(const vector<PeerID> &peers,
                                         const std::shared_ptr<MessageBaseToSend> &message) {
    for (PeerID peer: peers) {
        sendMessage(peer, message);
    }
}

void PeersConnectionManager::broadcast(const std::shared_ptr<MessageBaseToSend> &message) {
    // Create a local copy of peer IDs to avoid holding the lock during sendMessage
    std::vector<PeerID> peerIDs; {
        // Lock only to access the shared data
        std::unique_lock<std::mutex> registeredPeersLock(_mutexPeerConnections);
        for (const auto &peer: _registeredPeersFiles) {
            peerIDs.push_back(peer.first); // Collect PeerIDs
        }
    } // Mutex is released here

    // Send messages without holding the lock
    for (const auto &peerID: peerIDs) {
        sendMessage(peerID, message);
    }
}


void PeersConnectionManager::routePackets() {
    while (_isRunning->load()) {
        std::unique_lock<std::mutex> peersConnectionsLock(_mutexPeerConnections);
        for (auto currPeer = _peerConnections.begin(); currPeer != _peerConnections.end(); ++currPeer) {
            std::unique_lock<std::mutex> currPeerLock(currPeer->second.mutex);
            const int messagesCount = currPeer->second.connection->receivedMessagesCount();

            int currMessage = 0;
            for (currMessage = 0; currMessage < messagesCount; currMessage++) {
                MessageBaseReceived msg = currPeer->second.connection->receiveMessage();
                msg.from = currPeer->first;
                handleMessage(msg);
            }
        }
        peersConnectionsLock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void PeersConnectionManager::shareIceData() {
    while (_isRunning->load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        MessageBaseReceived req; {
            // Scoped lock for server socket receive
            //  std::lock_guard<std::mutex> serverLock(_mutexServerSocket);
            req = _serverSocket->receive([](const uint8_t code) {
                return code == ServerRequestCodes::AuthorizeICEConnection;
            });
        }

        // Use a single lock for checking and modifying peer connections
        std::lock_guard<std::mutex> peersConnectionsLock(_mutexPeerConnections);

        const auto authIceReq = ServerRequestAuthorizeICEConnection(req);

        if (_peerConnections.contains(authIceReq.from)) {
            // Scoped lock for sending response
            //    std::lock_guard<std::mutex> serverLock(_mutexServerSocket);
            _serverSocket->sendRequest(ClientResponseAlreadyConnected(authIceReq.requestId));
            continue;
        }

        if (_peerConnections.size() > MAXIMUM_CONNECTED_USERS) {
            // Scoped lock for sending response
            //       std::lock_guard<std::mutex> serverLock(_mutexServerSocket);
            _serverSocket->sendRequest(ClientResponseFullCapacity(authIceReq.requestId));
            continue;
        }

        const auto peerConnection = std::make_shared<ICEConnection>(false);
        const auto response = ClientResponseAuthorizedICEConnection(peerConnection->getLocalICEData(),
                                                                    authIceReq.requestId); {
            // Scoped lock for sending response
            //     std::lock_guard<std::mutex> serverLock(_mutexServerSocket);
            _serverSocket->sendRequest(response);
        }

        if (peerConnection->connectToPeer(authIceReq.iceCandidateInfo)) {
            ThreadSafeCout::cout("SUCCESSFULLY CONNECTED to peer in share ice data\n");
            // Lock already held from earlier
            _peerConnections[authIceReq.from] = PeerConnectionAndMutex(peerConnection);
        }
    }
}

// TODO - check if can leave as default or if there will be other message typess
void PeersConnectionManager::handleMessage(MessageBaseReceived &message) {
    switch (message.code) {
        case DEBUGGING_STRING_MESSAGE: {
            const DebuggingStringMessageReceived recvMessage(message);
            ThreadSafeCout::cout("Peers Connection Manager received: " + recvMessage.data + "\n\n");
            g_message(recvMessage.data.c_str());
            break;
        }
        case CODE_NO_MESSAGES_RECEIVED:
            break;
        default:
            TorrentManager::getInstance().handleMessage(message);
            break;
    }
}

bool PeersConnectionManager::isConnected(const PeerID &peer) {
    std::unique_lock<std::mutex> peersConnectionsLock(_mutexPeerConnections);
    return _peerConnections.contains(peer) && _peerConnections[peer].connection->isConnected();
}
