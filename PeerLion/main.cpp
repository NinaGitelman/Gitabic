// int main(int argc, char* argv[])

// {
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
#include "Torrent/PeersConnectionManager/PeersConnectionManager.h"
#include "Torrent/TorrentManager/TorrentManager.h"
#include "Utils/MetaDataFile/MetaDataFile.h"
#include "Utils/VectorUint8Utils.h"
#include <csignal>
#include <iostream>
#include <csignal>
#include <iostream>
#include "Utils/TorrentCLI.hpp"

// #define SERVER_ADDRESS "16.171.4.75"
#define SERVER_ADDRESS "0.0.0.0"
#define SERVER_PORT 4787

// TODO - talk about - the singletons dont cleanup because the constructors are not called. check this
// TODO - after a while it keeps connected - add smthing to torren tmanager so it ill check who hasnt talked to us in a while
void signalHandler(int signal) {
  std::cout << "Signal caught: " << signal << ". Cleaning up..." << std::endl;
  // Perform cleanup here or ensure objects are destroyed properly
  exit(signal);
}
//a
/// @brief  Heper function to pritn the DATA
void printDataAsASCII(vector<uint8_t> data) {
  for (const auto &byte: data) {
    if (std::isprint(byte)) {
      std::cout << static_cast<char>(byte); // Printable characters
    } else {
      std::cout << '.'; // Replace non-printable characters with '.'
    }
  }
  std::cout << std::endl;
}

void fakeProgress(const double percentage, FileIO &file_io) {
  for (uint i = 0; i < file_io.getDownloadProgress().getAmmountOfPieces() * percentage; i++) {
    file_io.getDownloadProgress().updatePieceStatus(i, DownloadStatus::Verified);
  }
  file_io.saveProgressToFile();
}

void main2(uint8_t n);



int main(const int argc, char **argv) {
  //signal(SIGINT, signalHandler);
  //signal(SIGTERM, signalHandler);

  std::string serverAddress = SERVER_ADDRESS;  // Default to hardcoded address
  if (argc > 1) {
    serverAddress = argv[1];  // Use address passed as argument if available
  }

  TorrentCLI cli(TorrentManager::getInstance(std::make_shared<TCPSocket>(Address(serverAddress, SERVER_PORT))),
                 argc > 2 ? atoi(argv[2]) : 0);
  cli.run();

  return 0;  // End after CLI is done
}
//
// int main(const int argc, char **argv) {
//   signal(SIGINT, signalHandler);
//   signal(SIGTERM, signalHandler);
//   TorrentCLI cli(TorrentManager::getInstance(std::make_shared<TCPSocket>(Address(SERVER_ADDRESS, SERVER_PORT))),
//                  argc > 1 ? atoi(argv[1]) : 0);
//   cli.run();
//   return 0;
//   if (argc == 2) {
//     main2(atoi(argv[1]));
//     return 0;
//   }
//   string filePath = getenv("HOME");
//
//   auto fileIO = FileIO::getAllFileIO()[0];

  // auto serverSocket = std::make_shared<TCPSocket>(fileIO.getDownloadProgress().getSignalingAddress());
//   auto serverSocket = std::make_shared<TCPSocket>(Address(SERVER_ADDRESS, SERVER_PORT));
//   auto res = serverSocket->receive([](const unsigned char code) { return code == ServerResponseCodes::NewID; });
//   auto &dataRepublish = DataRepublish::getInstance(serverSocket);
//   dataRepublish.saveData(fileIO.getDownloadProgress().getFileHash(), ServerResponseNewId(res).id);
//
//   TorrentManager::getInstance(serverSocket).addNewFileHandler(fileIO);
//
//   while (true) {
//     string input;
//     std::getline(std::cin, input);
//     if (input == "exit") {
//       break;
//     }
//   }
// }
//
// void main2(const uint8_t n) {
//   auto fileIO = FileIO("9e3d83b20958de1c578693a2df03f13869f7e4027a6d3bb166ba3e51a1b671f7", n);
//
//   const auto serverSocket = std::make_shared<TCPSocket>(Address(SERVER_ADDRESS, SERVER_PORT));
//   const auto res = serverSocket->receive([](const unsigned char code) { return code == ServerResponseCodes::NewID; });
//   auto &dataRepublish = DataRepublish::getInstance(serverSocket);
//   dataRepublish.saveData(fileIO.getDownloadProgress().getFileHash(), ServerResponseNewId(res).id);
//
//   TorrentManager &instanceTorrentManager = TorrentManager::getInstance(serverSocket);
//
//   std::cout << "in main" << std::endl;
//   instanceTorrentManager.addNewFileHandler(fileIO);
//
//   while (true) {
//     string input;
//     std::getline(std::cin, input);
//     if (input == "exit") {
//       break;
//     }
//   }
// }
//

