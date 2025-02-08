#include "DownloadProgress.h"

DownloadProgress::DownloadProgress(const MetaDataFile &metaData) {
    init(metaData);
}

DownloadProgress::DownloadProgress(const DownloadProgress &downloadProgress) : fileName(downloadProgress.fileName),
                                                                               creator(downloadProgress.creator),
                                                                               fileHash(downloadProgress.fileHash),
                                                                               completed(downloadProgress.completed),
                                                                               totalDownloadBytes(
                                                                                   downloadProgress.totalDownloadBytes),
                                                                               fileSize(downloadProgress.fileSize),
                                                                               startTime(downloadProgress.startTime),
                                                                               lastTime(downloadProgress.lastTime),
                                                                               pieces(downloadProgress.pieces) {
}

DownloadProgress &DownloadProgress::operator=(const MetaDataFile &meta) {
    init(meta);
    return *this;
}

DownloadProgress &DownloadProgress::operator=(DownloadProgress other) {
    swap(*this, other); // Reuse the swap function
    return *this;
}

void swap(DownloadProgress &first, DownloadProgress &second) noexcept {
    using std::swap;
    swap(first.fileName, second.fileName);
    swap(first.creator, second.creator);
    swap(first.fileHash, second.fileHash);
    swap(first.completed, second.completed);
    swap(first.totalDownloadBytes, second.totalDownloadBytes);
    swap(first.fileSize, second.fileSize);
    swap(first.startTime, second.startTime);
    swap(first.lastTime, second.lastTime);
    swap(first.pieces, second.pieces);
}

DownloadProgress::DownloadProgress(const vector<uint8_t> &data) {
    deserialize(data);
}

DownloadProgress::~DownloadProgress()
= default;

vector<uint8_t> DownloadProgress::serialize() {
    std::lock_guard<mutex> guard(mut);
    vector<uint8_t> data;

    // Serialize basic properties
    data.insert(data.end(), reinterpret_cast<uint8_t *>(&completed),
                reinterpret_cast<uint8_t *>(&completed) + sizeof(completed));
    data.insert(data.end(), reinterpret_cast<uint8_t *>(&totalDownloadBytes),
                reinterpret_cast<uint8_t *>(&totalDownloadBytes) + sizeof(totalDownloadBytes));
    data.insert(data.end(), reinterpret_cast<uint8_t *>(&fileSize),
                reinterpret_cast<uint8_t *>(&fileSize) + sizeof(fileSize));
    data.insert(data.end(), reinterpret_cast<uint8_t *>(&startTime),
                reinterpret_cast<uint8_t *>(&startTime) + sizeof(startTime));
    data.insert(data.end(), reinterpret_cast<uint8_t *>(&lastTime),
                reinterpret_cast<uint8_t *>(&lastTime) + sizeof(lastTime));
    data.insert(data.end(), fileHash.data(), fileHash.data() + fileHash.size());
    uint8_t nameSize = fileName.size();
    data.insert(data.end(), (uint8_t *) &nameSize, (uint8_t *) &nameSize + sizeof(nameSize));
    data.insert(data.end(), fileName.c_str(), fileName.c_str() + fileName.size());
    nameSize = creator.size();
    data.insert(data.end(), (uint8_t *) &nameSize, (uint8_t *) &nameSize + sizeof(nameSize));
    data.insert(data.end(), creator.c_str(), creator.c_str() + creator.size());

    // Serialize pieces
    uint32_t piecesCount = pieces.size();
    data.insert(data.end(), reinterpret_cast<uint8_t *>(&piecesCount),
                reinterpret_cast<uint8_t *>(&piecesCount) + sizeof(piecesCount));

    for (const auto &piece: pieces) {
        auto pieceData = piece.serialize();
        data.insert(data.end(), pieceData.begin(), pieceData.end());
    }

    return data;
}

void DownloadProgress::deserialize(vector<uint8_t> data) {
    std::lock_guard<mutex> guard(mut);

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

    uint8_t fileNameSize;
    memcpy(&fileNameSize, data.data() + offset, sizeof(fileNameSize));
    offset += sizeof(fileNameSize);
    fileName = string(data.data() + offset, data.data() + offset + fileNameSize);
    offset += fileNameSize;
    memcpy(&fileNameSize, data.data() + offset, sizeof(fileNameSize));
    offset += sizeof(fileNameSize);
    creator = string(data.data() + offset, data.data() + offset + fileNameSize);
    offset += fileNameSize;

    // Deserialize pieces
    uint32_t piecesCount;
    memcpy(&piecesCount, data.data() + offset, sizeof(piecesCount));
    offset += sizeof(piecesCount);

    pieces.clear();
    for (uint32_t i = 0; i < piecesCount; i++) {
        pieces.push_back(PieceProgress::deserialize(data, offset));
    }
}

