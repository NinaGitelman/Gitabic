#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <filesystem>
#include <Encryptions/SHA256/sha256.h>
#include <Encryptions/AES/AESHandler.h>
#include <random>

#define KB 1024UL

namespace Utils
{
    class FileUtils
    {
    public:
        static std::vector<uint8_t> readFileToVector(const std::string &filePath);
        static std::vector<uint8_t> readFileChunk(const std::string &filePath, size_t offset, size_t size);
        static void writeVectorToFile(const std::vector<uint8_t> &data, const std::string &filePath);
        static void writeChunkToFile(const std::vector<uint8_t> &data, const std::string &filePath, uint64_t offset);
        static void createFilePlaceHolder(const std::string &filePath, const uint64_t size);
        static bool verifyPiece(const std::string &filePath, uint64_t offset, const uint64_t size, HashResult hash);
    };

    class FileSplitter
    {
    public:
        static constexpr uint64_t MIN_PIECE_SIZE = 256 * KB;
        static constexpr uint64_t MAX_PIECE_SIZE = 4 * KB * KB;
        static constexpr uint16_t BLOCK_SIZE = 16 * KB;
        static uint pieceSize(const uint64_t fileSize);
    };

    class Conversions
    {
    public:
        static std::array<uint8_t, 16> cutArray(HashResult &from);
        static std::vector<uint8_t> toVector(HashResult hash);
        static std::vector<uint8_t> toVector(array<uint8_t, 16> aesKey);
        static array<uint8_t, 16> toKey(std::vector<uint8_t> vec);
    };
}