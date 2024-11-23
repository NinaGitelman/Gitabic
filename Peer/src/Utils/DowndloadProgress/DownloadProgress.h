#ifndef DOWNLOADPROGRESS_H
#define DOWNLOADPROGRESS_H

#pragma once
#include <bitset>
#include <Utils/MetaDataFile/MetaDataFile.h>
using std::bitset;

struct PieceProgress
{
    enum class PieceStatus
    {
        Empty,
        Downloading,
        Downloaded,
        Verified,
        Invalid
    } status;
    uint16_t index;
    std::vector<bitset<8>> blocksCompleted;
    uint32_t size;
    uint32_t bytesDownloaded;
    time_t lastAccess;

    PieceProgress(uint16_t index, uint32_t size)
    {
        this->index = index;
        this->size = size;
        this->bytesDownloaded = 0;
        lastAccess = time(nullptr);
        status = PieceStatus::Empty;
        blocksCompleted = vector<bitset<8>>((size / Utils::FileSplitter::BLOCK_SIZE / 8) + 1);
        blocksCompleted.shrink_to_fit();
        for (auto &&i : blocksCompleted)
        {
            i.reset();
        }
    }
};

class DownloadProgress
{
public:
    DownloadProgress(MetaDataFile *metaData);
    DownloadProgress(vector<uint8_t> data);
    ~DownloadProgress();
    vector<uint8_t> serialize();
    void deserialize(vector<uint8_t> data);
    void init(MetaDataFile &metaData);

private:
    HashResult fileHash;
    bool completed;
    uint64_t totalDownloadBytes;
    time_t startTime;
    time_t lastTime;
};

#endif