#include "MetaDataFile.h"

MetaDataFile::MetaDataFile(vector<uint8_t> &data)
{
    deserialize(data);
}

MetaDataFile::MetaDataFile(const string &path)
{
    deserialize(FileUtils::readFileToVector(path));
}

MetaDataFile::~MetaDataFile()
{
}

vector<uint8_t> MetaDataFile::serialize()
{
    vector<uint8_t> data;
    data.insert(data.end(), &sizes, &sizes + sizeof(sizes));
    serializeString(data, fileName);
    serializeString(data, creator);
    serializeHash(data, fileHash);
    for (auto &&it : partsHashes)
    {
        serializeHash(data, it);
    }
    data.insert(data.end(), signalingAddress.ip.data(), signalingAddress.ip.data() + signalingAddress.ip.size());
    data.insert(data.end(), &signalingAddress.port, &signalingAddress.port + sizeof(signalingAddress.port));
    serializeBlock(data, encryptedAesKey);
    if (sizes.hasPassword)
    {
        serializeBlock(data, salt);
    }
}

void MetaDataFile::deserialize(vector<uint8_t> data)
{
    uint i = 0;
    memcpy(&sizes, data.data(), sizeof(sizes));
    i += sizeof(sizes);
    uint size = 0;
    memcpy(&size, data.data() + i, sizeof(size));
    i += sizeof(size);
    fileName = string(data.data() + i, data.data() + i + size);
    i += size;
    memcpy(&size, data.data() + i, sizeof(size));
    i += sizeof(size);
    creator = string(data.data() + i, data.data() + i + size);
    i += size;
    memcpy(&fileHash, data.data() + i, fileHash.size());
    i += fileHash.size();
    for (size_t j = 0; j < sizes.partsCount; j++)
    {
        HashResult hash;
        memcpy(&hash, data.data() + i, hash.size());
        i += hash.size();
        partsHashes.push_back(hash);
    }
    memcpy(signalingAddress.ip.data(), data.data() + i, signalingAddress.ip.size());
    i += signalingAddress.ip.size();
    memcpy(&signalingAddress.port, data.data() + i, sizeof(signalingAddress.port));
    i += sizeof(signalingAddress.port);
    memcpy(encryptedAesKey.data(), data.data() + i, encryptedAesKey.size());
    i + encryptedAesKey.size();
    if (sizes.hasPassword)
    {
        memcpy(salt.data(), data.data() + i, salt.size());
        i + salt.size();
    }
}

void MetaDataFile::serializeString(vector<uint8_t> &data, string &str)
{
    uint strSize = str.size();
    data.insert(data.end(), (uint8_t *)&strSize, (uint8_t *)&strSize + sizeof(strSize));
    data.insert(data.end(), str.c_str(), str.c_str() + strSize);
}

void MetaDataFile::serializeHash(vector<uint8_t> &data, HashResult &hash)
{
    data.insert(data.end(), hash.data(), hash.data() + hash.size());
}

void MetaDataFile::serializeBlock(vector<uint8_t> &data, array<uint8_t, 16> &block)
{
    data.insert(data.end(), block.data(), block.data() + block.size());
}
