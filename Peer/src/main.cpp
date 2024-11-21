#include <iostream>
#include "Utils/MetaDataFile/MetaDataFile.h"

int main()
{
    // auto a = MetaDataFile::createMetaData("/home/user/Desktop/Maghshimim/Bit Torrent Uri + Nina/Gitabic/Peer/test.txt", "12345678", Address(), "Uri");
    // auto b = a.serialize();
    // Utils::FileUtils::writeVectorToFile(b, "/home/user/Desktop/Maghshimim/Bit Torrent Uri + Nina/Gitabic/Peer/test.gitabic");
    auto a = Utils::FileUtils::readFileToVector("/home/user/Desktop/Maghshimim/Bit Torrent Uri + Nina/Gitabic/Peer/test.gitabic"); // Need to create 300000001 'a' in test.txt : python3 -c 'print("a" * (99999999 + 1)*3)' > test.txt
    MetaDataFile metaDataFile(a);
    std::cout << Utils::FileUtils::verifyPiece("/home/user/Desktop/Maghshimim/Bit Torrent Uri + Nina/Gitabic/Peer/test.txt", 0, Utils::FileSplitter::pieceSize(metaDataFile.getFileSize()), metaDataFile.getPartsHashes()[0]);
    // SHA256::printHashAsString(metaDataFile.getPartsHashes()[0]);
    // SHA256::printHashAsString(SHA256::toHashSha256(Utils::FileUtils::readFileChunk("/home/user/Desktop/Maghshimim/Bit Torrent Uri + Nina/Gitabic/Peer/test.txt", 0, Utils::FileSplitter::pieceSize(metaDataFile.getFileSize()))));
}