vector<std::bitset<8> > DownloadProgress::getBitField() const {
    vector<std::bitset<8> > bitField;
    for (int i = 0; i < pieces.size(); ++i) {
        if (i % 8 == 0) {
            bitField.push_back(std::bitset<8>());
        }
        bitField.back()[i % 8] = pieces[i].status == DownloadStatus::Verified;
    }
    return bitField;
}

double DownloadProgress::progress() const {
    std::lock_guard<std::mutex> guard(mut);
    return totalDownloadBytes / static_cast<double>(fileSize);
}

bool DownloadProgress::downloadedBlock(const uint32_t piece, const uint16_t block) {
    std::lock_guard<mutex> guard(mut);

    if (piece >= pieces.size())
        throw std::out_of_range("No piece #" + piece);

    // Update last access time
    lastTime = time(nullptr);

    totalDownloadBytes += pieces[piece].downloadedBlock(block);
    return pieces[piece].status == DownloadStatus::Downloaded;
}

void DownloadProgress::updatePieceStatus(const uint32_t piece, const DownloadStatus status) {
    std::lock_guard<mutex> guard(mut);

    if (piece >= pieces.size())
        throw std::out_of_range("No piece #" + piece);

    // Update last access time
    lastTime = time(nullptr);

    totalDownloadBytes += pieces[piece].setStatus(status);
}

void DownloadProgress::updateBlockStatus(const uint32_t piece, const uint16_t block, const DownloadStatus status) {
    std::lock_guard<std::mutex> guard(mut);
    if (piece >= pieces.size())
        throw std::out_of_range("No piece #" + piece);

    // Update last access time
    lastTime = time(nullptr);

    pieces[piece].updateBlockStatus(block, status);
}

vector<PieceProgress> DownloadProgress::getPiecesStatused(const DownloadStatus status) {
    std::lock_guard<std::mutex> guard(mut);
    vector<PieceProgress> pieces;

    for (const auto &piece: this->pieces) {
        if (piece.status == status) {
            pieces.push_back(piece);
        }
    }

    return pieces;
}


PieceProgress DownloadProgress::getPiece(const uint32_t index) const {
    std::lock_guard<mutex> guard(mut);

    if (index >= pieces.size())
        throw std::out_of_range("No piece #" + index);
    return pieces[index];
}

uint DownloadProgress::getPieceIndex(const uint64_t offset) const {
    return offset / Utils::FileSplitter::pieceSize(fileSize);
}


void DownloadProgress::init(const MetaDataFile &metaData) {
    using namespace Utils;
    fileName = metaData.getFileName();
    creator = metaData.getCreator();
    startTime = time(nullptr);
    lastTime = startTime;
    fileSize = metaData.getFileSize();
    totalDownloadBytes = 0;
    completed = false;
    fileHash = metaData.getFileHash();
    auto temp = fileSize;
    const uint32_t pieceSize = FileSplitter::pieceSize(temp);
    int i = 0;
    while (temp > 0) {
        PieceProgress piece(fileSize - temp, temp > pieceSize ? pieceSize : temp, metaData.getPartsHashes()[i]);
        temp -= piece.size;
        pieces.push_back(piece);
        i++;
    }
}

PieceProgress::PieceProgress(const uint64_t offset, const uint32_t size, const HashResult &hash) : offset(offset),
                                                                                                   size(size),
                                                                                                   hash(hash) {
    this->bytesDownloaded = 0;
    lastAccess = time(nullptr);
    status = DownloadStatus::Empty;
    blocks = vector<BlockInfo>();
    auto temp = size;
    this->hash = hash;
    while (temp > 0) {
        BlockInfo block{};
        block.offset = size - temp;
        block.size = Utils::FileSplitter::BLOCK_SIZE < temp ? Utils::FileSplitter::BLOCK_SIZE : temp;
        temp -= block.size;
        block.isLastBlock = temp == 0;
        blocks.push_back(block);
    }
}

vector<uint8_t> BlockInfo::serialize() const {
    vector<uint8_t> data;
    data.insert(data.end(), (uint8_t *) &offset, (uint8_t *) &offset + sizeof(offset));
    data.insert(data.end(), (uint8_t *) &size, (uint8_t *) &size + sizeof(size));
    data.insert(data.end(), (uint8_t *) &isLastBlock, (uint8_t *) &isLastBlock + sizeof(isLastBlock));
    data.insert(data.end(), (uint8_t *) &status, (uint8_t *) &status + sizeof(status));
    return data;
}

