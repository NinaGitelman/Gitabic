//
// Created by user on 12/9/24.
//

#include "FileHandler.h"

#include <utility>
#include "../FileUtils/FileUtils.h"

size_t FileHandler::getOffset(const uint32_t pieceIndex, const uint16_t blockIndex) const {
    return pieceSize * pieceIndex + blockIndex * Utils::FileSplitter::BLOCK_SIZE;
}

FileHandler::FileHandler(const string &path, string name, const FileMode mode)
    : dirPath(path),
      fileName(std::move(name)),
      metaDataFile(path + "/" + fileName + ".gitabic"),
      mode(mode),
      downloadProgress(Utils::FileUtils::fileExists(path + "/" + fileName + ".prog")
                           ? DownloadProgress(Utils::FileUtils::readFileToVector(path + "/" + fileName + ".prog"))
                           : DownloadProgress(metaDataFile)),
      pieceSize(Utils::FileSplitter::pieceSize(metaDataFile.getFileSize())) {
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

vector<uint8_t> FileHandler::loadPiece(const uint32_t pieceIndex) const{
    std::lock_guard<std::mutex> lock(mutex_);
    return Utils::FileUtils::readFileChunk(dirPath + "/" + fileName, getOffset(pieceIndex), pieceSize);
}

vector<uint8_t> FileHandler::loadBlock(const uint32_t pieceIndex, const uint32_t blockIndex) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return Utils::FileUtils::readFileChunk(dirPath + "/" + fileName, getOffset(pieceIndex, blockIndex),
                                           Utils::FileSplitter::BLOCK_SIZE);
}
