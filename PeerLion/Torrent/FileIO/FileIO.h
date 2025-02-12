#ifndef FILEHANDLER_H
#define FILEHANDLER_H

#include "../../Utils/MetaDataFile/MetaDataFile.h"
#include "../../Utils/DowndloadProgress/DownloadProgress.h"

enum FileMode { Download, Seed, Hybrid, Default };

class FileIO {
	static const std::string _dirPath;
	string _fileName;
	FileMode _mode;
	DownloadProgress _downloadProgress;
	size_t _pieceSize;
	mutable mutex mutex_;
	uint8_t _n;

	[[nodiscard]] size_t getOffset(uint32_t pieceIndex, uint16_t blockIndex = 0) const;

	string getCurrentDirPath() const;

public:
	// Rule of five
	FileIO(const FileIO &other); // Copy constructor
	FileIO(FileIO &&other) noexcept; // Move constructor

	FileIO &operator=(FileIO other); // Unified assignment operator
	~FileIO() = default; // Destructor
	// Swap function
	friend void swap(FileIO &first, FileIO &second) noexcept;

	void initNew(const MetaDataFile &metaData);

	[[nodiscard]] string getFileName() const {
		return _fileName;
	}

	[[nodiscard]] FileMode getMode() const {
		return _mode;
	}

	[[nodiscard]] DownloadProgress &getDownloadProgress() {
		return _downloadProgress;
	}

	void setFileMode(const FileMode mode) {
		this->_mode = mode;
	}

	explicit FileIO(const string &hash);

	FileIO(const string &hash, uint8_t n);

	explicit FileIO(MetaDataFile &metaData, uint8_t n = 0, bool isSeed = false);

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
	bool saveBlock(uint32_t pieceIndex, uint16_t blockIndex, const vector<uint8_t> &data);

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

	void saveProgressToFile();

	void updatePieceStateToFile(uint32_t pieceIndex) const;

	[[nodiscard]] vector<uint16_t> getIllegalPieces() const;

	/**
	 * initiates all files io
	 * @return
	 */
	static vector<FileIO> getAllFileIO(uint8_t n = 0);
};


#endif //FILEHANDLER_H
