#include "DownloadProgress.h"

DownloadProgress::DownloadProgress(MetaDataFile &metaData)
{
    init(metaData);
}

DownloadProgress::DownloadProgress(vector<uint8_t> &data)
{
    deserialize(data);
}

DownloadProgress::~DownloadProgress()
{
}

vector<uint8_t> DownloadProgress::serialize()
{
    vector<uint8_t> data;

    // Serialize basic properties
    data.insert(data.end(), (uint8_t *)&completed, (uint8_t *)&completed + sizeof(completed));
    data.insert(data.end(), (uint8_t *)&totalDownloadBytes, (uint8_t *)&totalDownloadBytes + sizeof(totalDownloadBytes));
    data.insert(data.end(), (uint8_t *)&fileSize, (uint8_t *)&fileSize + sizeof(fileSize));
    data.insert(data.end(), (uint8_t *)&startTime, (uint8_t *)&startTime + sizeof(startTime));
    data.insert(data.end(), (uint8_t *)&lastTime, (uint8_t *)&lastTime + sizeof(lastTime));
    data.insert(data.end(), fileHash.data(), fileHash.data() + fileHash.size());

    // Serialize pieces
    uint32_t piecesCount = pieces.size();
    data.insert(data.end(), (uint8_t *)&piecesCount, (uint8_t *)&piecesCount + sizeof(piecesCount));

    for (const auto &piece : pieces)
    {
        auto pieceData = piece.serialize();
        data.insert(data.end(), pieceData.begin(), pieceData.end());
    }

    return data;
}

void DownloadProgress::deserialize(vector<uint8_t> data)
{
    size_t offset = 0;

    // Deserialize basic properties
    memcpy(&completed, data.data() + offset, sizeof(completed));
    offset += sizeof(completed);

    memcpy(&totalDownloadBytes, data.data() + offset, sizeof(totalDownloadBytes));
    offset += sizeof(totalDownloadBytes);

    memcpy(&fileSize, data.data() + offset, sizeof(fileSize));
    offset += sizeof(fileSize);

    memcpy(&startTime, data.data() + offset, sizeof(startTime));
    offset += sizeof(startTime);

    memcpy(&lastTime, data.data() + offset, sizeof(lastTime));
    offset += sizeof(lastTime);

    memcpy(fileHash.data(), data.data() + offset, fileHash.size());
    offset += fileHash.size();

    // Deserialize pieces
    uint32_t piecesCount;
    memcpy(&piecesCount, data.data() + offset, sizeof(piecesCount));
    offset += sizeof(piecesCount);

    pieces.clear();
    for (uint32_t i = 0; i < piecesCount; i++)
    {
        pieces.push_back(PieceProgress::deserialize(data, offset));
    }
}

void DownloadProgress::init(MetaDataFile &metaData)
{
    using namespace Utils;
    startTime = time(nullptr);
    lastTime = startTime;
    fileSize = metaData.getFileSize();
    totalDownloadBytes = 0;
    completed = false;
    fileHash = metaData.getFileHash();
    auto temp = fileSize;
    uint32_t pieceSize = FileSplitter::pieceSize(temp);
    int i = 0;
    while (temp > 0)
    {
        PieceProgress piece(fileSize - temp, temp > pieceSize ? pieceSize : temp, metaData.getPartsHashes()[i]);
        temp -= piece.size;
        pieces.push_back(piece);
    }
}

PieceProgress::PieceProgress(uint64_t offset, uint32_t size, HashResult hash)
{
    {
        this->offset = offset;
        this->size = size;
        this->bytesDownloaded = 0;
        lastAccess = time(nullptr);
        status = DownloadStatus::Empty;
        blocks = vector<BlockInfo>();
        auto temp = size;
        this->hash = hash;
        while (temp > 0)
        {
            BlockInfo block;
            block.offset = size - temp;
            block.size = Utils::FileSplitter::BLOCK_SIZE < temp ? Utils::FileSplitter::BLOCK_SIZE : temp;
            temp -= block.size;
            block.isLastBlock = temp == 0;
            blocks.push_back(block);
        }
    }
}

