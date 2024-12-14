//
// Created by user on 12/9/24.
//

#include "FileHandler.h"


#include "../ThreadSafeCout.h"
#include "../FileUtils/FileUtils.h"

using Utils::FileUtils;
using Utils::FileSplitter;

const std::string FileHandler::dirPath = FileHandler::getExpandedPath("~/Gitabic/.filesFolders/");

// Swap function definition
void swap(FileHandler &first, FileHandler &second) noexcept {
    using std::swap;
    swap(first.fileName, second.fileName);
    swap(first.mode, second.mode);
    swap(first.downloadProgress, second.downloadProgress);
    swap(first.pieceSize, second.pieceSize);
    // `std::mutex` is not swappable, so it remains as is for both objects
}

// Copy constructor
FileHandler::FileHandler(const FileHandler &other)
    : fileName(other.fileName),
      mode(other.mode),
      downloadProgress(other.downloadProgress),
      pieceSize(other.pieceSize) {
    // Note: `std::mutex` is not copied
}

// Move constructor
FileHandler::FileHandler(FileHandler &&other) noexcept
    : fileName(std::move(other.fileName)),
      mode(other.mode),
      downloadProgress(std::move(other.downloadProgress)),
      pieceSize(other.pieceSize) {
    // `std::mutex` remains default-initialized in the moved-from object
}

// Assignment operator (copy-and-swap)
FileHandler &FileHandler::operator=(FileHandler other) {
    swap(*this, other); // Reuse the swap function
    return *this;
}

size_t FileHandler::getOffset(const uint32_t pieceIndex, const uint16_t blockIndex) const {
    return pieceSize * pieceIndex + blockIndex * Utils::FileSplitter::BLOCK_SIZE;
}

std::string FileHandler::getExpandedPath(const std::string &path) {
    if (!path.empty() && path[0] == '~') {
        if (const char *home = std::getenv("HOME")) {
            return std::string(home) + path.substr(1); // Replace '~' with HOME
        }
    }
    return path; // Return unchanged if no '~' at the beginning
}

void FileHandler::initNew(const MetaDataFile &metaData) {
    const auto fileHash = string(metaData.getFileHash().begin(), metaData.getFileHash().end());
    auto path = FileUtils::createDownloadFolder(fileHash,
                                                metaData.getCreator() + " - " +
                                                std::filesystem::path(metaData.getFileName()).stem().string().append(
                                                    "/"));
    try {
        FileUtils::createFilePlaceHolder(path.append(metaData.getFileName()), metaData.getFileSize());
    } catch ([[maybe_unused]] const std::exception &e) {
        throw std::runtime_error("Not enough storage space to create file");
    }
    downloadProgress = metaData;
    FileUtils::writeVectorToFile(downloadProgress.serialize(),
                                 path.append(
                                     "." + string(metaData.getFileHash().begin(), metaData.getFileHash().end()) +
                                     ".gitabic"));
}

FileHandler::FileHandler(const string &hash)
    : mode(FileMode::Default),
      downloadProgress(DownloadProgress(FileUtils::readFileToVector(dirPath + "." + fileName + ".gitabic"))),
      pieceSize(FileSplitter::pieceSize(downloadProgress.get_file_size())) {
    fileName = downloadProgress.get_file_name();
}

FileHandler::FileHandler(const MetaDataFile &metaData) {
    initNew(metaData);
    this->mode = FileMode::Default;
    fileName = downloadProgress.get_file_name();
    pieceSize = FileSplitter::pieceSize(downloadProgress.get_file_size());
}

void FileHandler::savePiece(const uint32_t pieceIndex, const vector<uint8_t> &pieceData) {
    if (downloadProgress.getPiece(pieceIndex).hash == SHA256::toHashSha256(pieceData)) {
        std::lock_guard<std::mutex> lock(mutex_);
        Utils::FileUtils::writeChunkToFile(pieceData, fileName, getOffset(pieceIndex));
        downloadProgress.updatePieceStatus(pieceIndex, DownloadStatus::Downloaded);
    } else {
        downloadProgress.updatePieceStatus(pieceIndex, DownloadStatus::Empty);
    }
}


void FileHandler::saveBlock(const uint32_t pieceIndex, const uint16_t blockIndex, const vector<uint8_t> &data) {
    std::lock_guard<std::mutex> lock(mutex_);
    Utils::FileUtils::writeChunkToFile(data, fileName, getOffset(pieceIndex, blockIndex));
    if (downloadProgress.downloadedBlock(pieceIndex, blockIndex)) {
        const bool isGood = Utils::FileUtils::verifyPiece(dirPath + "/" + fileName, getOffset(pieceIndex), pieceSize,
                                                          downloadProgress.getPiece(pieceIndex).hash);
        downloadProgress.updatePieceStatus(pieceIndex, isGood ? DownloadStatus::Verified : DownloadStatus::Empty);
    }
}

vector<uint8_t> FileHandler::loadPiece(const uint32_t pieceIndex) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return Utils::FileUtils::readFileChunk(dirPath + "/" + fileName, getOffset(pieceIndex), pieceSize);
}

vector<uint8_t> FileHandler::loadBlock(const uint32_t pieceIndex, const uint32_t blockIndex) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return Utils::FileUtils::readFileChunk(dirPath + "/" + fileName, getOffset(pieceIndex, blockIndex),
                                           Utils::FileSplitter::BLOCK_SIZE);
}

vector<FileHandler> FileHandler::getAllHandlers() {
    vector<FileHandler> handlers;
    for (const auto &dir: FileUtils::listDirectories(dirPath)) {
        handlers.emplace_back(dir);
    }
    return handlers;
}
