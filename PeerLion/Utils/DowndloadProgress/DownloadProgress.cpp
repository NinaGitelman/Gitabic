#include "DownloadProgress.h"

uint64_t DownloadProgress::getPieceOffsetInProgressFile(uint32_t pieceIndex) const {
    constexpr size_t PIECES_OFFSET = 86;
    constexpr size_t PIECE_FIXED_DATA = 12;
    const uint PIECE_SIZE = 61 + _pieces[0].blocks.size() * 8;

    return PIECES_OFFSET + pieceIndex * PIECE_SIZE + PIECE_FIXED_DATA;
}

DownloadProgress::DownloadProgress(const MetaDataFile &metaData) {
    init(metaData);
}

DownloadProgress::DownloadProgress(const DownloadProgress &downloadProgress) : _fileName(downloadProgress._fileName),
                                                                               _creator(downloadProgress._creator),
                                                                               _fileHash(downloadProgress._fileHash),
                                                                               _completed(downloadProgress._completed),
                                                                               _totalDownloadBytes(
                                                                                   downloadProgress.
                                                                                   _totalDownloadBytes),
                                                                               _fileSize(downloadProgress._fileSize),
                                                                               _startTime(downloadProgress._startTime),
                                                                               _lastTime(downloadProgress._lastTime),
                                                                               _pieces(downloadProgress._pieces),
                                                                               _signalingAddress(
                                                                                   downloadProgress._signalingAddress) {
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
    swap(first._fileName, second._fileName);
    swap(first._creator, second._creator);
    swap(first._fileHash, second._fileHash);
    swap(first._completed, second._completed);
    swap(first._totalDownloadBytes, second._totalDownloadBytes);
    swap(first._fileSize, second._fileSize);
    swap(first._startTime, second._startTime);
    swap(first._lastTime, second._lastTime);
    swap(first._pieces, second._pieces);
    swap(first._signalingAddress, second._signalingAddress);
}

DownloadProgress::DownloadProgress(const vector<uint8_t> &data) {
    deserialize(data);
}

DownloadProgress::~DownloadProgress()
= default;

vector<uint8_t> DownloadProgress::serialize() {
    std::lock_guard<mutex> guard(_mut);
    vector<uint8_t> data;

    // Serialize basic properties
    data.insert(data.end(), reinterpret_cast<uint8_t *>(&_completed),
                reinterpret_cast<uint8_t *>(&_completed) + sizeof(_completed));
    data.insert(data.end(), reinterpret_cast<uint8_t *>(&_totalDownloadBytes),
                reinterpret_cast<uint8_t *>(&_totalDownloadBytes) + sizeof(_totalDownloadBytes));
    data.insert(data.end(), reinterpret_cast<uint8_t *>(&_fileSize),
                reinterpret_cast<uint8_t *>(&_fileSize) + sizeof(_fileSize));
    data.insert(data.end(), reinterpret_cast<uint8_t *>(&_startTime),
                reinterpret_cast<uint8_t *>(&_startTime) + sizeof(_startTime));
    data.insert(data.end(), reinterpret_cast<uint8_t *>(&_lastTime),
                reinterpret_cast<uint8_t *>(&_lastTime) + sizeof(_lastTime));
    data.insert(data.end(), _fileHash.data(), _fileHash.data() + _fileHash.size());
    uint8_t nameSize = _fileName.size();
    data.insert(data.end(), (uint8_t *) &nameSize, (uint8_t *) &nameSize + sizeof(nameSize));
    data.insert(data.end(), _fileName.c_str(), _fileName.c_str() + _fileName.size());
    nameSize = _creator.size();
    data.insert(data.end(), (uint8_t *) &nameSize, (uint8_t *) &nameSize + sizeof(nameSize));
    data.insert(data.end(), _creator.c_str(), _creator.c_str() + _creator.size());
    data.insert(data.end(), reinterpret_cast<uint8_t *>(&_signalingAddress),
                reinterpret_cast<uint8_t *>(&_signalingAddress) + sizeof(_signalingAddress));

    // Serialize pieces
    uint32_t piecesCount = _pieces.size();
    data.insert(data.end(), reinterpret_cast<uint8_t *>(&piecesCount),
                reinterpret_cast<uint8_t *>(&piecesCount) + sizeof(piecesCount));

    for (const auto &piece: _pieces) {
        auto pieceData = piece.serialize();
        data.insert(data.end(), pieceData.begin(), pieceData.end());
    }

    return data;
}

void DownloadProgress::deserialize(vector<uint8_t> data) {
    std::lock_guard<mutex> guard(_mut);

    size_t offset = 0;

    // Deserialize basic properties
    memcpy(&_completed, data.data() + offset, sizeof(_completed));
    offset += sizeof(_completed);

    memcpy(&_totalDownloadBytes, data.data() + offset, sizeof(_totalDownloadBytes));
    offset += sizeof(_totalDownloadBytes);

    memcpy(&_fileSize, data.data() + offset, sizeof(_fileSize));
    offset += sizeof(_fileSize);

    memcpy(&_startTime, data.data() + offset, sizeof(_startTime));
    offset += sizeof(_startTime);

    memcpy(&_lastTime, data.data() + offset, sizeof(_lastTime));
    offset += sizeof(_lastTime);

    memcpy(_fileHash.data(), data.data() + offset, _fileHash.size());
    offset += _fileHash.size();

    uint8_t fileNameSize;
    memcpy(&fileNameSize, data.data() + offset, sizeof(fileNameSize));
    offset += sizeof(fileNameSize);
    _fileName = string(data.data() + offset, data.data() + offset + fileNameSize);
    offset += fileNameSize;
    memcpy(&fileNameSize, data.data() + offset, sizeof(fileNameSize));
    offset += sizeof(fileNameSize);
    _creator = string(data.data() + offset, data.data() + offset + fileNameSize);
    offset += fileNameSize;
    _signalingAddress = *reinterpret_cast<Address *>(data.data() + offset);
    offset += sizeof(_signalingAddress);

    // Deserialize pieces
    uint32_t piecesCount;
    memcpy(&piecesCount, data.data() + offset, sizeof(piecesCount));
    offset += sizeof(piecesCount);

    _pieces.clear();
    for (uint32_t i = 0; i < piecesCount; i++) {
        _pieces.push_back(PieceProgress::deserialize(data, offset));
    }
}

