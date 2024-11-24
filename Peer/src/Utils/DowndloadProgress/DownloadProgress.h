#ifndef DOWNLOADPROGRESS_H
#define DOWNLOADPROGRESS_H

#pragma once
#include <Utils/MetaDataFile/MetaDataFile.h>
enum class DownloadStatus
{
    Empty,
    Downloading,
    Downloaded,
    Verified,
    Invalid
};
struct BlockInfo
{
    uint32_t offset;
    uint16_t size;
    bool isLastBlock;
    DownloadStatus status;

    vector<uint8_t> serialize() const;
    static BlockInfo deserialize(const vector<uint8_t> &data, size_t &offset);
};

struct PieceProgress
{
    DownloadStatus status;
    uint64_t offset;
    std::vector<BlockInfo> blocks;
    uint32_t size;
    uint32_t bytesDownloaded;
    time_t lastAccess;
    HashResult hash;

    PieceProgress(uint64_t offset, uint32_t size, HashResult hash);

    uint16_t downloadedBlock(uint16_t block);
    bool allBlocksDownloaded();
    void setStatus(DownloadStatus status);
    void updateBlockStatus(uint16_t block, DownloadStatus status);
    vector<uint8_t> serialize() const;
    static PieceProgress deserialize(const vector<uint8_t> &data, size_t &offset);
};

class DownloadProgress
{
public:
    DownloadProgress(MetaDataFile &metaData);
    DownloadProgress(vector<uint8_t> &data);
    ~DownloadProgress();
    vector<uint8_t> serialize();
    void deserialize(vector<uint8_t> data);
    double proggres();
    void downloadedBlock(uint32_t piece, uint16_t block);
    void updatePieceStatus(uint32_t piece, DownloadStatus status);
    void updateBlockStatus(uint32_t piece, uint16_t block, DownloadStatus status);

private:
    void init(MetaDataFile &metaData);

    string fileName;
    HashResult fileHash;
    bool completed;
    uint64_t totalDownloadBytes;
    uint64_t fileSize;
    time_t startTime;
    time_t lastTime;
    vector<PieceProgress> pieces;
};

#endif