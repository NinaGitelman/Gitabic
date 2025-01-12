#pragma once

#include <algorithm>
#include <unordered_set>
#include "../../ICEConnection/ICEConnection.h"
#include "../../NetworkUnit/ServerComm/TCPSocket/TCPSocket.h"
#include <unordered_map>
#include "../../Utils/ThreadSafeCout.h"


// technically the same as addres but im leaving it under a different name in case we want to add more things...

using PeerID = ID; // to be pretty :)
using FileID = ID; // to be pretty :)

using std::unordered_map;
using std::unordered_set;


// peer connection to manage this peers mutex and the ice connection
struct PeerConnectionAndMutex {
    std::shared_ptr<ICEConnection> connection;
    std::mutex mutex;

    PeerConnectionAndMutex(std::shared_ptr<ICEConnection> connection)
        : connection(std::move(connection)) {
    }

    // Add move constructor
    PeerConnectionAndMutex(PeerConnectionAndMutex &&other) noexcept
        : connection(std::move(other.connection))
          , mutex() {
    } // mutex is default constructed as it can't be moved

    // Delete copy constructor and assignment
    PeerConnectionAndMutex(const PeerConnectionAndMutex &) = delete;

    PeerConnectionAndMutex &operator=(const PeerConnectionAndMutex &) = delete;

    // Add move assignment if needed
    PeerConnectionAndMutex &operator=(PeerConnectionAndMutex &&other) noexcept {
        if (this != &other) {
            connection = std::move(other.connection);
            // mutex stays as is - can't be moved
        }
        return *this;
    }
};

class PeersConnectionManager {
public:
    // Singleton
    static PeersConnectionManager &getInstance(std::shared_ptr<TCPSocket> socket = nullptr);
    virtual ~PeersConnectionManager();

    /// @brief Function adds the given fileID to the connected peer list
    /// if the peer alreadt exists, adds only the file ID to its list
    /// if not, creates its ice connectionsa dn the vector of fileIds with the file id for it
    /// @param peer  the peerID
    /// @param fileID the fileID of the file to get from it
    bool addFileForPeer(const FileID& fileID,const PeerID &peer);


    /// @brief function removes given file form given peer
    /// if there is only this file, it will disconnect the peer
    /// @param fileID the fileId to remove
    /// @param peer the peer to remove from
    void removeFileFromPeer(const FileID fileID,const PeerID &peer);


    // @brief Function to send the given message to the given peer
    /// @param peer the peer to send message to
    /// @param message the message
    void sendMessage(const PeerID &peer, MessageBaseToSend *message);

    /// @brief funciton to send a broadcast (message to all users) of the given message
    /// @param message the message to broadcast
    void broadcast (MessageBaseToSend *message);

    // Function to check if the given peer is connected
    bool isConnected(const PeerID& peer);

private:
    std::shared_ptr<atomic<bool> > _isRunning; // bool to track if is connected to the other peer
    std::thread _routePacketThread; // Store the thread as a member

    static mutex mutexInstance;
    static std::unique_ptr<PeersConnectionManager> instance;

    /// TODO-later check if needs to add a mutex for this
    std::mutex _mutexServerSocket;
    std::shared_ptr<TCPSocket> _serverSocket;

    std::mutex _mutexRegisteredPeersFiles;
    unordered_map<PeerID, unordered_set<FileID> > _registeredPeersFiles;

    std::mutex _mutexPeerConnections;
    unordered_map<PeerID, PeerConnectionAndMutex> _peerConnections;

    // Delete copy and assignment to ensure singleton
    PeersConnectionManager(const PeersConnectionManager &) = delete;
    explicit PeersConnectionManager(std::shared_ptr<TCPSocket> socket = nullptr);
    PeersConnectionManager &operator=(const PeersConnectionManager &) = delete;

    /// TODO
    // thread that will be called from the constructor
    void routePackets(std::shared_ptr<atomic<bool> > isRunning);
    void handleMessage(MessageBaseReceived &message);



};
