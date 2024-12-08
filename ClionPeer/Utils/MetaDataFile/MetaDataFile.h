#ifndef METADATAFILE_H
#define METADATAFILE_H
#pragma once
#include <string>
#include <vector>
#include "../FileUtils/FileUtils.h"
#include "../../NetworkUnit/SocketHandler/SocketHandler.h"
#include "../../Encryptions/SHA256/sha256.h"
using std::string;
using std::vector;

/// @brief Structure containing file metadata sizes and flags
struct Sizes
{
    uint64_t fileSize : 47;   ///< Size of the file in bytes (47-bit field)
    uint64_t partsCount : 16; ///< Number of parts the file is split into (16-bit field)
    uint64_t hasPassword : 1; ///< Flag indicating if the file is password-protected (1-bit field)
};

/// @brief Class handling file metadata including encryption, hashing, and file information
class MetaDataFile
{
public:
    /// @brief Constructs metadata object from serialized data
    /// @param data Vector containing serialized metadata
    MetaDataFile(vector<uint8_t> &data);

    /// @brief Constructs metadata object from a file path
    /// @param path Path to the metadata file
    MetaDataFile(const string &path);

    /// @brief Default constructor
    MetaDataFile() = default;

    /// @brief Destructor
    ~MetaDataFile();

    /// @brief Creates a new metadata file for a given file
    /// @param filePath Path to the file to create metadata for
    /// @param password Optional password for encryption
    /// @param signalingAddress Network address for signaling
    /// @param creator Identity of the metadata creator
    /// @return New MetaDataFile object
    /// @throw std::runtime_error if file does not exist
    static MetaDataFile createMetaData(const std::string &filePath, const std::string password,
                                       const Address &signalingAddress, const string &creator);

    /// @brief Serializes the metadata to a byte vector
    /// @return Vector containing serialized metadata
    vector<uint8_t> serialize();

    /// @brief Deserializes metadata from a byte vector
    /// @param data Vector containing serialized metadata
    void deserialize(vector<uint8_t> &data);

    // Getters and setters
    /// @brief Gets the file name
    /// @return File name string
    string getFileName() const { return fileName; }

    /// @brief Sets the file name
    /// @param fileName_ New file name
    void setFileName(const string &fileName_) { fileName = fileName_; }

    /// @brief Gets the creator identifier
    /// @return Creator string
    string getCreator() const { return creator; }

    /// @brief Sets the creator identifier
    /// @param creator_ New creator identifier
    void setCreator(const string &creator_) { creator = creator_; }

    /// @brief Gets the file hash
    /// @return Hash of the complete file
    HashResult getFileHash() const { return fileHash; }

    /// @brief Sets the file hash
    /// @param fileHash_ New file hash
    void setFileHash(const HashResult &fileHash_) { fileHash = fileHash_; }

    /// @brief Gets hashes of all file parts
    /// @return Vector of part hashes
    vector<HashResult> getPartsHashes() const { return partsHashes; }

    /// @brief Sets hashes for all file parts
    /// @param partsHashes_ Vector of new part hashes
    void setPartsHashes(const vector<HashResult> &partsHashes_) { partsHashes = partsHashes_; }

    /// @brief Gets the signaling address
    /// @return Network address for signaling
    Address getSignalingAddress() const { return signalingAddress; }

    /// @brief Sets the signaling address
    /// @param signalingAddress_ New signaling address
    void setSignalingAddress(const Address &signalingAddress_) { signalingAddress = signalingAddress_; }

    /// @brief Checks if file has password protection
    /// @return true if password protected, false otherwise
    bool getHasPassword() const { return sizes.hasPassword; }

    /// @brief Sets password protection status
    /// @param hasPassword_ New password protection status
    void setHasPassword(bool hasPassword_) { sizes.hasPassword = hasPassword_; }

    /// @brief Gets the encrypted AES key
    /// @return Encrypted AES key
    array<uint8_t, 16> getEncryptedAesKey() const { return encryptedAesKey; }

    /// @brief Sets the encrypted AES key
    /// @param encryptedAesKey_ New encrypted AES key
    void setEncryptedAesKey(const array<uint8_t, 16> &encryptedAesKey_) { encryptedAesKey = encryptedAesKey_; }

    /// @brief Gets the salt used for password hashing
    /// @return Salt value
    array<uint8_t, 16> getSalt() const { return salt; }

    /// @brief Sets the salt for password hashing
    /// @param salt_ New salt value
    void setSalt(const array<uint8_t, 16> &salt_) { salt = salt_; }

    /// @brief Gets the file size
    /// @return Size of the file in bytes
    uint64_t getFileSize() const { return sizes.fileSize; }

    /// @brief Sets the file size
    /// @param size New file size in bytes
    void setFileSize(uint64_t size) { sizes.fileSize = size; }

private:
    /// @brief Serializes a string to a byte vector
    /// @param data Vector to append serialized string to
    /// @param str String to serialize
    void serializeString(vector<uint8_t> &data, string &str);

    /// @brief Serializes a hash to a byte vector
    /// @param data Vector to append serialized hash to
    /// @param hash Hash to serialize
    void serializeHash(vector<uint8_t> &data, HashResult &hash);

    /// @brief Serializes a 16-byte block to a byte vector
    /// @param data Vector to append serialized block to
    /// @param block Block to serialize
    void serializeBlock(vector<uint8_t> &data, array<uint8_t, 16> &block);

    string fileName;                    ///< Name of the file
    string creator;                     ///< Creator identifier
    HashResult fileHash;                ///< Hash of the complete file
    vector<HashResult> partsHashes;     ///< Hashes of individual file parts
    Address signalingAddress;           ///< Network address for signaling
    Sizes sizes;                        ///< File size and metadata information
    array<uint8_t, 16> encryptedAesKey; ///< Encrypted AES key for file encryption
    array<uint8_t, 16> salt;            ///< Salt for password hashing
};
#endif