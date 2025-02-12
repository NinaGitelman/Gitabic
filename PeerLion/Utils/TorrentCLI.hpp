#include <iostream>
#include <string>
#include <thread>
#include <iomanip>
#include <sstream>
#include "../Torrent/TorrentManager/TorrentManager.h"
#include "../Torrent/FileIO/FileIO.h"
#include <cstdlib>

enum class MenuChoice {
    ListFiles = static_cast<int>('1'),
    StartAll,
    StopAll,
    AddFileToDownload,
    AddFileToSeed,
    stopFile,
    startFile,
    Exit
};

class TorrentCLI {
private:
    TorrentManager &_manager;
    bool _running = true;
    std::mutex _display_mutex;
    uint8_t _n;

    [[nodiscard]] uint8_t chooseFile(const vector<FileIO> &files) {
        if (files.empty()) {
            std::cout << "No files available.\n";
            return 0;
        }

        displayFiles();

        int fileIndex;
        std::cout << "Enter file number: ";
        std::cin >> fileIndex;
        std::cin.ignore();

        if (fileIndex < 1 || fileIndex > files.size()) {
            std::cout << "Invalid file number.\n";
            return 0;
        }

        return fileIndex;
    }

    static std::string getProgressBar(double percentage, int width = 30) {
        int pos = width * percentage;
        std::string bar = "[";
        for (int i = 0; i < width; i++) {
            if (i < pos) bar += "=";
            else if (i == pos) bar += ">";
            else bar += " ";
        }
        bar += "]";
        return bar;
    }

    static std::string formatFileSize(uint64_t bytes) {
        const char *suffix[] = {"B", "KB", "MB", "GB", "TB"};
        int i = 0;
        double size = bytes;
        while (size >= 1024 && i < 4) {
            size /= 1024;
            i++;
        }
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << size << " " << suffix[i];
        return ss.str();
    }

    void displayFiles() {
        std::lock_guard<std::mutex> lock(_display_mutex);
        std::cout << "\n=== Current Files ===\n";

        auto files = _manager.getFilesIOs();
        if (files.empty()) {
            std::cout << "No files in the system.\n";
            return;
        }

        int index = 1;
        for (auto &file: files) {
            auto &progress = file.getDownloadProgress();
            std::cout << index++ << ". " << file.getFileName() << "\n";
            std::cout << "   Hash: " << SHA256::hashToString(progress.getFileHash()) << "\n";
            std::cout << "   Size: " << formatFileSize(progress.getFileSize()) << "\n";
            std::cout << "   Progress: " << getProgressBar(progress.progress())
                    << " " << std::fixed << std::setprecision(1)
                    << (progress.progress() * 100) << "%\n";
            std::cout << "   Mode: ";
            switch (file.getMode()) {
                case FileMode::Download: std::cout << "Downloading";
                    break;
                case FileMode::Seed: std::cout << "Seeding";
                    break;
                case FileMode::Hybrid: std::cout << "Downloading & Seeding";
                    break;
                case FileMode::Default: std::cout << "Idle";
                    break;
            }
            std::cout << "\n\n";
        }
    }

    void handleAddFile() const {
        std::string path;
        std::cout << "Enter the path to the metadata file: ";
        std::getline(std::cin, path);

        try {
            MetaDataFile metadata(path);
            FileIO fileIO(metadata, _n);
            _manager.addNewFileHandler(fileIO);
            std::cout << "File added successfully!\n";
        } catch (const std::exception &e) {
            std::cout << "Error adding file: " << e.what() << "\n";
        }
    }

