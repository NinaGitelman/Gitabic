//
// Created by user on 12/9/24.
//

#include "FileIO.h"


#include "../../Utils/ThreadSafeCout.h"
#include "../../Utils/FileUtils/FileUtils.h"

using Utils::FileUtils;
using Utils::FileSplitter;


const std::string FileIO::_dirPath = FileUtils::getExpandedPath("~/Gitabic/.filesFolders/");

// Swap function definition
void swap(FileIO &first, FileIO &second) noexcept {
    using std::swap;
    swap(first._fileName, second._fileName);
    swap(first._mode, second._mode);
    swap(first._downloadProgress, second._downloadProgress);
    swap(first._pieceSize, second._pieceSize);
    swap(first._n, second._n);
    // `std::mutex` is not swappable, so it remains as is for both objects
}

// Copy constructor
FileIO::FileIO(const FileIO &other)
    : _fileName(other._fileName),
      _mode(other._mode),
      _downloadProgress(other._downloadProgress),
      _pieceSize(other._pieceSize), _n(other._n) {
    // Note: `std::mutex` is not copied
}

// Move constructor
FileIO::FileIO(FileIO &&other) noexcept
    : _fileName(std::move(other._fileName)),
      _mode(other._mode),
      _downloadProgress(other._downloadProgress),
      _pieceSize(other._pieceSize), _n(other._n) {
    // `std::mutex` remains default-initialized in the moved-from object
}

// Assignment operator (copy-and-swap)
FileIO &FileIO::operator=(FileIO other) {
    swap(*this, other); // Reuse the swap function
    return *this;
}

size_t FileIO::getOffset(const uint32_t pieceIndex, const uint16_t blockIndex) const {
    return _pieceSize * pieceIndex + blockIndex * Utils::FileSplitter::BLOCK_SIZE;
}

string FileIO::getCurrentDirPath() const {
    if (_n) {
        return FileUtils::getExpandedPath("~/Gitabic" + std::to_string(_n) + "/.filesFolders/") + SHA256::hashToString(
                   _downloadProgress.getFileHash()) + '/';
    }
    return _dirPath + SHA256::hashToString(_downloadProgress.getFileHash()) + '/';
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

    _downloadProgress = metaData;
    FileUtils::writeVectorToFile(_downloadProgress.serialize(),
                                 path.append(
                                     ("." + metaData.getFileName() +
                                      ".gitabic")));
}

FileIO::FileIO(const string &hash)
    : _mode(FileMode::Default),
      _n(0) {
    string dir = _dirPath + hash + '/';
    for (const auto &entry: std::filesystem::directory_iterator(dir)) {
        string name = entry.path().filename().string();
        if (name.find(".gitabic") == string::npos) {
            _fileName = name;
            break;
        }
    }
    auto path = _dirPath + hash + "/" + _fileName;
    _downloadProgress =
            DownloadProgress(FileUtils::readFileToVector(_dirPath + hash + "/." + _fileName + ".gitabic"));
    _pieceSize = FileSplitter::pieceSize(_downloadProgress.getFileSize());
}

FileIO::FileIO(const string &hash, uint8_t n) : _mode(FileMode::Default),
                                                _n(n) {
    string dir = FileUtils::getExpandedPath("~/Gitabic" + std::to_string(n) + "/.filesFolders/") + hash + '/';
    for (const auto &entry: std::filesystem::directory_iterator(dir)) {
        string name = entry.path().filename().string();
        if (name.find(".gitabic") == string::npos) {
            _fileName = name;
            break;
        }
    }
    auto path = _dirPath + hash + "/" + _fileName;
    _downloadProgress =
            DownloadProgress(FileUtils::readFileToVector(
                FileUtils::getExpandedPath("~/Gitabic" + std::to_string(n) + "/.filesFolders/") + hash + "/." +
                _fileName
                + ".gitabic"));
    _pieceSize = FileSplitter::pieceSize(_downloadProgress.getFileSize());
}

FileIO::FileIO(MetaDataFile &metaData, const uint8_t n, const bool isSeed) : _n(n) {
    initNew(metaData);
    this->_mode = FileMode::Default;
    _fileName = _downloadProgress.getFileName();
    _pieceSize = FileSplitter::pieceSize(_downloadProgress.getFileSize());
    if (isSeed) {
        _downloadProgress.done();
        saveProgressToFile();
        setFileMode(Seed);
    }
    if (!FileUtils::dirExists(FileUtils::getExpandedPath("~/Gitabic/MetaDatas/"))) {
        FileUtils::createDirectory(FileUtils::getExpandedPath("~/Gitabic/MetaDatas/"));
    }
    FileUtils::writeVectorToFile(metaData.serialize(),
                                 FileUtils::getExpandedPath("~/Gitabic/MetaDatas/") + _fileName + ".gitabic"
    );
}

