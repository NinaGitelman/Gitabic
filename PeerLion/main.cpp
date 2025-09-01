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

//#define SERVER_ADDRESS "3.93.173.123"
 #define SERVER_ADDRESS "0.0.0.0"
#define SERVER_PORT 4787

void signalHandler(int signal) {
  std::cout << "Signal caught: " << signal << ". Cleaning up..." << std::endl;
  // Perform cleanup here or ensure objects are destroyed properly
  exit(signal);
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