    void addFileToSeed() const {
        std::string path, creator, password;
        std::cout << "Enter the path to the metadata file: ";
        std::getline(std::cin, path);

        std::cout << "Enter the creator: ";
        std::getline(std::cin, creator);

        std::cout << "Enter the password (empty for public file): ";
        std::getline(std::cin, password);

        std::cout << "Enter the signaling IP: ";
        std::string signalingIP;
        std::string signalingPort;
        std::getline(std::cin, signalingIP);
        std::cout << "Enter the signaling port: ";
        std::getline(std::cin, signalingPort);
        const auto signalingAddress = Address(signalingIP, std::stoi(signalingPort));


        MetaDataFile metadata = MetaDataFile::createMetaData(path, password, signalingAddress, creator);

        FileIO fileIO(metadata, _n, true);
        Utils::FileUtils::writeVectorToFile(Utils::FileUtils::readFileToVector(path),
                                            Utils::FileUtils::getExpandedPath(
                                                "~/Gitabic" + (_n ? std::to_string(_n) : "") + "/.filesFolders/" +
                                                SHA256::hashToString(metadata.getFileHash()) + "/" +
                                                metadata.getFileName()));
        _manager.addNewFileHandler(fileIO);
    }

    void handlestopFile() {
        auto files = _manager.getFilesIOs();
        if (files.empty()) {
            std::cout << "No files available.\n";
            return;
        }

        displayFiles();

        int fileIndex;
        std::cout << "Enter file number to remove: ";
        std::cin >> fileIndex;
        std::cin.ignore();

        if (fileIndex < 1 || fileIndex > files.size()) {
            std::cout << "Invalid file number.\n";
            return;
        }

        try {
            _manager.stopFileHandler(files[fileIndex - 1].getDownloadProgress().getFileHash());
            std::cout << "File removed successfully!\n";
        } catch (const std::exception &e) {
            std::cout << "Error removing file: " << e.what() << "\n";
        }
    }

    void clearScreen() {
        // Set the TERM environment variable if it's not already set
        if (!std::getenv("TERM")) {
            setenv("TERM", "xterm", 1); // Set TERM to "xterm" (a common terminal type)
        }

        // Clear the screen
        system("clear");
    }

public:
    explicit TorrentCLI(TorrentManager &mgr, const uint8_t n) : _manager(mgr), _n(n) {
        auto fileIOs = FileIO::getAllFileIO(n);
        _manager.start(fileIOs, false);
    }

    void run() {
        std::cout << "Welcome to BitTorrent CLI\n";

        while (_running) {
            ThreadSafeCout::print << "\nPress Enter to continue...";
            std::cin.get(); // Wait for Enter key
            clearScreen();
            std::cout << "\n=== Main Menu ===\n"
                    << "1. List files\n"
                    << "2. Start all files\n"
                    << "3. Stop all files\n"
                    << "4. Add file to download\n"
                    << "5. Add file to seed\n"
                    << "6. Stop file\n"
                    << "7. Start file\n"
                    << "8. Exit\n"
                    << "Choice: ";

            std::string choice;
            std::getline(std::cin, choice);

            if (choice.empty()) {
                continue;
            }

            switch (static_cast<int>(choice[0])) {
                case static_cast<int>(MenuChoice::ListFiles):
                    displayFiles();
                    break;
                case static_cast<int>(MenuChoice::StartAll): {
                    auto fileIOs = FileIO::getAllFileIO();
                    _manager.start(fileIOs, true);
                    break;
                }
                case static_cast<int>(MenuChoice::StopAll):
                    _manager.stopAll();
                    break;
                case static_cast<int>(MenuChoice::AddFileToDownload):
                    handleAddFile();
                    break;
                case static_cast<int>(MenuChoice::AddFileToSeed):
                    addFileToSeed();
                    break;
                case static_cast<int>(MenuChoice::stopFile):
                    handlestopFile();
                    break;
                case static_cast<int>(MenuChoice::startFile): {
                    auto files = _manager.getFilesIOs();
                    const auto fileIndex = chooseFile(files);
                    if (fileIndex) {
                        _manager.addNewFileHandler(files[fileIndex - 1]);
                    }
                    break;
                }
                case static_cast<int>(MenuChoice::Exit):
                    _running = false;
                    break;
                default:
                    std::cout << "Invalid choice.\n";
                    break;
            }
        }
    }
};