vector<std::bitset<8> > DownloadProgress::getBitField() const {
    vector<std::bitset<8> > bitField;
    for (int i = 0; i < _pieces.size(); ++i) {
        if (i % 8 == 0) {
            bitField.push_back(std::bitset<8>());
        }
        bitField.back()[i % 8] = _pieces[i].status == DownloadStatus::Verified;
    }
    return bitField;
}

double DownloadProgress::progress() const {
    std::lock_guard<std::mutex> guard(_mut);
    return _totalDownloadBytes / static_cast<double>(_fileSize);
}

bool DownloadProgress::downloadedBlock(const uint32_t piece, const uint16_t block) {
    std::lock_guard<mutex> guard(_mut);

    if (piece >= _pieces.size())
        throw std::out_of_range("No piece #" + piece);

    // Update last access time
    _lastTime = time(nullptr);

    _totalDownloadBytes += _pieces[piece].downloadedBlock(block);
    return _pieces[piece].status == DownloadStatus::Downloaded;
}

void DownloadProgress::updatePieceStatus(const uint32_t piece, const DownloadStatus status) {
    std::unique_lock<mutex> guard(_mut);

    if (piece >= _pieces.size())
        throw std::out_of_range("No piece #" + piece);

    // Update last access time
    _lastTime = time(nullptr);

    _totalDownloadBytes += _pieces[piece].setStatus(status);
    guard.unlock();
    if (progress() >= 1) {
        _completed = true;
    }
}

void DownloadProgress::updateBlockStatus(const uint32_t piece, const uint16_t block, const DownloadStatus status) {
    std::unique_lock<std::mutex> guard(_mut);
    if (piece >= _pieces.size())
        throw std::out_of_range("No piece #" + piece);

    // Update last access time
    _lastTime = time(nullptr);

    _pieces[piece].updateBlockStatus(block, status);

    guard.unlock();
    if (progress() >= 1) {
        _completed = true;
    }
}

void DownloadProgress::done() {
    for (auto &piece: _pieces) {
        _totalDownloadBytes += piece.setStatus(DownloadStatus::Verified);
    }
    _completed = true;
}


vector<PieceProgress> DownloadProgress::getPiecesStatused(const DownloadStatus status) {
    std::lock_guard<std::mutex> guard(_mut);
    vector<PieceProgress> pieces;

    for (const auto &piece: this->_pieces) {
        if (piece.status == status) {
            pieces.push_back(piece);
        }
    }

    return pieces;
}


PieceProgress DownloadProgress::getPiece(const uint32_t index) const {
    std::lock_guard<mutex> guard(_mut);

    if (index >= _pieces.size())
        throw std::out_of_range("No piece #" + index);
    return _pieces[index];
}

uint DownloadProgress::getPieceIndex(const uint64_t offset) const {
    return offset / Utils::FileSplitter::pieceSize(_fileSize);
}


void DownloadProgress::init(const MetaDataFile &metaData) {
    using namespace Utils;
    _fileName = metaData.getFileName();
    _creator = metaData.getCreator();
    _startTime = time(nullptr);
    _lastTime = _startTime;
    _fileSize = metaData.getFileSize();
    _totalDownloadBytes = 0;
    _completed = false;
    _fileHash = metaData.getFileHash();
    _signalingAddress = metaData.getSignalingAddress();
    auto temp = _fileSize;
    const uint32_t pieceSize = FileSplitter::pieceSize(temp);
    int i = 0;
    while (temp > 0) {
        PieceProgress piece(_fileSize - temp, temp > pieceSize ? pieceSize : temp, metaData.getPartsHashes()[i]);
        temp -= piece.size;
        _pieces.push_back(piece);
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
    data.insert(data.end(), reinterpret_cast<uint8_t *>(&blocksCount),
                reinterpret_cast<uint8_t *>(&blocksCount) + sizeof(blocksCount));

    for (const auto &block: blocks) {
        auto blockData = block.serialize();
        data.insert(data.end(), blockData.begin(), blockData.end());
    }

    return data;
}

vector<uint8_t> PieceProgress::serializeForFileUpdate() const {
    std::vector<uint8_t> data;

    // Serialize only the necessary fields
    data.insert(data.end(), reinterpret_cast<const uint8_t *>(&bytesDownloaded),
                reinterpret_cast<const uint8_t *>(&offset) + sizeof(bytesDownloaded));
    data.insert(data.end(), reinterpret_cast<const uint8_t *>(&status),
                reinterpret_cast<const uint8_t *>(&status) + sizeof(status));
    data.insert(data.end(), reinterpret_cast<const uint8_t *>(&lastAccess),
                reinterpret_cast<const uint8_t *>(&lastAccess) + sizeof(lastAccess));
    data.insert(data.end(), hash.data(), hash.data() + hash.size());

    // Serialize blocks
    uint32_t blocksCount = blocks.size();
    data.insert(data.end(), reinterpret_cast<uint8_t *>(&blocksCount),
                reinterpret_cast<uint8_t *>(&blocksCount) + sizeof(blocksCount));

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
