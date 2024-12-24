#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include "Utils/FileHandler/FileHandler.h"
#include "Utils/FileUtils/FileUtils.h"
#include "Utils/MetaDataFile/MetaDataFile.h"

void SetUp() {
    // Create a test file and metadata
    Utils::FileUtils::createFilePlaceHolder("./test_file.txt", 1024);
}

void TearDown() {
    // Clean up test files
    std::filesystem::remove("./test_file.txt");
    std::filesystem::remove_all(Utils::FileUtils::getExpandedPath("~/Gitabic/.filesFolders/"));
}

void InitNewFileHandler() {
    MetaDataFile metaData = MetaDataFile::createMetaData("./test_file.txt", "password", Address("127.0.0.1", 8080),
                                                         "creator");
    FileHandler fileHandler(metaData);
    if (fileHandler.getFileName() == "test_file.txt" && fileHandler.getDownloadProgress().get_file_name() ==
        "test_file.txt") {
        std::cout << "InitNewFileHandler passed" << std::endl;
    } else {
        std::cout << "InitNewFileHandler failed" << std::endl;
    }
}

void SaveAndLoadPiece() {
    MetaDataFile metaData = MetaDataFile::createMetaData("./test_file.txt", "password", Address("127.0.0.1", 8080),
                                                         "creator");
    FileHandler fileHandler(metaData);
    std::vector<uint8_t> pieceData(1024, 'A');
    fileHandler.savePiece(0, pieceData);

    auto loadedPiece = fileHandler.loadPiece(0);
    if (loadedPiece == pieceData) {
        std::cout << "SaveAndLoadPiece passed" << std::endl;
    } else {
        std::cout << "SaveAndLoadPiece failed" << std::endl;
    }
}

void SaveAndLoadBlock() {
    MetaDataFile metaData = MetaDataFile::createMetaData("./test_file.txt", "password", Address("127.0.0.1", 8080),
                                                         "creator");
    FileHandler fileHandler(metaData);
    std::vector<uint8_t> blockData(512, 'B');
    fileHandler.saveBlock(0, 0, blockData);

    auto loadedBlock = fileHandler.loadBlock(0, 0);
    if (loadedBlock == blockData) {
        std::cout << "SaveAndLoadBlock passed" << std::endl;
    } else {
        std::cout << "SaveAndLoadBlock failed" << std::endl;
    }
}

void GetAllHandlers() {
    MetaDataFile metaData = MetaDataFile::createMetaData("./test_file.txt", "password", Address("127.0.0.1", 8080),
                                                         "creator");
    FileHandler fileHandler(metaData);
    auto handlers = FileHandler::getAllHandlers();
    if (!handlers.empty()) {
        std::cout << "GetAllHandlers passed" << std::endl;
    } else {
        std::cout << "GetAllHandlers failed" << std::endl;
    }
}

int main() {
    SetUp();
    InitNewFileHandler();
    SaveAndLoadPiece();
    SaveAndLoadBlock();
    GetAllHandlers();
    TearDown();

    return 0;
}
