#ifndef METADATAFILE_H
#define METADATAFILE_H

#pragma once

#include <string>
#include <vector>
#include <Encryptions/SHA256/sha256.h>
#include <NetworkUnit/SocketHandler/SocketHandler.h>

using std::string;
using std::vector;

class MetaDataFile
{
public:
    MetaDataFile();
    ~MetaDataFile();

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

    bool getHasPassword() const { return hasPassword; }
    void setHasPassword(bool hasPassword_) { hasPassword = hasPassword_; }

    array<uint8_t, 16> getEncryptedAesKey() const { return encryptedAesKey; }
    void setEncryptedAesKey(const array<uint8_t, 16> &encryptedAesKey_) { encryptedAesKey = encryptedAesKey_; }

    array<uint8_t, 16> getSalt() const { return salt; }
    void setSalt(const array<uint8_t, 16> &salt_) { salt = salt_; }

private:
    string fileName;
    string creator;
    HashResult fileHash;
    vector<HashResult> partsHashes;
    Address signalingAddress;
    bool hasPassword;
    array<uint8_t, 16> encryptedAesKey;
    array<uint8_t, 16> salt;
};

#endif