
#pragma once
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <vector>
#include <stdint.h>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <map>
#include "../Utils/ThreadSafeCout.h"
#include "../Utils/VectorUint8Utils.h"
#include "../NetworkUnit/ServerComm/Messages.h"

#define COMPONENT_ID_RTP 1

/// TURN configs
// the ip should not change - the last two times i checked it didnt change...
// if needed to check, do nslookup for: relay1.expressturn.com
// using free express turn....
// todo later - put this in a configs file in a safer way....
#define TURN_ADDR "23.26.133.136"
#define TURN_USERNAME "ef8X4GWHOIXIDE3M2R"
#define TURN_PASSWORD "rpKpXiK0tpIWNzOB"
#define TURN_PORT 3478

/// STUN configs
#define STUN_PORT 3478
#define STUN_ADDR "stun.stunprotocol.org" // TODO - make it a list and do many tries in case this does not work (out of service../)

using std::map;
using std::mutex;
using std::queue;
using std::vector;

extern "C"
{
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <agent.h>

#include <gio/gnetworking.h>
}

// OBS:
// There is an isConnected function. this is how we can know if its still conected or not. maybe we will need to check it every few minutes or something...

class ICEConnection
{
    // comment to change for commit
private:
    // glib is thread safe so no need for mutex for everything used in threads
    // variables needed for the nice connection
    NiceAgent *_agent; // the ncie agent that will be used for this connection
    const gchar *_stunAddr = STUN_ADDR;
    const guint _stunPort = STUN_PORT;

    gboolean _isControlling;          // if this conneciton is controllling or being controlled
    GMainContext *_context;           // this connections context
    GMainLoop *_gloop;                // a blocking loop only for this connection
    guint _streamId;                  // Add this
    bool _candidatesGathered = false; // bool to track if candidates were already gathered

    mutex _mutexReceivedMessages;                 // mutex to lock the received messages queue
    queue<MessageBaseReceived> _receivedMessages; // queue where all received messages will be

    mutex _mutexIsConnectedBool; // mutex for the is connected bool
    bool _isConnected;           // bool to set if its conencted and receiveing messages

    // turn constants
    const gchar *turnUsername = TURN_USERNAME;
    const gchar *turnPassword = TURN_PASSWORD;

    /// @brief Helper function to get the candidate data and put it into the given buffer
    /// @param localDataBuffer The buffer in which the localData will be put (empty null buffer, malloc will be dealt on this function)
    /// @return If it gathered the data right
    int
    getCandidateData(char **localDataBuffer);

    /// @brief Callbis ack to handle if the candidate gathering is done
    static void callbackCandidateGatheringDone(NiceAgent *agent, guint streamId, gpointer userData);

    /// @brief Helper function to add the candidates from the remote data
    /// @param remoteData the remote data received from the client (called from connect ot peer)
    /// @return If it was successful or not
    int addRemoteCandidates(char *remoteData);

    /// @brief Helper function to parse each candidate (called by addRemoteCandidates)
    /// @param scand The candidate
    /// @return A NiceCandidate object from it
    NiceCandidate *parseCandidate(char *scand);

    /// @brief Callback function to notify when a component state is changed -> only used for printing the state...
    //   has to have those inputs because libnice expects it  even if we only use one of them...
    static void callbackComponentStateChanged(NiceAgent *agent, guint streamId, guint componentId, guint state, gpointer data);

    /// @brief Callback that will handle the received messages and out it into the messages queue of the IceConnection object passed in the data
    /// @param data The ice connection that calls it
    static void callbackReceive(NiceAgent *agent, guint _stream_id, guint component_id, guint len, gchar *buf, gpointer data);

    /// @brief Function to transform received data as a vector into a MessageBaseReceived object
    /// @param messageData the message data
    /// @param len the message data length
    static MessageBaseReceived deserializeMessage(gchar *buf, guint len);

public:
    // constexpr means constant expression
    // The nice agent will be only one for the class and then for each object will add a stream
    // used because this variables will be computed at compile time instead of run time, it helps program run faster and use less memory.
    static constexpr const gchar *candidateTypeName[4] = {"host", "srflx", "prflx", "relay"};
    static constexpr const gchar *stateName[4] = {"disconnected", "gathering", "connecting", "connected"};

    /// OBS: the consructor throws exceptions so put it in a try catch when creating object...
    /// @brief constructor that intializes the object
    /// @param isControlling if the object was created because someone is requesting to talk to the peer (0)
    ///                      or if the peer wants to talk to someone (1)
    ICEConnection(const bool isControlling);

    /// @brief Destructor - cleans up
    ~ICEConnection();

    /// @brief Function to get the local ice data - deals with all of the candiadtes gatheirng and everythign needed
    /// @return a vector<uint8_t> with the local ice data
    vector<uint8_t> getLocalICEData();

    /// @brief Function to be used to connect to a peer, send the remote data received from the peer
    /// @param remoteData the ICE connection data
    /// @return if it was successful or not
    /// it is a thread so it gotta be called as a thread (because of main gloop)
    /*
       std::thread peerThread([&handler1, &authIceReq]()
                             { handler1.connectToPeer(authIceReq.iceCandidateInfo); });
      peerThread.detach();
    */
    int connectToPeer(const vector<uint8_t> &remoteData);

    /// @brief Thread safe function to check if the ice connection is still valid
    /// @return if the connection is still valid
    bool isConnected();

    /// @brief Function to get a received message
    /// @return a received message from the list or a message with code CODE_NO_MESSAGES_RECEIVED if there were no new received messages
    MessageBaseReceived receiveMessage();

    /// @brief Function gets the amount of messages in the received messages queue
    /// @return amount of received messages
    int receivedMessagesCount();
};