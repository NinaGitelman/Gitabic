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
	size_t pieceSize;

	[[nodiscard]] size_t getOffset(uint32_t pieceIndex, uint16_t blockIndex = 0) const;

public:
	[[nodiscard]] string getFileName() const {
		return fileName;
	}

	[[nodiscard]] MetaDataFile& getMetaData()  {
		return metaDataFile;
	}

	[[nodiscard]] FileMode getMode() const {
		return mode;
	}

	[[nodiscard]] DownloadProgress& getDownloadProgress() {
		return downloadProgress;
	}

private:
	std::mutex mut;
public:
    FileHandler();
	~FileHandler() = default;


	void savePiece(uint32_t pieceIndex, const vector<uint8_t>& pieceData) const;
    vector<uint8_t> loadPiece(uint32_t pieceIndex) const;
    void saveBlock(uint32_t pieceIndex, uint16_t blockIndex, const vector<uint8_t> &data) const;
    vector<uint8_t> loadBlock(uint32_t pieceIndex, uint32_t blockIndex);


};



#endif //FILEHANDLER_H
