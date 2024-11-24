//
// Created by user on 24.11.2024 
//
#pragma once
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <vector>
#include "../Utils/ThreadSafeCout.h"
#define STUN_PORT 3478
#define STUN_ADDR "stun.stunprotocol.org" // TODO - make it a list and do many tries in case this does not work (out of service../)


extern "C"
{
#include "../../c/LibNice/LibNice.h"
 
}
class LibNiceHandler
{

private:
    NiceAgent *_agent;
    const gchar *_stunAddr = STUN_ADDR;
    const guint _stunPort = STUN_PORT;
    gboolean _isControlling;   

public:
    /// @brief constructor that intializes the object
    /// @param isControlling if the object was created because someone is requesting to talk to the peer (0)
                    ///     or if the peer wants to talk to someone (1)
    LibNiceHandler(bool isControlling);
    
};