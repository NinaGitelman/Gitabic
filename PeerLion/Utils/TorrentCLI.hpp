#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <iomanip>
#include <sstream>
#include "../Torrent/TorrentManager/TorrentManager.h"
#include "../Torrent/FileIO/FileIO.h"

enum class MenuChoice {
    ListFiles = (int)'1',
    StartAll,
    StopAll,
    AddFileToDownload,
    AddFileToSeed,
    RemoveFile,
    Exit
};

class TorrentCLI {


private:
    TorrentManager &manager;
    bool running = true;
    std::mutex display_mutex;

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
        std::lock_guard<std::mutex> lock(display_mutex);
        std::cout << "\n=== Current Files ===\n";

        auto files = manager.getFilesIOs();
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
            FileIO fileIO(metadata);
            manager.addNewFileHandler(fileIO);
            std::cout << "File added successfully!\n";
        } catch (const std::exception &e) {
            std::cout << "Error adding file: " << e.what() << "\n";
        }
    }

    void addFileToSeed(){
        std::string path, creator, password;
        Address signalingAddress;
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
        signalingAddress = Address(signalingIP, std::stoi(signalingPort));



        MetaDataFile metadata = MetaDataFile::createMetaData(path, password, signalingAddress, creator);

        FileIO fileIO(metadata);
        fileIO.setFileMode(FileMode::Seed);
        //TODO make progress automatically full
        manager.addNewFileHandler(fileIO);
    }

    void handleRemoveFile() {
        auto files = manager.getFilesIOs();
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
            manager.removeFileHandler(files[fileIndex - 1].getDownloadProgress().getFileHash());
            std::cout << "File removed successfully!\n";
        } catch (const std::exception &e) {
            std::cout << "Error removing file: " << e.what() << "\n";
        }
    }

public:
    explicit TorrentCLI(TorrentManager &mgr) : manager(mgr) {
    }

    void run() {
        std::cout << "Welcome to BitTorrent CLI\n";

        while (running) {
            std::cout << "\n=== Main Menu ===\n"
                    << "1. List all files\n"
                    << "2. Add new file\n"
                    << "3. Remove file\n"
                    << "4. Exit\n"
                    << "Choice: ";

            std::string choice;
            std::getline(std::cin, choice);


            switch ((int)choice[0]) {
              	case (int)MenuChoice::StartAll:
            {
                  auto fileIOs = FileIO::getAllFileIO();
                  manager.start(fileIOs);
                	break;
                }
				case (int)MenuChoice::StopAll:
					manager.stopAll();
                	break;
                case (int)MenuChoice::ListFiles:
                    displayFiles();
                    break;
                case (int)MenuChoice::AddFileToDownload:
                    handleAddFile();
                    break;
                case (int)MenuChoice::AddFileToSeed:
                  	addFileToSeed();
                    break;
                case (int)MenuChoice::RemoveFile:
                    handleRemoveFile();
                    break;
                case (int)MenuChoice::Exit:
                    running = false;
                    break;
                default:
                    std::cout << "Invalid choice.\n";
                    break;
            }
        }
    }
};
