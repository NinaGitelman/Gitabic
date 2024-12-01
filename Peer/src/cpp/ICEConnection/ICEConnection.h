
#pragma once
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <vector>
#include <stdint.h>
#include <map>
#include "../Utils/ThreadSafeCout.h"
#include "../Utils/VectorUint8Utils.h"

#define COMPONENT_ID_RTP 1


#define STUN_PORT 3478
#define STUN_ADDR "stun.stunprotocol.org" // TODO - make it a list and do many tries in case this does not work (out of service../)

using std::vector;
using std::map;

extern "C"
{
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <agent.h>

#include <gio/gnetworking.h>

}
class ICEConnection
{
// comment to change for commit
private:

// static as we dont need many of it and will only need one (what changes for each is the streams)


    static NiceAgent *_agent; // the ncie agent that will be used for this connection
    const gchar *_stunAddr = STUN_ADDR;
    const guint _stunPort = STUN_PORT;

    // non static
    gboolean _isControlling;   // if this conneciton is controllling or being controlled
    GMainContext* _context;  // this connections context
    GMainLoop* _gloop; // a blocking loop only for this connection    
    guint _streamId;  // Add this
    bool _candidatesGathered = false; // bool to track if candidates were already gathered
    
    
    

    /// @brief Helper function to get the candidate data and put it into the given buffer
    /// @param localDataBuffer The buffer in which the localData will be put (empty null buffer, malloc will be dealt on this function)
    /// @return If it gathered the data right
    int getCandidateData(char **localDataBuffer);
    
    /// @brief Callbis ack to handle if the candidate gathering is done
    static void callbackCandidateGatheringDone(NiceAgent* agent, guint streamId, gpointer userData);
    
    /// @brief Helper function to add the candidates from the remote data
    /// @param remoteData the remote data received from the client (called from connect ot peer)
    /// @return If it was successful or not
    int addRemoteCandidates(char* remoteData);

    /// @brief Helper function to parse each candidate (called by addRemoteCandidates)
    /// @param scand The candidate
    /// @return A NiceCandidate object from it
    NiceCandidate *parseCandidate(char *scand);

    /// @brief Callback function to notify when a component state is changed -> only used for printing the state...
    //   has to have those inputs because libnice expects it  even if we only use one of them...
    static void callbackComponentStateChanged(NiceAgent *agent, guint streamId, guint componentId, guint state, gpointer data);


    /// @brief Calbback that will handle how the messages are received 
    /// @param data The gloop of the handler 
    static void callbackReceive(NiceAgent *agent, guint _stream_id, guint component_id, guint len, gchar *buf, gpointer data);

public:

    // constexpr means constant expression
    // used because this variables will be computed at compile time instead of run time, it helps program run faster and use less memory.
    static constexpr const gchar* candidateTypeName[4] = {"host", "srflx", "prflx", "relay"};
    static constexpr const gchar* stateName[4] = {"disconnected", "gathering", "connecting", "connected"};



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
    int connectToPeer(const vector<uint8_t>& remoteData);

   
    
};