void FileIO::savePiece(const uint32_t pieceIndex, const vector<uint8_t> &pieceData) {
    if (_downloadProgress.getPiece(pieceIndex).hash == SHA256::toHashSha256(pieceData)) {
        std::lock_guard<std::mutex> lock(mutex_);
        Utils::FileUtils::writeChunkToFile(pieceData, _fileName, getOffset(pieceIndex));
        _downloadProgress.updatePieceStatus(pieceIndex, DownloadStatus::Downloaded);
    } else {
        _downloadProgress.updatePieceStatus(pieceIndex, DownloadStatus::Empty);
    }
}


bool FileIO::saveBlock(const uint32_t pieceIndex, const uint16_t blockIndex, const vector<uint8_t> &data) {
    std::lock_guard<std::mutex> lock(mutex_);
    Utils::FileUtils::writeChunkToFile(data, getCurrentDirPath() + _fileName, getOffset(pieceIndex, blockIndex));
    if (_downloadProgress.downloadedBlock(pieceIndex, blockIndex)) {
        const bool isGood = Utils::FileUtils::verifyPiece(getCurrentDirPath() + _fileName, getOffset(pieceIndex),
                                                          _downloadProgress.getPiece(pieceIndex).size,
                                                          _downloadProgress.getPiece(pieceIndex).hash);
        _downloadProgress.updatePieceStatus(pieceIndex, isGood ? DownloadStatus::Verified : DownloadStatus::Empty);
        updatePieceStateToFile(pieceIndex);
        return isGood;
    }
    return false;
}

vector<uint8_t> FileIO::loadPiece(const uint32_t pieceIndex) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return Utils::FileUtils::readFileChunk(getCurrentDirPath() + _fileName, getOffset(pieceIndex), _pieceSize);
}

vector<uint8_t> FileIO::loadBlock(const uint32_t pieceIndex, const uint32_t blockIndex) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return Utils::FileUtils::readFileChunk(getCurrentDirPath() + _fileName, getOffset(pieceIndex, blockIndex),
                                           Utils::FileSplitter::BLOCK_SIZE);
}

void FileIO::saveProgressToFile() {
    FileUtils::writeVectorToFile(_downloadProgress.serialize(),
                                 getCurrentDirPath().append(
                                     ("." + _downloadProgress.getFileName() +
                                      ".gitabic")));
}

void FileIO::updatePieceStateToFile(const uint32_t pieceIndex) const {
    FileUtils::writeChunkToFile(_downloadProgress.getPiece(pieceIndex).serializeForFileUpdate(),
                                getCurrentDirPath() + "." + _fileName + ".gitabic",
                                _downloadProgress.getPieceOffsetInProgressFile(pieceIndex));

    //write bytes read to file
    constexpr uint8_t BYTES_READ_OFFSET = 1;
    uint64_t totalBytes = _downloadProgress.getTotalDownloadBytes();
    FileUtils::writeChunkToFile(vector<uint8_t>(reinterpret_cast<uint8_t *>(&totalBytes),
                                                reinterpret_cast<uint8_t *>(&totalBytes) + sizeof(totalBytes)),
                                getCurrentDirPath() + "." + _fileName + ".gitabic",
                                BYTES_READ_OFFSET);
    //write last access to file
    constexpr uint8_t LAST_ACCESS_OFFSET = 25;
    time_t lastAccess = time(nullptr);
    FileUtils::writeChunkToFile(vector<uint8_t>(reinterpret_cast<uint8_t *>(&lastAccess),
                                                reinterpret_cast<uint8_t *>(&lastAccess) + sizeof(lastAccess)),
                                getCurrentDirPath() + "." + _fileName + ".gitabic",
                                LAST_ACCESS_OFFSET);
}

vector<uint16_t> FileIO::getIllegalPieces() const {
    vector<uint16_t> res;
    for (uint32_t i = 0; i < _downloadProgress.getAmmountOfPieces(); i++) {
        const PieceProgress piece = _downloadProgress.getPiece(i);
        if (!Utils::FileUtils::verifyPiece(getCurrentDirPath() + _fileName, piece.offset,
                                           piece.size,
                                           piece.hash)) {
            res.push_back(i);
        }
    }
    return res;
}

vector<FileIO> FileIO::getAllFileIO(const uint8_t n) {
    vector<FileIO> handlers;
    for (const auto &dir: FileUtils::listDirectories(
             FileUtils::getExpandedPath("~/Gitabic" + (n ? std::to_string(n) : "") + "/.filesFolders/"))) {
        FileIO handler(dir);
        handlers.push_back(handler);
    }
    return handlers;
}
