#include <iostream>
#include "NetworkUnit/ServerComm/DataRepublish/DataRepublish.h"
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

int main()
{
  Address serverAdd = Address("0.0.0.0", 4789);
  TCPSocket socket = TCPSocket(serverAdd);

  ServerResponseNewId newId(socket.receive([](uint8_t code)
                                           { return code == ServerResponseCodes::NewID; }));

  ID id = newId.id;
  ServerResponseNewId otherNewId(socket.receive([](uint8_t code)
                                                { return code == ServerResponseCodes::NewID; }));

  ID otherId = otherNewId.id;

  std::function<bool(uint8_t)> isRelevant = [](uint8_t code)
  {
    return code == ServerResponseCodes::UserAuthorizedICEData;
  };

  std::string message = "Hello world!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
  vector<uint8_t> iceTestData(message.begin(), message.end());

  ClientRequestGetUserICEInfo requestIce = ClientRequestGetUserICEInfo(otherId, iceTestData);
  socket.sendRequest(requestIce);

  ServerResponseUserAuthorizedICEData response = socket.receive(isRelevant);
  printDataAsASCII(response.iceCandidateInfo);

  // int connect = 1;

  // if (argc > 2)
  // {
  //   connect = std::stoi(argv[1]);
  // }
  // else
  // {
  //   connect = 0;
  // }

  // // First handler negotiation
  // ICEConnection handler1(connect);
  // std::vector<uint8_t> data1 = handler1.getLocalICEData();
  // VectorUint8Utils::printVectorUint8(data1);
  // std::cout << "\n\n\n";
  // std::vector<uint8_t> remoteData1 = VectorUint8Utils::readFromCin();

  // try
  // {
  //   handler1.connectToPeer(remoteData1);
  // }
  // catch (const std::exception &e)
  // {
  //   std::cout << e.what() << " in main.cpp";
  // }

  // return EXIT_SUCCESS;
}
