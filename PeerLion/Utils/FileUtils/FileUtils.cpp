#include "FileUtils.h"

#include "../ThreadSafeCout.h"

using namespace Utils;

std::pmr::unordered_map<std::string, std::mutex> FileUtils::fileMutexes;

std::vector<uint8_t> FileUtils::readFileToVector(const std::string &filePath) {
    std::lock_guard guard(getFileMutex(filePath));
    std::ifstream file(filePath, std::ios::in | std::ios::binary);

    if (!file) {
        throw std::runtime_error("Could not open the file: " + filePath);
    }

    // Move to the end of the file to get the file size
    file.seekg(0, std::ios::end);
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Resize vector to hold the file data and read the file contents
    std::vector<uint8_t> content(fileSize);
    if (file.read(reinterpret_cast<char *>(content.data()), fileSize)) {
        return content;
    } else {
        throw std::runtime_error("Error reading the file: " + filePath);
    }
}

std::vector<uint8_t> Utils::FileUtils::readFileChunk(const std::string &filePath, const size_t offset,
                                                     const size_t size) {
    std::lock_guard guard(getFileMutex(filePath));
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }

    file.seekg(offset);
    std::vector<uint8_t> buffer(size);
    file.read(reinterpret_cast<char *>(buffer.data()), size);
    buffer.resize(file.gcount()); // Trim if we couldn't read the full requested size
    return buffer;
}

void FileUtils::writeVectorToFile(const std::vector<uint8_t> &data, const std::string &filePath) {
    std::lock_guard guard(getFileMutex(filePath));
    std::ofstream outFile(filePath, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!outFile) {
        throw std::ios_base::failure("Failed to open file for writing");
    }
    outFile.write(reinterpret_cast<const char *>(data.data()), data.size());
    outFile.close();
}

void Utils::FileUtils::writeChunkToFile(const std::vector<uint8_t> &data, const std::string &filePath,
                                        uint64_t offset) {
    std::lock_guard guard(getFileMutex(filePath));
    // Open the file in binary mode for both reading and writing, without truncating
    std::ofstream outFile(filePath, std::ios::in | std::ios::out | std::ios::binary);
    if (!outFile) {
        throw std::ios_base::failure("Failed to open file for writing");
    }

    // Move the write pointer to the specified offset
    outFile.seekp(static_cast<std::streampos>(offset));
    if (!outFile) {
        throw std::ios_base::failure("Failed to seek to the specified offset");
    }

    // Write the data at the offset
    outFile.write(reinterpret_cast<const char *>(data.data()), data.size());
    if (!outFile) {
        throw std::ios_base::failure("Failed to write data to the file");
    }

    // Close the file
    outFile.close();
}

void FileUtils::createFilePlaceHolder(const std::string &filePath, const uint64_t size) {
    std::lock_guard guard(getFileMutex(filePath));
    // Open the file in binary mode for writing
    std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to create file: " + filePath);
    }

    // Allocate a buffer to write in chunks
    constexpr size_t bufferSize = 4 * 1024; // 4 KB buffer
    const std::vector<char> buffer(bufferSize, '0'); // Fill buffer with zeros

    uint64_t bytesWritten = 0;
    while (bytesWritten < size) {
        size_t bytesToWrite = std::min<uint64_t>(bufferSize, size - bytesWritten);
        file.write(buffer.data(), bytesToWrite);
        bytesWritten += bytesToWrite;

        if (!file) {
            throw std::runtime_error("Error while writing to file: " + filePath);
        }
    }

    // Close the file
    file.close();
}

bool FileUtils::verifyPiece(const std::string &filePath, const uint64_t offset, const uint64_t size,
                            const HashResult &hash) {
    std::lock_guard guard(getFileMutex(filePath));

    const auto data = readFileChunk(filePath, offset, size);
    return hash == SHA256::toHashSha256(data);
}

bool FileUtils::fileExists(const std::string &filePath) {
    return std::filesystem::exists(filePath);
}

bool FileUtils::dirExists(const std::string &filePath) {
    return std::filesystem::is_directory(filePath);
}

std::string FileUtils::getExpandedPath(const std::string &path) {
    if (!path.empty() && path[0] == '~') {
        if (const char *home = std::getenv("HOME")) {
            return std::string(home) + path.substr(1); // Replace '~' with HOME
        }
    }
    return path; // Return unchanged if no '~' at the beginning
}

std::mutex &FileUtils::getFileMutex(const std::string &filePath) {
    if (!fileMutexes.contains(filePath)) {
        fileMutexes[filePath];
    }
    return fileMutexes[filePath];
}

std::filesystem::path FileUtils::createDownloadFolder(const std::string &fileHash, const std::string &friendlyName,
                                                      const uint8_t n) {
    std::filesystem::path hashFolder = "~/Gitabic/.filesFolders/" + fileHash;
    if (n) hashFolder = "~/Gitabic" + std::to_string(n) + "/.filesFolders/" + fileHash;

    std::filesystem::path expandedHashFolder = std::filesystem::absolute(
        getExpandedPath(hashFolder.string()));

    // Ensure the directory exists
    if (!std::filesystem::exists(expandedHashFolder)) {
        std::filesystem::create_directories(expandedHashFolder);
    } else {
        return expandedHashFolder;
    }

    // Create a symbolic link with the friendly name
    const std::filesystem::path symlink = "~/Gitabic" + (n ? std::to_string(n) : "") + "/" + friendlyName;
    std::filesystem::path expandedSymlink = std::filesystem::absolute(getExpandedPath(symlink.string()));

    // Remove existing symlink if it exists
    while (std::filesystem::exists(expandedSymlink)) {
        expandedSymlink += "2";
    }

    std::filesystem::create_symlink(expandedHashFolder, expandedSymlink);

    return expandedHashFolder;
}

vector<std::string> FileUtils::listDirectories(const std::string &path) {
    vector<std::string> directories;
    try {
        for (const auto &entry: std::filesystem::directory_iterator(path)) {
            if (entry.is_directory()) {
                directories.push_back(entry.path().filename().string());
            }
        }
    } catch (const std::filesystem::filesystem_error &e) {
        ThreadSafeCout::cout(std::string("Error accessing directory: ") + e.what());
    }
    return directories;
}

std::array<uint8_t, 16> Conversions::cutArray(const HashResult &from) {
    std::array<uint8_t, 16> result{};
    memcpy(result.data(), from.data(), result.size());
    return result;
}

std::vector<uint8_t> Utils::Conversions::toVector(HashResult hash) {
    vector<uint8_t> result;
    result.insert(result.end(), hash.begin(), hash.end());
    return result;
}

std::vector<uint8_t> Utils::Conversions::toVector(array<uint8_t, 16> aesKey) {
    vector<uint8_t> result;
    result.insert(result.end(), aesKey.begin(), aesKey.end());
    return result;
}

array<uint8_t, 16> Utils::Conversions::toKey(const std::vector<uint8_t> &vec) {
    array<uint8_t, 16> result;
    memcpy(result.data(), vec.data(), std::min(16, static_cast<int>(vec.size())));
    return result;
}

uint FileSplitter::pieceSize(const uint64_t fileSize) {
    if (fileSize <= MIN_PIECE_SIZE) {
        return fileSize;
    }
    if (fileSize <= KB * MIN_PIECE_SIZE) {
        return MIN_PIECE_SIZE;
    }
    if (fileSize <= MAX_PIECE_SIZE * KB) {
        return fileSize / KB;
    }
    return MAX_PIECE_SIZE;
}
