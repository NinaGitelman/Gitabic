#include "FileUtils.h"

using namespace Utils;

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

std::array<uint8_t, 16> Conversions::cutArray(HashResult &from)
{
    std::array<uint8_t, 16> result;
    memcpy(result.data(), from.data(), result.size());
    return result;
}

std::vector<uint8_t> Utils::Conversions::toVector(HashResult hash)
{
    vector<uint8_t> result;
    result.insert(result.end(), hash.begin(), hash.end());
    return result;
}

std::vector<uint8_t> Utils::Conversions::toVector(array<uint8_t, 16> aesKey)
{
    vector<uint8_t> result;
    result.insert(result.end(), aesKey.begin(), aesKey.end());
    return result;
}

array<uint8_t, 16> Utils::Conversions::toKey(std::vector<uint8_t> vec)
{
    array<uint8_t, 16> result;
    memcpy(result.data(), vec.data(), std::min(16, (int)vec.size()));
    return result;
}

uint FileSplitter::pieceSize(const uint64_t fileSize)
{
    if (fileSize <= KB * MIN_PIECE_SIZE)
    {
        return MIN_PIECE_SIZE;
    }
    if (fileSize <= MAX_PIECE_SIZE * KB)
    {
        return fileSize / KB;
    }
    return MAX_PIECE_SIZE;
}
