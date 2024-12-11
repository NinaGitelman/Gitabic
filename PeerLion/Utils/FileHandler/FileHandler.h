#ifndef FILEHANDLER_H
#define FILEHANDLER_H

#include "../../Encryptions/SHA256/sha256.h"
#include "../MetaDataFile/MetaDataFile.h"
#include "../DowndloadProgress/DownloadProgress.h"

enum FileMode { Download, Seed, Hybrid };

class FileHandler {
private:
	string dirPath;
	string fileName;
	MetaDataFile metaDataFile;
	FileMode mode;
	DownloadProgress downloadProgress;
	size_t pieceSize;
	mutable mutex mutex_;

	[[nodiscard]] size_t getOffset(uint32_t pieceIndex, uint16_t blockIndex = 0) const;

public:
	[[nodiscard]] string getFileName() const {
		return fileName;
	}

	[[nodiscard]] MetaDataFile &getMetaData() {
		return metaDataFile;
	}

	[[nodiscard]] FileMode getMode() const {
		return mode;
	}

	[[nodiscard]] DownloadProgress &getDownloadProgress() {
		return downloadProgress;
	}

	void setFileMode(const FileMode mode) {
		this->mode = mode;
	}

private:
	std::mutex mut;

public:
	FileHandler(const string &path, string name, FileMode mode);

	~FileHandler() = default;

	/**
	 *
	 * @param pieceIndex The index of the piece
	 * @param pieceData The data to save
	 */
	void savePiece(uint32_t pieceIndex, const vector<uint8_t> &pieceData);

	/**
	 *
	 * @param pieceIndex The index of the piece
	 * @param blockIndex The index of the block
	 * @param data The block data
	 */
	void saveBlock(uint32_t pieceIndex, uint16_t blockIndex, const vector<uint8_t> &data);

	/**
	 * @param pieceIndex the piece index
	 * @return The loaded piece data
	 */
	[[nodiscard]] vector<uint8_t> loadPiece(uint32_t pieceIndex) const;

	/**
	 * @param pieceIndex the piece index
	 * @param blockIndex the block index
	 * @return The loaded piece data
	 */
	[[nodiscard]] vector<uint8_t> loadBlock(uint32_t pieceIndex, uint32_t blockIndex) const;
};


#endif //FILEHANDLER_H
