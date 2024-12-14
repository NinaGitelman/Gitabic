
#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <filesystem>
#include "../../Encryptions/SHA256/sha256.h"
#include "../../Encryptions/AES/AESHandler.h"
#include <random>

#define KB 1024UL

namespace Utils {
    /// @brief Class providing file manipulation utilities
    class FileUtils {
    public:
        /// @brief Reads entire file into a byte vector
        /// @param filePath Path to the file to read
        /// @return Vector containing file contents
        /// @throw std::runtime_error if file cannot be opened or read
        static std::vector<uint8_t> readFileToVector(const std::string &filePath);

        /// @brief Reads a chunk of data from a file
        /// @param filePath Path to the file to read from
        /// @param offset Starting position in the file
        /// @param size Number of bytes to read
        /// @return Vector containing the read chunk
        /// @throw std::runtime_error if file cannot be opened or read
        static std::vector<uint8_t> readFileChunk(const std::string &filePath, size_t offset, size_t size);

        /// @brief Writes a vector of bytes to a file
        /// @param data Data to write
        /// @param filePath Path to the destination file
        /// @throw std::ios_base::failure if file cannot be opened or written
        static void writeVectorToFile(const std::vector<uint8_t> &data, const std::string &filePath);

        /// @brief Writes a chunk of data to a specific position in a file
        /// @param data Data to write
        /// @param filePath Path to the destination file
        /// @param offset Position in the file to write at
        /// @throw std::ios_base::failure if file cannot be opened or written
        static void writeChunkToFile(const std::vector<uint8_t> &data, const std::string &filePath, uint64_t offset);

        /// @brief Creates an empty file of specified size
        /// @param filePath Path to create the file at
        /// @param size Size of the file to create
        /// @throw std::runtime_error if file cannot be created or written
        static void createFilePlaceHolder(const std::string &filePath, const uint64_t size);

        /// @brief Verifies a piece of file against its hash
        /// @param filePath Path to the file containing the piece
        /// @param offset Start position of the piece
        /// @param size Size of the piece
        /// @param hash Expected hash value
        /// @return true if piece matches hash, false otherwise
        static bool verifyPiece(const std::string &filePath, uint64_t offset, const uint64_t size,
                                const HashResult &hash);

        static bool fileExists(const std::string &filePath);

        static bool dirExists(const std::string &filePath);

        static std::filesystem::path createDownloadFolder(const std::string &fileHash, const std::string &friendlyName);

        static vector<std::string> listDirectories(const std::string &path);
    };

    /// @brief Class handling file splitting logic
    class FileSplitter {
    public:
        static constexpr uint64_t MIN_PIECE_SIZE = 256 * KB; ///< Minimum size of a file piece
        static constexpr uint64_t MAX_PIECE_SIZE = 4 * KB * KB; ///< Maximum size of a file piece
        static constexpr uint16_t BLOCK_SIZE = 16 * KB; ///< Size of individual blocks

        /// @brief Calculates appropriate piece size for a file
        /// @param fileSize Size of the file
        /// @return Appropriate piece size in bytes
        static uint pieceSize(const uint64_t fileSize);
    };

    /// @brief Class providing data conversion utilities
    class Conversions {
    public:
        /// @brief Extracts first 16 bytes from a hash
        /// @param from Source hash
        /// @return Array containing first 16 bytes
        static std::array<uint8_t, 16> cutArray(const HashResult &from);

        /// @brief Converts a hash to a byte vector
        /// @param hash Hash to convert
        /// @return Vector containing hash bytes
        static std::vector<uint8_t> toVector(HashResult hash);

        /// @brief Converts a 16-byte array to a vector
        /// @param aesKey Array to convert
        /// @return Vector containing array bytes
        static std::vector<uint8_t> toVector(array<uint8_t, 16> aesKey);

        /// @brief Converts a vector to a 16-byte array
        /// @param vec Vector to convert
        /// @return 16-byte array (truncated if vector is larger)
        static array<uint8_t, 16> toKey(const std::vector<uint8_t> &vec);
    };
}
