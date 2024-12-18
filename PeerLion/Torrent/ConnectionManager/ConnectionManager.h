//
// Created by user on 12/18/24.
//
#pragma once

#include "../../ICEConnection/ICEConnection.h"


// technically the same as addres but im leaving it under a different name in case we want to add more things...
struct PeerTorrent
{
    ID id;


    bool operator==(const PeerTorrent &other) const
    {
      bool isEqual = false;
      // this works as the arrays of the id are of equal size
      if(this->id.size() == other.id.size())
      {
          isEqual =  this->id == other.id;
      }

      return isEqual;
    }

};
class ConnectionManager {

};


