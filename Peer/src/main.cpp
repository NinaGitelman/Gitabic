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
#include "Utils/VectorUint8Utils.h"
#include <iostream>
#include <unistd.h>
#include "NetworkUnit/ServerComm/TCPSocket/TCPSocket.h"
#include "NetworkUnit/SocketHandler/SocketHandler.h"
#include "NetworkUnit/ServerComm/Messages.h"

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

void main1()
{
  Address serverAdd = Address("0.0.0.0", 4789);
  TCPSocket socket = TCPSocket(serverAdd);

  ServerResponseNewId newId(socket.receive([](uint8_t code)
                                           { return code == ServerResponseCodes::NewID; }));

  ID id = newId.id;
  // ServerResponseNewId otherNewId(socket.receive([](uint8_t code)
  //                                               { return code == ServerResponseCodes::NewID; }));

  // ID otherId = otherNewId.id;

  DataRepublish &dataRepublish = DataRepublish::getInstance(&socket);
  dataRepublish.saveData(ID(), id);
  std::this_thread::sleep_for(std::chrono::seconds(60));
  std::function<bool(uint8_t)> isRelevant = [](uint8_t code)
  {
    return code == ServerResponseCodes::UserAuthorizedICEData;
  };

  std::string message = "Hello world!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
  vector<uint8_t> iceTestData(message.begin(), message.end());

  // ClientRequestGetUserICEInfo requestIce = ClientRequestGetUserICEInfo(otherId, iceTestData);
  // socket.sendRequest(requestIce);

  // ServerResponseUserAuthorizedICEData response = socket.receive(isRelevant);
  // printDataAsASCII(response.iceCandidateInfo);
}
void main2()
{
  Address serverAdd = Address("0.0.0.0", 4789);
  TCPSocket socket = TCPSocket(serverAdd);

  ServerResponseNewId newId(socket.receive([](uint8_t code)
                                           { return code == ServerResponseCodes::NewID; }));

  ID id = newId.id;

  ServerRequestAuthorizeICEConnection authIceReq(socket.receive([](uint8_t code)
                                                                { return code == ServerRequestCodes::AuthorizeICEConnection; }));

  std::cout << "Received!!\n";
  printDataAsASCII(authIceReq.iceCandidateInfo);

  std::string message = "123456 654321!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
  vector<uint8_t> iceTestData(message.begin(), message.end());

  ClientResponseAuthorizedICEConnection iceResponse = ClientResponseAuthorizedICEConnection(iceTestData, authIceReq.requestId);
  socket.sendRequest(iceResponse);
  std::cout << "Done!!\n";
}

int main(int argc, char **argv)
{
  short option = -1;
  if (argc == 2)
  {
    option = std::stoi(argv[1]);
  }
  if (option < 1 || option > 2)
  {
    std::cout << "Enter an option (1 or 2): ";
    std::cin >> option;

    // Validate input
    while (std::cin.fail() || (option != 1 && option != 2))
    {
      std::cin.clear();                                                   // Clear the error flag
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Ignore invalid input
      std::cout << "Invalid input. Please enter 1 or 2: ";
      std::cin >> option;
    }
  }
  switch (option)
  {
  case 1:
    main1();
    break;
  case 2:
    main2();
    break;
  }

  return 0;
}
