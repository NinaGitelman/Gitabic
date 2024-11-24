//
// Created by user on 24.11.2024 
//
#pragma once
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define CONTROLLING 1
#define STUN_PORT 3478
#define STUN_ADDR "stun.stunprotocol.org" // TODO - make it a list and do many tries in case this does not work (out of service../)
extern "C"
{
#include "../../c/LibNice/LibNice.h"
 
}
class LibNiceHandler
{
    
    NiceAgent *_agent;
    const gchar *_stunAddr = STUN_ADDR;
    const guint _stunPort = STUN_PORT;
    gboolean _isControlling;   

    LibNiceHandler();

};