vector<uint8_t> BlockInfo::serialize() const
{
    vector<uint8_t> data;
    data.insert(data.end(), (uint8_t *)&offset, (uint8_t *)&offset + sizeof(offset));
    data.insert(data.end(), (uint8_t *)&size, (uint8_t *)&size + sizeof(size));
    data.insert(data.end(), (uint8_t *)&isLastBlock, (uint8_t *)&isLastBlock + sizeof(isLastBlock));
    data.insert(data.end(), (uint8_t *)&status, (uint8_t *)&status + sizeof(status));
    return data;
}

BlockInfo BlockInfo::deserialize(const vector<uint8_t> &data, size_t &offset)
{
    BlockInfo block;
    memcpy(&block.offset, data.data() + offset, sizeof(block.offset));
    offset += sizeof(block.offset);

    memcpy(&block.size, data.data() + offset, sizeof(block.size));
    offset += sizeof(block.size);

    memcpy(&block.isLastBlock, data.data() + offset, sizeof(block.isLastBlock));
    offset += sizeof(block.isLastBlock);

    memcpy(&block.status, data.data() + offset, sizeof(block.status));
    offset += sizeof(block.status);

    return block;
}

vector<uint8_t> PieceProgress::serialize() const
{
    vector<uint8_t> data;

    // Serialize piece properties
    data.insert(data.end(), (uint8_t *)&offset, (uint8_t *)&offset + sizeof(offset));
    data.insert(data.end(), (uint8_t *)&size, (uint8_t *)&size + sizeof(size));
    data.insert(data.end(), (uint8_t *)&bytesDownloaded, (uint8_t *)&bytesDownloaded + sizeof(bytesDownloaded));
    data.insert(data.end(), (uint8_t *)&lastAccess, (uint8_t *)&lastAccess + sizeof(lastAccess));
    data.insert(data.end(), (uint8_t *)&status, (uint8_t *)&status + sizeof(status));
    data.insert(data.end(), hash.data(), hash.data() + hash.size());

    // Serialize blocks
    uint32_t blocksCount = blocks.size();
    data.insert(data.end(), (uint8_t *)&blocksCount, (uint8_t *)&blocksCount + sizeof(blocksCount));

    for (const auto &block : blocks)
    {
        auto blockData = block.serialize();
        data.insert(data.end(), blockData.begin(), blockData.end());
    }

    return data;
}

PieceProgress PieceProgress::deserialize(const vector<uint8_t> &data, size_t &vecOffset)
{
    uint64_t offset;
    uint32_t size;
    HashResult hash;

    memcpy(&offset, data.data() + vecOffset, sizeof(offset));
    vecOffset += sizeof(offset);

    memcpy(&size, data.data() + vecOffset, sizeof(size));
    vecOffset += sizeof(size);

    // Skip bytesDownloaded for constructor
    uint32_t bytesDownloaded;
    memcpy(&bytesDownloaded, data.data() + vecOffset, sizeof(bytesDownloaded));
    vecOffset += sizeof(bytesDownloaded);

    // Skip lastAccess for constructor
    time_t lastAccess;
    memcpy(&lastAccess, data.data() + vecOffset, sizeof(lastAccess));
    vecOffset += sizeof(lastAccess);

    // Skip status for constructor
    DownloadStatus status;
    memcpy(&status, data.data() + vecOffset, sizeof(status));
    vecOffset += sizeof(status);

    memcpy(hash.data(), data.data() + vecOffset, hash.size());
    vecOffset += hash.size();

    // Create the piece with constructor
    PieceProgress piece(offset, size, hash);
    piece.bytesDownloaded = bytesDownloaded;
    piece.lastAccess = lastAccess;
    piece.status = status;

    // Deserialize blocks
    uint32_t blocksCount;
    memcpy(&blocksCount, data.data() + vecOffset, sizeof(blocksCount));
    vecOffset += sizeof(blocksCount);

    piece.blocks.clear();
    for (uint32_t i = 0; i < blocksCount; i++)
    {
        piece.blocks.push_back(BlockInfo::deserialize(data, vecOffset));
    }

    return piece;
}
