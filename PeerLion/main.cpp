#include <iostream>
#include "NetworkUnit/ServerComm/DataRepublish/DataRepublish.h"
#include "NetworkUnit/ServerComm/Messages.h"
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
  Address serverAdd = Address("3.80.37.75", 4787);
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

    std::cout << "This peer is starting the connection request\n\n - getting my ice data";

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
      std::thread peerThread([&handler1, &response]()
                             { handler1.connectToPeer(response.iceCandidateInfo); });
      peerThread.detach();

      DebuggingStringMessageToSend debuggingStringMessage = DebuggingStringMessageToSend("Hello world from terminal");
      DebuggingStringMessageToSend debuggingStringMessage2 = DebuggingStringMessageToSend("Hello world from terminal");
      DebuggingStringMessageToSend debuggingStringMessage3 = DebuggingStringMessageToSend("Hello world from terminal");
      handler1.sendMessage(&debuggingStringMessage);
      handler1.sendMessage(&debuggingStringMessage2);
      handler1.sendMessage(&debuggingStringMessage3);

      int count =0;
      while (count <3)
      {
        sleep(0.2);
        while (handler1.receivedMessagesCount() > 0)
        {
           MessageBaseReceived newMessage = handler1.receiveMessage();
          if (newMessage.code == ClientRequestCodes::DebuggingStringMessage)
          {
            count++;
            DebuggingStringMessageReceived recvMessage = DebuggingStringMessageReceived(newMessage);
            recvMessage.printDataAsASCII();
          }
        }
      }
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
      std::thread peerThread([&handler1, &authIceReq]()
                             { handler1.connectToPeer(authIceReq.iceCandidateInfo); });
      peerThread.detach();

     /// TODO the problem is that it isnt sending the message content right...
      DebuggingStringMessageToSend debuggingStringMessage = DebuggingStringMessageToSend("Hello world from clion");
      DebuggingStringMessageToSend debuggingStringMessage2 = DebuggingStringMessageToSend("Hello world from clion");
      DebuggingStringMessageToSend debuggingStringMessage3 = DebuggingStringMessageToSend("Hello world from clion");
      handler1.sendMessage(&debuggingStringMessage);
      handler1.sendMessage(&debuggingStringMessage2);
      handler1.sendMessage(&debuggingStringMessage3);
     // handler1.sendMessage(debuggingStringMessage);
      while (true)
      {
        sleep(1);
        while (handler1.receivedMessagesCount() > 0)
        {

          MessageBaseReceived newMessage = handler1.receiveMessage();
          if (newMessage.code == ClientRequestCodes::DebuggingStringMessage)
          {
            DebuggingStringMessageReceived recvMessage = DebuggingStringMessageReceived(newMessage);
            recvMessage.printDataAsASCII();

          }
        }
      }
    }
    catch (const std::exception &e)
    {
      std::cout << e.what() << " in main.cpp";
    }
  }


  return EXIT_SUCCESS;
}
