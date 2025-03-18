//
// Created by user on 1/4/25.
//

#include "TorrentManager.h"
#include <ranges>
#include <utility>


std::mutex TorrentManager::mutexInstance;
std::unique_ptr<TorrentManager> TorrentManager::instance;


TorrentManager &TorrentManager::getInstance(std::shared_ptr<TCPSocket> socket) {
    std::lock_guard<std::mutex> lock(mutexInstance);

    if (!instance) {
        // Create a new TorrentManager instance
        instance = std::unique_ptr<TorrentManager>(new TorrentManager(socket));
    }
    return *instance;
}

TorrentManager::TorrentManager(std::shared_ptr<TCPSocket> &socket): _serverSocket(std::move(socket)) {
    const auto res = _serverSocket->receive([](const unsigned char code) {
        return code == ServerResponseCodes::NewID;
    });
    _id = ServerResponseNewId(res).id;
}


/// TODO - change empty aes key later...
void TorrentManager::addNewFileHandler(FileIO &fileIO, bool autoStart) {
    const FileID fileID = fileIO.getDownloadProgress().getFileHash();
    // tnis line throws seg fault: the next one
    std::unique_lock<std::mutex> lockFileHandlers(_mutexFileHandlers);
    // Check if fileId already exists
    if (!fileHandlers.contains(fileID)) {
        auto torrentFileHandler = std::make_shared<TorrentFileHandler>(fileIO, _serverSocket, AESKey(), _id, autoStart);


        // Create a FileHandlerAndMutex object
        auto fileHandlerAndMutex = std::make_shared<FileHandlerAndMutex>(torrentFileHandler);

        // Insert into the unordered_map
        fileHandlers[fileID] = fileHandlerAndMutex;
    } else {
        fileHandlers[fileID]->fileHandler->resume();
    }
}

void TorrentManager::stopFileHandler(const FileID &fileID) {
    std::unique_lock<std::mutex> lockFileHandlers(_mutexFileHandlers);
    // Check if fileId already exists
    auto fileHandlerIt = fileHandlers.find(fileID);
    if (fileHandlerIt != fileHandlers.end()) {
        // stop the file handler's processes
        (fileHandlerIt->second)->fileHandler->stop();

        // fileHandlers.erase(fileHandlerIt);
    } else {
        throw std::runtime_error("FileHandler for this fileID does not exist");
    }
}

void TorrentManager::handleMessage(MessageBaseReceived &message) {
    TorrentMessageBase torrentMessage(message);
    //TODO nina check - for some reason comes with empty buffer, which result crash. maybe problem in ice

    std::unique_lock<std::mutex> lockFileHandlers(_mutexFileHandlers);
    auto fileHandlerIt = fileHandlers.find(torrentMessage.fileId);
    if (fileHandlerIt != fileHandlers.end()) {
        lockFileHandlers.unlock();
        std::lock_guard guard(fileHandlerIt->second->mutex);
        /// TODO - this isnt the smartest thing as we already make it a TorrentMessgeBase but it made a mess to do differently...
        fileHandlerIt->second->fileHandler->addMessage(message);
    } else {
        throw std::runtime_error("FileHandler for this fileID does not exist");
    }
}

void TorrentManager::start(vector<FileIO> &files, const bool autoStart) {
    for (auto &file: files) {
        addNewFileHandler(file, autoStart);
    }
}

void TorrentManager::stopAll() const {
    for (auto &handler: fileHandlers | std::ranges::views::values) {
        handler->fileHandler->stop();
    }
}

vector<FileIO> TorrentManager::getFilesIOs() {
    vector<FileIO> result;
    for (const auto &handler: fileHandlers | std::ranges::views::values) {
        result.push_back(handler->fileHandler->getFileIO());
    }
    return result;
}
