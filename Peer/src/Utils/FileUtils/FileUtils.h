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

namespace Utils
{
    class FileUtils
    {
    public:
        static std::vector<uint8_t> readFileToVector(const std::string &filePath);
        static void writeVectorToFile(const std::vector<uint8_t> &data, const std::string &filePath);
    };

#define KB 1024
#define MIN_PIECE_SIZE 256 * KB
#define MAX_PIECE_SIZE 4 * KB *KB

    class FileSplitter
    {
    public:
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