#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <cstdint>
#include <stdexcept>

class FileUtils
{
    public:

    static std::vector<uint8_t> readFileToVector(const std::string& filePath); // Function receives a filePath and returns a file contents (also valid for non txt files)


};
