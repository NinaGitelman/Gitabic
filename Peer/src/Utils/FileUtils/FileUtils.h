#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <cstdint>
#include <stdexcept>

class FileUtils
{
public:
    static std::vector<uint8_t> readFileToVector(const std::string &filePath);
    static void writeVectorToFile(const std::vector<uint8_t> &data, const std::string &filePath);
};
