//
// Created by user on 12/9/24.
//

#include "FileIO.h"


#include "../../Utils/ThreadSafeCout.h"
#include "../../Utils/FileUtils/FileUtils.h"

using Utils::FileUtils;
using Utils::FileSplitter;


const std::string FileIO::dirPath = FileUtils::getExpandedPath("~/Gitabic/.filesFolders/");

// Swap function definition
void swap(FileIO &first, FileIO &second) noexcept {
    using std::swap;
    swap(first.fileName, second.fileName);
    swap(first.mode, second.mode);
    swap(first.downloadProgress, second.downloadProgress);
    swap(first.pieceSize, second.pieceSize);
    swap(first._n, second._n);
    // `std::mutex` is not swappable, so it remains as is for both objects
}

// Copy constructor
FileIO::FileIO(const FileIO &other)
    : fileName(other.fileName),
      mode(other.mode),
      downloadProgress(other.downloadProgress),
      pieceSize(other.pieceSize), _n(other._n) {
    // Note: `std::mutex` is not copied
}

// Move constructor
FileIO::FileIO(FileIO &&other) noexcept
    : fileName(std::move(other.fileName)),
      mode(other.mode),
      downloadProgress(other.downloadProgress),
      pieceSize(other.pieceSize), _n(other._n) {
    // `std::mutex` remains default-initialized in the moved-from object
}

// Assignment operator (copy-and-swap)
FileIO &FileIO::operator=(FileIO other) {
    swap(*this, other); // Reuse the swap function
    return *this;
}

size_t FileIO::getOffset(const uint32_t pieceIndex, const uint16_t blockIndex) const {
    return pieceSize * pieceIndex + blockIndex * Utils::FileSplitter::BLOCK_SIZE;
}

string FileIO::getCurrentDirPath() const {
    if (_n) {
        return FileUtils::getExpandedPath("~/Gitabic" + std::to_string(_n) + "/.filesFolders/") + SHA256::hashToString(
                   downloadProgress.getFileHash()) + '/';
    }
    return dirPath + SHA256::hashToString(downloadProgress.getFileHash()) + '/';
}


void FileIO::initNew(const MetaDataFile &metaData) {
    const auto fileHash = SHA256::hashToString(metaData.getFileHash());
    auto path = FileUtils::createDownloadFolder(fileHash,
                                                metaData.getCreator() + " - " +
                                                std::filesystem::path(metaData.getFileName()).stem().string(), _n);
    auto filePath = path;
    filePath.append(metaData.getFileName());
    try {
        FileUtils::createFilePlaceHolder(filePath, metaData.getFileSize());
    } catch ([[maybe_unused]] const std::exception &e) {
        std::cerr << e.what();
        throw std::runtime_error("Not enough storage space to create file");
    }

    downloadProgress = metaData;
    FileUtils::writeVectorToFile(downloadProgress.serialize(),
                                 path.append(
                                     ("." + metaData.getFileName() +
                                      ".gitabic")));
}

FileIO::FileIO(const string &hash)
    : mode(FileMode::Default),
      pieceSize(FileSplitter::pieceSize(downloadProgress.getFileSize())), _n(0) {
    string dir = dirPath + hash + '/';
    for (const auto &entry: std::filesystem::directory_iterator(dir)) {
        string name = entry.path().filename().string();
        if (name.find(".gitabic") == string::npos) {
            fileName = name;
            break;
        }
    }
    auto path = dirPath + hash + "/" + fileName;
    downloadProgress =
            DownloadProgress(FileUtils::readFileToVector(dirPath + hash + "/." + fileName + ".gitabic"));
}

FileIO::FileIO(const string &hash, uint8_t n) : mode(FileMode::Default),
                                                pieceSize(FileSplitter::pieceSize(downloadProgress.getFileSize())),
                                                _n(n) {
    string dir = dirPath + hash + '/';
    for (const auto &entry: std::filesystem::directory_iterator(dir)) {
        string name = entry.path().filename().string();
        if (name.find(".gitabic") == string::npos) {
            fileName = name;
            break;
        }
    }
    auto path = dirPath + hash + "/" + fileName;
    downloadProgress =
            DownloadProgress(FileUtils::readFileToVector(dirPath + hash + "/." + fileName + ".gitabic"));
}

FileIO::FileIO(const MetaDataFile &metaData, const uint8_t n) : _n(n) {
    initNew(metaData);
    this->mode = FileMode::Default;
    fileName = downloadProgress.getFileName();
    pieceSize = FileSplitter::pieceSize(downloadProgress.getFileSize());
}

void FileIO::savePiece(const uint32_t pieceIndex, const vector<uint8_t> &pieceData) {
    if (downloadProgress.getPiece(pieceIndex).hash == SHA256::toHashSha256(pieceData)) {
        std::lock_guard<std::mutex> lock(mutex_);
        Utils::FileUtils::writeChunkToFile(pieceData, fileName, getOffset(pieceIndex));
        downloadProgress.updatePieceStatus(pieceIndex, DownloadStatus::Downloaded);
    } else {
        downloadProgress.updatePieceStatus(pieceIndex, DownloadStatus::Empty);
    }
}


bool FileIO::saveBlock(const uint32_t pieceIndex, const uint16_t blockIndex, const vector<uint8_t> &data) {
    std::lock_guard<std::mutex> lock(mutex_);
    Utils::FileUtils::writeChunkToFile(data, fileName, getOffset(pieceIndex, blockIndex));
    if (downloadProgress.downloadedBlock(pieceIndex, blockIndex)) {
        const bool isGood = Utils::FileUtils::verifyPiece(getCurrentDirPath() + fileName, getOffset(pieceIndex),
                                                          pieceSize,
                                                          downloadProgress.getPiece(pieceIndex).hash);
        downloadProgress.updatePieceStatus(pieceIndex, isGood ? DownloadStatus::Verified : DownloadStatus::Empty);
        return isGood;
    }
    return false;
}

vector<uint8_t> FileIO::loadPiece(const uint32_t pieceIndex) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return Utils::FileUtils::readFileChunk(getCurrentDirPath() + fileName, getOffset(pieceIndex), pieceSize);
}

vector<uint8_t> FileIO::loadBlock(const uint32_t pieceIndex, const uint32_t blockIndex) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return Utils::FileUtils::readFileChunk(getCurrentDirPath() + fileName, getOffset(pieceIndex, blockIndex),
                                           Utils::FileSplitter::BLOCK_SIZE);
}

void FileIO::saveProgressToFile() {
    FileUtils::writeVectorToFile(downloadProgress.serialize(),
                                 getCurrentDirPath().append(
                                     ("." + downloadProgress.getFileName() +
                                      ".gitabic")));
}

vector<FileIO> FileIO::getAllFileIO() {
    vector<FileIO> handlers;
    for (const auto &dir: FileUtils::listDirectories(dirPath)) {
        FileIO handler(dir);
        handlers.push_back(handler);
    }
    return handlers;
}
