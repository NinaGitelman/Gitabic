
#ifndef TORRENTMANAGER_H
#define TORRENTMANAGER_H

#include "../TorrentFileHandler/TorrentFileHandler.h"
#include "../PeersConnectionManager/PeersConnectionManager.h"
#include "../FileIO/FileIO.h"
#include <mutex>

class TorrentFileHandler;
class PeersConnectionManager;

using std::mutex;

// file handler and mutex to manage the file handlers and mutex S
struct FileHandlerAndMutex {
    std::shared_ptr<TorrentFileHandler> fileHandler;
    mutable std::mutex mutex;

    FileHandlerAndMutex(std::shared_ptr<TorrentFileHandler> fileHandler)
        : fileHandler(std::move(fileHandler)) {
    }

    // Add move constructor
    FileHandlerAndMutex(FileHandlerAndMutex &&other) noexcept
        : fileHandler(std::move(other.fileHandler))
          , mutex() {
    } // mutex is default constructed as it can't be moved

    // Delete copy constructor and assignment
    FileHandlerAndMutex(const FileHandlerAndMutex &) = delete;

    FileHandlerAndMutex &operator=(const FileHandlerAndMutex &) = delete;

    // Add move assignment if needed
    FileHandlerAndMutex &operator=(FileHandlerAndMutex &&other) noexcept {
        if (this != &other) {
            fileHandler = std::move(other.fileHandler);
            // mutex stays as is - can't be moved
        }
        return *this;
    }
};


// class to manage all the torrent files (TorrentFileHandler)
class TorrentManager {
private:
    // Singleton - Delete copy and assignment to ensure singleton
    TorrentManager(const TorrentManager &) = delete;

    explicit TorrentManager(std::shared_ptr<TCPSocket> socket = nullptr);

    TorrentManager &operator=(const TorrentManager &) = delete;

    static mutex mutexInstance;
    static std::unique_ptr<TorrentManager> instance;

    mutex _mutexServerSocket;
    std::shared_ptr<TCPSocket> _serverSocket;

    std::mutex _mutexFileHandlers;
    unordered_map<FileID, std::shared_ptr<FileHandlerAndMutex> > fileHandlers;

public:
    // singleton
    static TorrentManager &getInstance(std::shared_ptr<TCPSocket> socket = nullptr);

    /// @brief function to add a new file handler - add new file to the torrent system
    /// @param fileIO the fileIO of the file to add
    /// @throw runtime_error if the file id is already in the system
    void addNewFileHandler(FileIO &fileIO);

    /// @brief function to remove file from the torrent system
    /// @param fileID the fileIO of the file to add
    /// @throw runtime_error if the file id is already in the system
    void removeFileHandler(const FileID &fileID);


    void handleMessage(MessageBaseReceived &message);
};


#endif //TORRENTMANAGER_H