// int main(int argc, char *argv[]) {
//   //   int connect = 1;
//   //  // First handler negotiation
//   //   ICEConnection handler1(connect);
//   //   std::vector<uint8_t> data1 = handler1.getLocalICEData();
//   //   VectorUint8Utils::printVectorUint8(data1);
//   //   std::cout << "\n\n\n";
//   //   std::vector<uint8_t> remoteData1 = VectorUint8Utils::readFromCin();
//
//   //   try
//   //   {
//   //     handler1.connectToPeer(remoteData1);
//   //   }
//   //   catch (const std::exception &e)
//   //   {
//   //     std::cout << e.what() << " in main.cpp";
//   //   }
//
//
//   // working example with server for 2 diferent peers
//
//   int connect = 1;
//   Address serverAdd = Address("0.0.0.0", 4787);
//   std::shared_ptr<TCPSocket> serverSocket = std::make_shared<TCPSocket>(serverAdd);
//   PeersConnectionManager &connectionManager = PeersConnectionManager::getInstance(serverSocket);
//
//
//   HashResult fileID = SHA256::toHashSha256(vector<uint8_t>(10));
//
//   ServerResponseNewId newId(serverSocket->receive([](uint8_t code) {
//     return code == ServerResponseCodes::NewID;
//   }));
//
//
//   ID id = newId.id;
//
//
//   try {
//     const string filePath = "/media/user/OS/LinuxExtraSpace/gitabic/emek-yizrael-1701-gitabic/unnamed.png";
//     MetaDataFile file = MetaDataFile::createMetaData(filePath, "1234", serverAdd, "nina");
//     const MetaDataFile &md1(file);
//     FileIO fileIO = FileIO(md1);
//     //  FileIO fileIO2= FileIO(mdFile2);
//
//
//     TorrentManager &instanceTorrentManager = TorrentManager::getInstance(serverSocket);
//     instanceTorrentManager.addNewFileHandler(fileIO);
//     //instanceTorrentManager.addNewFileHandler(fileIO2);
//
//     //  instanceTorrentManager.removeFileHandler(fileIO2.getDownloadProgress().get_file_hash());
//     instanceTorrentManager.removeFileHandler(fileIO.getDownloadProgress().getFileHash());
//   } catch (std::exception &e) {
//     std::cout << e.what() << std::endl;
//   }
//
//
//   if (argc >= 2) // if there is cmd arg then this will start the connection
//   {
//     ServerResponseNewId otherNewId(serverSocket->
//     receive([](uint8_t code) {
//       return code == ServerResponseCodes::NewID;
//     }));
//
//     ID otherId = otherNewId.id;
//
//     std::cout << "This peer is starting the connection request\n\n";
//
//
//     // connection start
//     // by now using this just to check for if added the file
//     bool connected = connectionManager.addFileForPeer(fileID, otherId);
//     sleep(1);
//     try {
//       DebuggingStringMessageToSend debuggingStringMessage = DebuggingStringMessageToSend(
//         "Hello world from connection manager");
//       DebuggingStringMessageToSend debuggingStringMessage2 = DebuggingStringMessageToSend(
//         "Hello world from connection manager");
//       DebuggingStringMessageToSend debuggingStringMessage3 = DebuggingStringMessageToSend(
//         "Hello world from connection manager");
//
//       connectionManager.sendMessage(otherId, &debuggingStringMessage);
//       connectionManager.sendMessage(otherId, &debuggingStringMessage);
//       connectionManager.sendMessage(otherId, &debuggingStringMessage);
//       sleep(60);
//     } catch (const std::exception &e) {
//       std::cout << e.what() << " in main.cpp";
//     }
//   } else {
//     std::cout << "This peer is receiveing the connection request\n\n";
//
//     connect = 0;
//
//     ServerRequestAuthorizeICEConnection authIceReq(serverSocket->receive([](uint8_t code) {
//       return code == ServerRequestCodes::AuthorizeICEConnection;
//     }));
//
//     std::cout << "Received connection request from another peer: \n";
//     printDataAsASCII(authIceReq.iceCandidateInfo);
//     std::cout << "\n\n\n";
//
//     // get my ice data
//     // First handler negotiation
//     ICEConnection handler1(connect);
//     std::vector<uint8_t> myIceData = handler1.getLocalICEData();
//     std::cout << "my ice data: ";
//     printDataAsASCII(myIceData);
//     ClientResponseAuthorizedICEConnection connectionResponse(myIceData, authIceReq.requestId);
//     serverSocket->sendRequest(connectionResponse);
//     try {
//       std::thread peerThread([&handler1, authIceReq]() {
//         handler1.connectToPeer(authIceReq.iceCandidateInfo);
//       });
//       peerThread.detach();
//
//       /// TODO the problem is that it isnt sending the message content right...
//       DebuggingStringMessageToSend debuggingStringMessage = DebuggingStringMessageToSend("Hello world from main");
//       DebuggingStringMessageToSend debuggingStringMessage2 = DebuggingStringMessageToSend("Hello world from main");
//       DebuggingStringMessageToSend debuggingStringMessage3 = DebuggingStringMessageToSend("Hello world from main");
//       handler1.sendMessage(&debuggingStringMessage);
//       handler1.sendMessage(&debuggingStringMessage2);
//       handler1.sendMessage(&debuggingStringMessage3);
//       // handler1.sendMessage(debuggingStringMessage);
//       while (true) {
//         while (handler1.receivedMessagesCount() > 0) {
//           MessageBaseReceived newMessage = handler1.receiveMessage();
//           if (newMessage.code == ClientRequestCodesToServer::DebuggingStringMessage) {
//             DebuggingStringMessageReceived recvMessage = DebuggingStringMessageReceived(newMessage);
//             recvMessage.printDataAsASCII();
//           }
//         }
//       }
//     } catch (const std::exception &e) {
//       std::cout << e.what() << " in main.cpp";
//     }
//   }
//
//
//   return EXIT_SUCCESS;
// }

