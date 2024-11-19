#include "FileUtils.h"

std::vector<uint8_t> FileUtils::readFileToVector(const std::string &filePath)
{
    std::ifstream file(filePath, std::ios::in | std::ios::binary);

    if (!file)
    {
        throw std::runtime_error("Could not open the file: " + filePath);
    }

    // Move to the end of the file to get the file size
    file.seekg(0, std::ios::end);
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Resize vector to hold the file data and read the file contents
    std::vector<uint8_t> content(fileSize);
    if (file.read(reinterpret_cast<char *>(content.data()), fileSize))
    {
        return content;
    }
    else
    {
        throw std::runtime_error("Error reading the file: " + filePath);
    }
}

void FileUtils::writeVectorToFile(const std::vector<uint8_t> &data, const std::string &filePath)
{
    std::ofstream outFile(filePath, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!outFile)
    {
        throw std::ios_base::failure("Failed to open file for writing");
    }
    outFile.write(reinterpret_cast<const char *>(data.data()), data.size());
    outFile.close();
}