BlockInfo BlockInfo::deserialize(const vector<uint8_t> &data, size_t &offset) {
    BlockInfo block{};
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

uint16_t PieceProgress::downloadedBlock(const uint16_t block) {
    if (block >= blocks.size()) {
        throw std::out_of_range("No block # " + block);
    }
    // Update last access time
    lastAccess = time(nullptr);

    blocks[block].status = DownloadStatus::Downloaded;
    if (allBlocksDownloaded()) {
        status = DownloadStatus::Downloaded;
    }
    return blocks[block].size;
}


bool PieceProgress::allBlocksDownloaded() const {
    for (auto &&i: blocks) {
        if (i.status != DownloadStatus::Downloaded) {
            return false;
        }
    }

    return true;
}

int32_t PieceProgress::setStatus(const DownloadStatus status) {
    int32_t sizeChanged = 0;
    const bool toRemove = status == DownloadStatus::Empty && this->status != DownloadStatus::Empty;
    const bool toAdd = (status == DownloadStatus::Verified || status == DownloadStatus::Downloaded) && (
                           this->status == DownloadStatus::Empty || this->status == DownloadStatus::Downloading);
    // Update last access time
    lastAccess = time(nullptr);

    this->status = status;
    if (status != DownloadStatus::Downloading) {
        for (auto &&i: blocks) {
            if (toRemove && (i.status == DownloadStatus::Downloaded || i.status == DownloadStatus::Verified)) {
                sizeChanged -= i.size;
            }
            if (toAdd && (i.status == DownloadStatus::Empty || i.status == DownloadStatus::Downloading)) {
                sizeChanged += i.size;
            }
            i.status = status;
        }
    }
    return sizeChanged;
}

void PieceProgress::updateBlockStatus(const uint16_t block, const DownloadStatus status) {
    if (block >= blocks.size())
        throw std::out_of_range("No block #" + block);

    // Update last access time
    lastAccess = time(nullptr);

    blocks[block].status = status;
    if (status == DownloadStatus::Downloaded && allBlocksDownloaded()) {
        this->status = DownloadStatus::Downloaded;
    }
    if (status == DownloadStatus::Downloading) {
        this->status = DownloadStatus::Downloading;
    }
}

BlockInfo PieceProgress::getBlockInfo(const uint16_t block) const {
    if (block >= blocks.size())
        throw std::out_of_range("No block #" + block);
    return blocks[block];
}

vector<BlockInfo> PieceProgress::getBlocksStatused(const DownloadStatus status) const {
    vector<BlockInfo> relevantBlcoks;
    for (auto &&i: blocks) {
        if (i.status == status) {
            relevantBlcoks.push_back(i);
        }
    }
    return relevantBlcoks;
}

uint16_t PieceProgress::getBlockIndex(const uint64_t offset) {
    return offset / Utils::FileSplitter::BLOCK_SIZE;
}

vector<uint8_t> PieceProgress::serialize() const {
    vector<uint8_t> data;

    // Serialize piece properties
    data.insert(data.end(), (uint8_t *) &offset, (uint8_t *) &offset + sizeof(offset));
    data.insert(data.end(), (uint8_t *) &size, (uint8_t *) &size + sizeof(size));
    data.insert(data.end(), (uint8_t *) &bytesDownloaded, (uint8_t *) &bytesDownloaded + sizeof(bytesDownloaded));
    data.insert(data.end(), (uint8_t *) &lastAccess, (uint8_t *) &lastAccess + sizeof(lastAccess));
    data.insert(data.end(), (uint8_t *) &status, (uint8_t *) &status + sizeof(status));
    data.insert(data.end(), hash.data(), hash.data() + hash.size());

    // Serialize blocks
    uint32_t blocksCount = blocks.size();
    data.insert(data.end(), (uint8_t *) &blocksCount, (uint8_t *) &blocksCount + sizeof(blocksCount));

    for (const auto &block: blocks) {
        auto blockData = block.serialize();
        data.insert(data.end(), blockData.begin(), blockData.end());
    }

    return data;
}

PieceProgress PieceProgress::deserialize(const vector<uint8_t> &data, size_t &vecOffset) {
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
    for (uint32_t i = 0; i < blocksCount; i++) {
        piece.blocks.push_back(BlockInfo::deserialize(data, vecOffset));
    }

    return piece;
}