//   //   int connect = 1;
//   //  // First handler negotiation
//   //   ICEConnection handler1(connect);
//   //   std::vector<uint8_t> data1 = handler1.getLocalICEData();
//   //   VectorUint8Utils::printVectorUint8(data1);
//   //   std::cout << "\n\n\n";
//   //   std::vector<uint8_t> remoteData1 = VectorUint8Utils::readFromCin();
//
//   //   try
//   //   {
//   //     handler1.connectToPeer(remoteData1);
//   //   }
//   //   catch (const std::exception &e)
//   //   {
//   //     std::cout << e.what() << " in main.cpp";
//   //   }
//
//   // working example with server for 2 diferent peers
//
//   int connect = 1;
//   Address serverAdd = Address("3.90.184.187", 4787);
//   TCPSocket socket = TCPSocket(serverAdd);
//
//   ServerResponseNewId newId(socket.receive([](uint8_t code)
//   {
//     return code == ServerResponseCodes::NewID;
//   }));
//
//   ID id = newId.id;
//
//   if (argc >= 2) // if there is cmd arg then this will start the connection
//   {
//     ServerResponseNewId otherNewId(socket.receive([](uint8_t code)
//     {
//       return code == ServerResponseCodes::NewID;
//     }));
//
//     ID otherId = otherNewId.id;
//
//     std::function<bool(uint8_t)> isRelevant = [](uint8_t code)
//     {
//       return code == ServerResponseCodes::UserAuthorizedICEData;
//     };
//
//     std::cout << "This peer is starting the connection request\n\n - getting my ice data";
//
//     // get my ice data
//     // First handler negotiation
//     ICEConnection handler1(connect);
//     std::vector<uint8_t> myIceData = handler1.getLocalICEData();
//
//     std::cout << "sending to server my ice data\n";
//     VectorUint8Utils::printVectorUint8(myIceData);
//     std::cout << "\n\n";
//
//     ClientRequestGetUserICEInfo requestIce = ClientRequestGetUserICEInfo(otherId, myIceData);
//     socket.sendRequest(requestIce);
//
//     ServerResponseUserAuthorizedICEData response = socket.receive(isRelevant);
//
//     std::cout << "second peer ice data received from server: \n";
//     printDataAsASCII(response.iceCandidateInfo);
//     std::cout << "\n\n";
//
//     try
//     {
//       std::thread peerThread([&handler1, &response]()
//       {
//         handler1.connectToPeer(response.iceCandidateInfo);
//       });
//       peerThread.detach();
//
//       DebuggingStringMessageToSend debuggingStringMessage = DebuggingStringMessageToSend("Hello world from terminal");
//       DebuggingStringMessageToSend debuggingStringMessage2 = DebuggingStringMessageToSend("Hello world from terminal");
//       DebuggingStringMessageToSend debuggingStringMessage3 = DebuggingStringMessageToSend("Hello world from terminal");
//       handler1.sendMessage(&debuggingStringMessage);
//       handler1.sendMessage(&debuggingStringMessage2);
//       handler1.sendMessage(&debuggingStringMessage3);
//
//       int count = 0;
//       while (count < 3)
//       {
//         sleep(0.2);
//         while (handler1.receivedMessagesCount() > 0)
//         {
//           MessageBaseReceived newMessage = handler1.receiveMessage();
//           if (newMessage.code == ClientRequestCodes::DebuggingStringMessage)
//           {
//             count++;
//             DebuggingStringMessageReceived recvMessage = DebuggingStringMessageReceived(newMessage);
//             recvMessage.printDataAsASCII();
//           }
//         }
//       }
//     }
//     catch (const std::exception& e)
//     {
//       std::cout << e.what() << " in main.cpp";
//     }
//   }
//   else
//   {
//     std::cout << "This peer is receiveing the connection request\n\n";
//
//     connect = 0;
//
//     ServerRequestAuthorizeICEConnection authIceReq(socket.receive([](uint8_t code)
//     {
//       return code == ServerRequestCodes::AuthorizeICEConnection;
//     }));
//
//     std::cout << "Received connection request from another peer: \n";
//     printDataAsASCII(authIceReq.iceCandidateInfo);
//     std::cout << "\n\n\n";
//
//     // get my ice data
//     // First handler negotiation
//     ICEConnection handler1(connect);
//     std::vector<uint8_t> myIceData = handler1.getLocalICEData();
//
//     ClientResponseAuthorizedICEConnection connectionResponse(myIceData, authIceReq.requestId);
//     socket.sendRequest(connectionResponse);
//     try
//     {
//       std::thread peerThread([&handler1, &authIceReq]()
//       {
//         handler1.connectToPeer(authIceReq.iceCandidateInfo);
//       });
//       peerThread.detach();
//
//       /// TODO the problem is that it isnt sending the message content right...
//       DebuggingStringMessageToSend debuggingStringMessage = DebuggingStringMessageToSend("Hello world from clion");
//       DebuggingStringMessageToSend debuggingStringMessage2 = DebuggingStringMessageToSend("Hello world from clion");
//       DebuggingStringMessageToSend debuggingStringMessage3 = DebuggingStringMessageToSend("Hello world from clion");
//       handler1.sendMessage(&debuggingStringMessage);
//       handler1.sendMessage(&debuggingStringMessage2);
//       handler1.sendMessage(&debuggingStringMessage3);
//       // handler1.sendMessage(debuggingStringMessage);
//       while (true)
//       {
//         sleep(1);
//         while (handler1.receivedMessagesCount() > 0)
//         {
//           MessageBaseReceived newMessage = handler1.receiveMessage();
//           if (newMessage.code == ClientRequestCodes::DebuggingStringMessage)
//           {
//             DebuggingStringMessageReceived recvMessage = DebuggingStringMessageReceived(newMessage);
//             recvMessage.printDataAsASCII();
//           }
//         }
//       }
//     }
//     catch (const std::exception& e)
//     {
//       std::cout << e.what() << " in main.cpp";
//     }
//   }
//
//
//   return EXIT_SUCCESS;
// }
