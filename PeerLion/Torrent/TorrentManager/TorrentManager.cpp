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

void TorrentManager::removeFileHandler(const FileID& fileID)
{
    std::unique_lock<std::mutex> lockFileHandlers(_mutexFileHandlers);
    // Check if fileId already exists
    auto fileHandlerIt = fileHandlers.find(fileID);
    if (fileHandlerIt != fileHandlers.end())
    {
        // stop the file handler's processes
        (fileHandlerIt->second)->fileHandler->stop();

        fileHandlers.erase(fileHandlerIt);
    }
    else
    {
        throw std::runtime_error("FileHandler for this fileID does not exist");
    }
}

void TorrentManager::handleMessage(MessageBaseReceived& message)
{
    TorrentMessageBase torrentMessage(message, message.code);

    std::unique_lock<std::mutex> lockFileHandlers(_mutexFileHandlers);
    auto fileHandlerIt = fileHandlers.find(torrentMessage.fileId);
    if (fileHandlerIt != fileHandlers.end())
    {
        fileHandlerIt->second->mutex.lock();
        /// TODO - this isnt the smartest thing as we already make it a TorrentMessgeBase but it made a mess to do differently...
        fileHandlerIt->second->fileHandler->addMessage(message);
    }
    else
    {
        throw std::runtime_error("FileHandler for this fileID does not exist");
    }
}
