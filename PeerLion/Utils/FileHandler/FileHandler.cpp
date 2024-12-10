//
// Created by user on 12/9/24.
//

#include "FileHandler.h"
#include "../FileUtils/FileUtils.h"

size_t FileHandler::getOffset(const uint32_t pieceIndex, const uint16_t blockIndex) const {
    return pieceSize * pieceIndex + blockIndex * Utils::FileSplitter::BLOCK_SIZE;
}

FileHandler::FileHandler(const string &path, const string &name, const FileMode mode) {
    this->dirPath = path;
    this->fileName = name;
    const string files = path + "/" + this->fileName;
    metaDataFile = MetaDataFile(files + ".gitabic");
    downloadProgress = DownloadProgress(Utils::FileUtils::fileExists(files + ".prog")
                                            ? Utils::FileUtils::readFileToVector(files + ".prog")
                                            : metaDataFile);
    pieceSize = Utils::FileSplitter::pieceSize(metaDataFile.getFileSize());
    this->mode = mode;
}

void FileHandler::savePiece(const uint32_t pieceIndex, const vector<uint8_t> &pieceData) const {
    Utils::FileUtils::writeChunkToFile(pieceData, fileName, getOffset(pieceIndex));
}

vector<uint8_t> FileHandler::loadPiece(const uint32_t pieceIndex) const {
    return Utils::FileUtils::readFileChunk(dirPath + "/" + fileName, getOffset(pieceIndex), pieceSize);
}

void FileHandler::saveBlock(const uint32_t pieceIndex, const uint16_t blockIndex, const vector<uint8_t> &data) const {
    Utils::FileUtils::writeChunkToFile(data, fileName, getOffset(pieceIndex, blockIndex));
}

vector<uint8_t> FileHandler::loadBlock(const uint32_t pieceIndex, const uint32_t blockIndex) const {
    return Utils::FileUtils::readFileChunk(dirPath + "/" + fileName, getOffset(pieceIndex, blockIndex),
                                           Utils::FileSplitter::BLOCK_SIZE);
}
