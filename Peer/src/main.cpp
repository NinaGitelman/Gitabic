#include <iostream>
#include "NetworkUnit/ServerComm/DataRepublish/DataRepublish.h"
#include "../../Peer/src/NetworkUnit/ServerComm/Messages.h"
#include "Encryptions/SHA256/sha256.h"
#include "Encryptions/AES/AESHandler.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <vector>
#include <cstdint>
#include "ICEConnection/ICEConnection.h"
#include "Utils/VectorUint8Utils.h"

/// @brief  Heper function to pritn the DATA
void printDataAsASCII(vector<uint8_t> data)
{
  for (const auto &byte : data)
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

int main(int argc, char *argv[])
{
  //   int connect = 1;
  //  // First handler negotiation
  //   ICEConnection handler1(connect);
  //   std::vector<uint8_t> data1 = handler1.getLocalICEData();
  //   VectorUint8Utils::printVectorUint8(data1);
  //   std::cout << "\n\n\n";
  //   std::vector<uint8_t> remoteData1 = VectorUint8Utils::readFromCin();

  //   try
  //   {
  //     handler1.connectToPeer(remoteData1);
  //   }
  //   catch (const std::exception &e)
  //   {
  //     std::cout << e.what() << " in main.cpp";
  //   }

  // working example with server for 2 diferent peers

  int connect = 1;
  Address serverAdd = Address("3.87.119.17", 4789);
  TCPSocket socket = TCPSocket(serverAdd);

  ServerResponseNewId newId(socket.receive([](uint8_t code)
                                           { return code == ServerResponseCodes::NewID; }));

  ID id = newId.id;

  if (argc >= 2) // if there is cmd arg then this will start the connection
  {

    ServerResponseNewId otherNewId(socket.receive([](uint8_t code)
                                                  { return code == ServerResponseCodes::NewID; }));

    ID otherId = otherNewId.id;

    std::function<bool(uint8_t)> isRelevant = [](uint8_t code)
    {
      return code == ServerResponseCodes::UserAuthorizedICEData;
    };

    std::cout << "This peer is starting the connection request\n\n";

    // get my ice data
    // First handler negotiation
    ICEConnection handler1(connect);
    std::vector<uint8_t> myIceData = handler1.getLocalICEData();

    std::cout << "sending to server my ice data\n";
    VectorUint8Utils::printVectorUint8(myIceData);
    std::cout << "\n\n";

    ClientRequestGetUserICEInfo requestIce = ClientRequestGetUserICEInfo(otherId, myIceData);
    socket.sendRequest(requestIce);

    ServerResponseUserAuthorizedICEData response = socket.receive(isRelevant);

    std::cout << "second peer ice data received from server: \n";
    printDataAsASCII(response.iceCandidateInfo);
    std::cout << "\n\n";

    try
    {
      handler1.connectToPeer(response.iceCandidateInfo);
    }
    catch (const std::exception &e)
    {
      std::cout << e.what() << " in main.cpp";
    }
  }
  else
  {

    std::cout << "This peer is receiveing the connection request\n\n";

    connect = 0;

    ServerRequestAuthorizeICEConnection authIceReq(socket.receive([](uint8_t code)
                                                                  { return code == ServerRequestCodes::AuthorizeICEConnection; }));

    std::cout << "Received connection request from another peer: \n";
    printDataAsASCII(authIceReq.iceCandidateInfo);
    std::cout << "\n\n\n";

    // get my ice data
    // First handler negotiation
    ICEConnection handler1(connect);
    std::vector<uint8_t> myIceData = handler1.getLocalICEData();

    ClientResponseAuthorizedICEConnection connectionResponse(myIceData, authIceReq.requestId);
    socket.sendRequest(connectionResponse);
    try
    {
      handler1.connectToPeer(authIceReq.iceCandidateInfo);
    }
    catch (const std::exception &e)
    {
      std::cout << e.what() << " in main.cpp";
    }
  }

  return EXIT_SUCCESS;
}
