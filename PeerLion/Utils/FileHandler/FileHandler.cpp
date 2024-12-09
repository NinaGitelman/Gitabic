//
// Created by user on 12/9/24.
//

#include "FileHandler.h"
#include "../FileUtils/FileUtils.h"
void FileHandler::savePiece(uint32_t pieceIndex, vector<uint8_t> pieceData){
    Utils::FileUtils::writeChunkToFile(pieceData, fileName,pieceIndex* Utils::FileSplitter::pieceSize(metaDataFile.getFileSize()));
}
