
#pragma once
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <vector>
#include <stdint.h>
#include "../Utils/ThreadSafeCout.h"
#include "../../c/LibNice/LibNice.h"
#include "../Utils/VectorUint8Utils.h"

#define COMPONENT_ID_RTP 1


#define STUN_PORT 3478
#define STUN_ADDR "stun.stunprotocol.org" // TODO - make it a list and do many tries in case this does not work (out of service../)

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
class LibNiceHandler
{

private:
    NiceAgent *_agent; // the ncie agent that will be used for this connection
    const gchar *_stunAddr = STUN_ADDR;
    const guint _stunPort = STUN_PORT;
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
    

    int addRemoteCandidates(char* remoteData);

    NiceCandidate *parseCandidate(char *scand);



public:

    // constexpr meeans constant expression
    // used because this variables will be computed at compile time instead of run time, it helps program run faster and use less memory.
    static constexpr const gchar* candidate_type_name[4] = {"host", "srflx", "prflx", "relay"};
    static constexpr const gchar* state_name[4] = {"disconnected", "gathering", "connecting", "connected"};



    /// @brief constructor that intializes the object
    /// @param isControlling if the object was created because someone is requesting to talk to the peer (0)
    ///                      or if the peer wants to talk to someone (1)
    LibNiceHandler(const bool isControlling);
    
    /// @brief DDestructor - cleans up
    ~LibNiceHandler();
    
    /// @brief Function to get the local ice data - deals with all of the candiadtes gatheirng and everythign needed
    /// @return a vector<uint8_t> with the local ice data
    vector<uint8_t> getLocalICEData();


    int connectToPeer(const vector<uint8_t> remoteIceData);

   
    
};