//
// Created by user on 12/18/24.
//
#pragma once

#include "../../ICEConnection/ICEConnection.h"
#include "../../NetworkUnit/SocketHandler/SocketHandler.h"


// technically the same as addres but im leaving it under a different name in case we want to add more things...
struct PeerTorrent
{
    Address address;


    bool operator==(const PeerTorrent &other) const
    {
        return this->address == other.address;
    }

};
class ConnectionManager {

};


