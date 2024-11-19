#ifndef METADATAFILE_H
#define METADATAFILE_H

#pragma once

#include <string>
#include <vector>
#include <Encryptions/SHA256/sha256.h>
#include <NetworkUnit/SocketHandler/SocketHandler.h>
#include <../FileUtils/FileUtils.h>

using std::string;
using std::vector;

struct Sizes
{
    uint64_t fileSize : 47;
    uint64_t partsCount : 16;
    uint64_t hasPassword : 1;
};

class MetaDataFile
{
public:
    MetaDataFile(vector<uint8_t> &data);
    MetaDataFile(const string &path);
    ~MetaDataFile();

    vector<uint8_t> serialize();
    void deserialize(vector<uint8_t> data);

    string getFileName() const { return fileName; }
    void setFileName(const string &fileName_) { fileName = fileName_; }

    string getCreator() const { return creator; }
    void setCreator(const string &creator_) { creator = creator_; }

    HashResult getFileHash() const { return fileHash; }
    void setFileHash(const HashResult &fileHash_) { fileHash = fileHash_; }

    vector<HashResult> getPartsHashes() const { return partsHashes; }
    void setPartsHashes(const vector<HashResult> &partsHashes_) { partsHashes = partsHashes_; }

    Address getSignalingAddress() const { return signalingAddress; }
    void setSignalingAddress(const Address &signalingAddress_) { signalingAddress = signalingAddress_; }

    bool getHasPassword() const { return sizes.hasPassword; }
    void setHasPassword(bool hasPassword_) { sizes.hasPassword = hasPassword_; }

    array<uint8_t, 16> getEncryptedAesKey() const { return encryptedAesKey; }
    void setEncryptedAesKey(const array<uint8_t, 16> &encryptedAesKey_) { encryptedAesKey = encryptedAesKey_; }

    array<uint8_t, 16> getSalt() const { return salt; }
    void setSalt(const array<uint8_t, 16> &salt_) { salt = salt_; }

private:
    void serializeString(vector<uint8_t> &data, string &str);
    void serializeHash(vector<uint8_t> &data, HashResult &hash);
    void serializeBlock(vector<uint8_t> &data, array<uint8_t, 16> &block);
    string fileName;
    string creator;
    HashResult fileHash;
    vector<HashResult> partsHashes;
    Address signalingAddress;
    Sizes sizes;
    array<uint8_t, 16> encryptedAesKey;
    array<uint8_t, 16> salt;
};

#endif