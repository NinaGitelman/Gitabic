//
// Created by user on 1/4/25.
//

#include "TorrentManager.h"

#include <utility>


std::mutex TorrentManager::mutexInstance;
std::unique_ptr<TorrentManager> TorrentManager::instance;

TorrentManager& TorrentManager::getInstance(std::shared_ptr<TCPSocket> socket)
{
    std::lock_guard<std::mutex> lock(mutexInstance);

    if (!instance)
    {
        instance = std::unique_ptr<TorrentManager>();
    }
    else if (socket)
    {
        // Optionally handle attempts to reinitialize with a different socket
        throw std::runtime_error("PeersConnectionManager already initialized");
    }

    return *instance;
}
TorrentManager::TorrentManager(std::shared_ptr<TCPSocket> socket): _serverSocket(std::move(socket))
{}


/// TODO - change empty aes key later...
void TorrentManager::addNewFileHandler(FileIO& fileIO)
{
    FileID fileID = fileIO.getDownloadProgress().get_file_hash();

    std::unique_lock<std::mutex> lockFileHandlers(_mutexFileHandlers);
    // Check if fileId already exists
    if (fileHandlers.find(fileID) == fileHandlers.end())
    {
        // Create a new TorrentFileHandler wrapped in a shared_ptr
        std::unique_lock<std::mutex> lockServerSock(_mutexFileHandlers);
        auto torrentFileHandler = std::make_shared<TorrentFileHandler>(fileIO, _serverSocket, AESKey());
        lockServerSock.unlock();

        // Create a FileHandlerAndMutex object
        auto fileHandlerAndMutex = std::make_shared<FileHandlerAndMutex>(torrentFileHandler);

        // Insert into the unordered_map
        fileHandlers[fileID] = fileHandlerAndMutex;
    }
    else
        {
        throw std::runtime_error("FileHandler for this fileID already exists");
    }
}
