#ifndef DOWNLOADPROGRESS_H
#define DOWNLOADPROGRESS_H
#pragma once
#include "../MetaDataFile/MetaDataFile.h"

/// Represents the current status of a download operation
enum class DownloadStatus {
    Empty, ///< No data has been downloaded
    Downloading, ///< Download is in progress
    Downloaded, ///< Data has been completely downloaded
    Verified, ///< Downloaded data has been verified
};

/// @brief Represents information about a block within a piece of the download
struct BlockInfo {
    uint32_t offset; ///< Offset of the block within its piece
    uint16_t size; ///< Size of the block in bytes
    bool isLastBlock; ///< Indicates if this is the last block in the piece
    DownloadStatus status; ///< Current download status of the block

    /// @brief Serializes the block information into a byte vector
    /// @return Vector containing the serialized block data
    [[nodiscard]] vector<uint8_t> serialize() const;

    /// @brief Deserializes block information from a byte vector
    /// @param data Vector containing the serialized data
    /// @param offset Current position in the data vector, updated after deserialization
    /// @return Reconstructed BlockInfo object
    static BlockInfo deserialize(const vector<uint8_t> &data, size_t &offset);
};

/// @brief Represents progress information for a piece of the download
struct PieceProgress {
    DownloadStatus status; ///< Current download status of the piece
    uint64_t offset; ///< Offset of the piece within the file
    std::vector<BlockInfo> blocks; ///< List of blocks within the piece
    uint32_t size; ///< Size of the piece in bytes
    uint32_t bytesDownloaded; ///< Number of bytes downloaded for this piece
    time_t lastAccess; ///< Timestamp of last access to this piece
    HashResult hash; ///< Hash of the piece for verification

    /// @brief Constructs a new piece progress tracker
    /// @param offset Offset of the piece within the file
    /// @param size Size of the piece in bytes
    /// @param hash Expected hash value for verification
    PieceProgress(uint64_t offset, uint32_t size, const HashResult &hash);

    /// @brief Marks a block as downloaded and updates piece status
    /// @param block Index of the block that was downloaded
    /// @return Size of the downloaded block in bytes
    /// @throw std::out_of_range if block index is invalid
    uint16_t downloadedBlock(uint16_t block);

    /// @brief Checks if all blocks in the piece have been downloaded
    /// @return true if all blocks are downloaded, false otherwise
    [[nodiscard]] bool allBlocksDownloaded() const;

    /// @brief Updates the download status of the piece
    /// @param status New status to set
    uint32_t setStatus(DownloadStatus status);

    /// @brief Updates the status of a specific block
    /// @param block Index of the block to update
    /// @param status New status to set
    /// @throw std::out_of_range if block index is invalid
    void updateBlockStatus(uint16_t block, DownloadStatus status);

    [[nodiscard]] BlockInfo getBlockInfo(uint16_t block) const;

    [[nodiscard]] PieceProgress getBlocksStatused(DownloadStatus status = DownloadStatus::Empty) const;


    /// @brief Serializes the piece progress information
    /// @return Vector containing the serialized piece data
    [[nodiscard]] vector<uint8_t> serialize() const;

    /// @brief Deserializes piece progress information
    /// @param data Vector containing the serialized data
    /// @param offset Current position in the data vector, updated after deserialization
    /// @return Reconstructed PieceProgress object
    static PieceProgress deserialize(const vector<uint8_t> &data, size_t &offset);
};

/// @brief Manages and tracks the progress of a file download
class DownloadProgress {
public:
    [[nodiscard]] const string &get_file_name() const {
        return fileName;
    }

    [[nodiscard]] const string &get_creator() const {
        return creator;
    }

    [[nodiscard]] const HashResult &get_file_hash() const {
        return fileHash;
    }

    [[nodiscard]] bool is_completed() const {
        return completed;
    }

    [[nodiscard]] uint64_t get_total_download_bytes() const {
        return totalDownloadBytes;
    }

    [[nodiscard]] uint64_t get_file_size() const {
        return fileSize;
    }

    [[nodiscard]] time_t get_start_time() const {
        return startTime;
    }

    [[nodiscard]] time_t get_last_time() const {
        return lastTime;
    }

    /// @brief Constructs a new download progress tracker from metadata
    /// @param metaData Metadata of the file being downloaded
    explicit DownloadProgress(const MetaDataFile &metaData);

    DownloadProgress(const DownloadProgress &downloadProgress);

    DownloadProgress() = default;

    DownloadProgress &operator=(const MetaDataFile &meta);

    DownloadProgress &operator=(DownloadProgress other);

    // Swap function
    friend void swap(DownloadProgress &first, DownloadProgress &second) noexcept;

    /// @brief Constructs a download progress tracker from serialized data
    /// @param data Vector containing serialized download progress data
    explicit DownloadProgress(const vector<uint8_t> &data);

    /// @brief Destructor
    ~DownloadProgress();

    /// @brief Serializes the current download progress state
    /// @return Vector containing the serialized download progress data
    vector<uint8_t> serialize();

    /// @brief Deserializes download progress state from data
    /// @param data Vector containing serialized download progress data
    void deserialize(vector<uint8_t> data);

    /// @brief Calculates the current download progress as a percentage
    /// @return Progress value between 0.0 and 1.0
    [[nodiscard]] double progress() const;

    /// @brief Updates progress when a block is downloaded
    /// @param piece Index of the piece containing the block
    /// @param block Index of the downloaded block within the piece
    /// @throw std::out_of_range if piece or block index is invalid
    bool downloadedBlock(uint32_t piece, uint16_t block);

    /// @brief Updates the status of a specific piece
    /// @param piece Index of the piece to update
    /// @param status New status to set
    /// @throw std::out_of_range if piece index is invalid
    void updatePieceStatus(uint32_t piece, DownloadStatus status);

    /// @brief Updates the status of a specific block within a piece
    /// @param piece Index of the piece containing the block
    /// @param block Index of the block to update
    /// @param status New status to set
    /// @throw std::out_of_range if piece or block index is invalid
    void updateBlockStatus(uint32_t piece, uint16_t block, DownloadStatus status);

    vector<PieceProgress> getPiecesStatused(DownloadStatus status = DownloadStatus::Empty);

    [[nodiscard]] vector<PieceProgress> getBlocksStatused(DownloadStatus status = DownloadStatus::Empty) const;

    [[nodiscard]] PieceProgress getPiece(uint32_t index) const;

private:
    /// @brief Initializes the download progress tracker with metadata
    /// @param metaData Metadata of the file being downloaded
    void init(const MetaDataFile &metaData);

    string fileName; ///< Name of the file being downloaded
    string creator; ///< Name of the creator of file being downloaded
    HashResult fileHash{}; ///< Hash of the complete file
    bool completed{}; ///< Indicates if download is complete
    uint64_t totalDownloadBytes{}; ///< Total bytes downloaded so far
    uint64_t fileSize{}; ///< Total size of the file
    time_t startTime{}; ///< Time when download started
    time_t lastTime{}; ///< Time of last download activity
    vector<PieceProgress> pieces; ///< List of all pieces in the download
    mutable std::mutex mut;
};

#endif
