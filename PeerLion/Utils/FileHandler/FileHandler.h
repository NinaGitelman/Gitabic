#ifndef FILEHANDLER_H
#define FILEHANDLER_H

#include "../../Encryptions/SHA256/sha256.h"
#include "../MetaDataFile/MetaDataFile.h"
#include "../DowndloadProgress/DownloadProgress.h"

enum FileMode {Download, Seed, Hybrid};

class FileHandler {
private:
	HashResult torrentHash;
    string dirPath;
    string fileName;
    MetaDataFile metaDataFile;
    FileMode mode;
    DownloadProgress downloadProgress;
    std::mutex mut;
public:
    FileHandler();
	~FileHandler();
	void savePiece(uint32_t pieceIndex, vector<uint8_t> pieceData);
    vector<uint8_t> loadPiece(uint32_t pieceIndex);
    void saveBlock(uint32_t pieceIndex, uint16_t blockIndex, vector<uint8_t> data);
    vector<uint8_t> loadBlock(uint32_t pieceIndex, uint32_t blockIndex);
	DownloadProgress& getDownloadProgress();
    MetaDataFile& getMetaDataFile();


};



#endif //FILEHANDLER_H
