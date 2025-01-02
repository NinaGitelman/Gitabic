//
// Created by user on 12/14/24.
//

#ifndef BITTORRENTMESSAGES_H
#define BITTORRENTMESSAGES_H
#include <bitset>

#include "Messages.h"
#include "../../Encryptions/AES/AESHandler.h"
#include <memory>

/**
 * @enum BitTorrentRequestCodes
 * @brief Enum representing the request codes for BitTorrent messages.
 */
enum BitTorrentRequestCodes {
 keepAlive = 101,
 fileInterested = 110,
 fileNotInterested = 111,
 dataRequest = 121,
 cancelDataRequest = 122,
 choke = 130,
 unchoke = 131,
 hasPieceUpdate = 140,
 lostPieceUpdate = 141
};

/**
 * @enum BitTorrentResponseCodes
 * @brief Enum representing the response codes for BitTorrent messages.
 */
enum BitTorrentResponseCodes {
 keepAliveRes = 102,
 bitField = 112,
 missingFile = 113,
 dataResponse = 123,
 missingDataResponse = 124
};

/**
 * @struct TorrentMessageBase
 * @brief Base structure for BitTorrent messages.
 */
struct TorrentMessageBase : MessageBaseToSend, GeneralRecieve {
 ID fileId{}; ///< File ID.
 HashResult hash{}; ///< Hash result.
 AESKey initVector{}; ///< AES initialization vector.
 static constexpr uint SIZE = SHA256_SIZE * 2 + BLOCK; ///< Size of the message.

 /**
  * @brief Constructor with file ID and initialization vector.
  * @param fileId The file ID.
  * @param initVector The AES initialization vector.
  * @param code The message code.
  */
 TorrentMessageBase(const ID &fileId, const AESKey &initVector, const uint8_t code) : MessageBaseToSend(
                                                                                       code), GeneralRecieve(),
                                                                                      fileId(fileId),
                                                                                      initVector(initVector) {
 }

 /**
  * @brief Constructor with received message.
  * @param msg The received message.
  * @param code The message code.
  * @param isDeserialized Flag indicating if the message is deserialized.
  */
 TorrentMessageBase(const MessageBaseReceived &msg, const uint8_t code,
                    const bool isDeserialized = false) : GeneralRecieve(
  msg.from) {
  if (!isDeserialized) {
   TorrentMessageBase::deserialize(msg.data);
  }
 }

 /**
  * @brief Serialize the message.
  * @param PreviousSize The previous size.
  * @return The serialized message.
  */
 [[nodiscard]] vector<uint8_t> serialize(const uint32_t PreviousSize = 0) const override {
  auto baseSerialized = MessageBaseToSend::serialize(PreviousSize + SIZE);
  SerializeDeserializeUtils::copyToEnd(baseSerialized, fileId);
  SerializeDeserializeUtils::addToEnd(baseSerialized, hash);
  SerializeDeserializeUtils::addToEnd(baseSerialized, initVector);

  return baseSerialized;
 }

 /**
  * @brief Deserialize the message.
  * @param data The data to deserialize.
  * @return The size of the deserialized data.
  */
 virtual uint deserialize(const vector<uint8_t> &data) {
  memcpy(fileId.data(), data.data(), SHA256_SIZE);
  memcpy(hash.data(), data.data() + SHA256_SIZE, SHA256_SIZE);
  memcpy(initVector.data(), data.data() + SIZE - initVector.size(), initVector.size());
  return SIZE;
 }

 /**
  * @brief Update the hash with the given data.
  * @param fullData The full data to update the hash with.
  */
 static void UpdateHash(const vector<uint8_t> &fullData) {
  const vector<uint8_t> data(fullData.begin() + SIZE, fullData.end());
  auto hash = SHA256::toHashSha256(data);
  memcpy(hash.data(), data.data() + SHA256_SIZE, SHA256_SIZE);
 }

 /**
  * @brief Verify the hash with the given data.
  * @param fullData The full data to verify the hash with.
  * @return True if the hash is verified, false otherwise.
  */
 bool verifyHash(const vector<uint8_t> &fullData) {
  const vector<uint8_t> data(fullData.begin() + SIZE, fullData.end());
  const HashResult computedhash = SHA256::toHashSha256(data);
  memcpy(hash.data(), data.data() + SHA256_SIZE, SHA256_SIZE);
  return hash == computedhash;
 }
};

struct ResultMessages {
 ID to;
 vector<std::shared_ptr<TorrentMessageBase> > messages;
};

//////////////////////////////////////////////
///	BitTorrent requests //////////////////////
//////////////////////////////////////////////

/**
 * @struct DataRequest
 * @brief Structure for BitTorrent data request messages.
 */
struct DataRequest : TorrentMessageBase {
 uint32_t pieceIndex{}; ///< Piece index.
 uint16_t blockIndex{}; ///< Block index.
 uint16_t blocksCount{}; ///< Number of blocks.

 /**
  * @brief Constructor with received message.
  * @param msg The received message.
  */
 explicit DataRequest(const MessageBaseReceived &msg) : TorrentMessageBase(msg, true) {
  DataRequest::deserialize(msg.data);
 }

 /**
  * @brief Constructor with file ID, initialization vector, piece index, block index, and blocks count.
  * @param fileId The file ID.
  * @param initVector The AES initialization vector.
  * @param pieceIndex The piece index.
  * @param blockIndex The block index.
  * @param blocksCount The number of blocks.
  */
 DataRequest(const ID &fileId, const AESKey &initVector, const uint32_t pieceIndex, const uint16_t blockIndex,
             const uint16_t blocksCount) : TorrentMessageBase(fileId, initVector,
                                                              BitTorrentRequestCodes::dataRequest) {
  DataRequest::pieceIndex = pieceIndex;
  DataRequest::blockIndex = blockIndex;
  DataRequest::blocksCount = blocksCount;
 }

 /**
  * @brief Serialize the data request message.
  * @param PreviousSize The previous size.
  * @return The serialized message.
  */
 [[nodiscard]] vector<uint8_t> serialize(const uint32_t PreviousSize = 0) const override {
  auto baseSerialized = TorrentMessageBase::serialize(
   PreviousSize + sizeof(pieceIndex) + sizeof(blockIndex) + sizeof(blocksCount));
  SerializeDeserializeUtils::copyToEnd(baseSerialized, pieceIndex);
  SerializeDeserializeUtils::copyToEnd(baseSerialized, blockIndex);
  SerializeDeserializeUtils::copyToEnd(baseSerialized, blocksCount);

  return baseSerialized;
 }

 /**
  * @brief Deserialize the data request message.
  * @param data The data to deserialize.
  * @return The size of the deserialized data.
  */
 uint deserialize(const vector<uint8_t> &data) override {
  const auto offset = TorrentMessageBase::deserialize(data);
  memcpy(&pieceIndex, data.data() + offset, sizeof(pieceIndex));
  memcpy(&blockIndex, data.data() + offset + sizeof(pieceIndex), sizeof(blockIndex));
  memcpy(&blocksCount, data.data() + offset + sizeof(pieceIndex) + sizeof(blockIndex), sizeof(blocksCount));

  return offset + sizeof(pieceIndex) + sizeof(blockIndex) + sizeof(blocksCount);
 }
};

/**
 * @struct CancelDataRequest
 * @brief Structure for BitTorrent cancel data request messages.
 */
struct CancelDataRequest : TorrentMessageBase {
 uint32_t pieceIndex{}; ///< Piece index.
 uint16_t blockIndex{}; ///< Block index.
 uint16_t blocksCount{}; ///< Number of blocks.

 /**
  * @brief Constructor with received message.
  * @param msg The received message.
  */
 explicit CancelDataRequest(const MessageBaseReceived &msg) : TorrentMessageBase(msg, true) {
  CancelDataRequest::deserialize(msg.data);
 }

 /**
  * @brief Constructor with file ID, initialization vector, piece index, block index, and blocks count.
  * @param fileId The file ID.
  * @param initVector The AES initialization vector.
  * @param pieceIndex The piece index.
  * @param blockIndex The block index.
  * @param blocksCount The number of blocks.
  */
 CancelDataRequest(const ID &fileId, const AESKey &initVector, const uint32_t pieceIndex, const uint16_t blockIndex,
                   const uint16_t blocksCount) : TorrentMessageBase(fileId, initVector,
                                                                    BitTorrentRequestCodes::cancelDataRequest) {
  CancelDataRequest::pieceIndex = pieceIndex;
  CancelDataRequest::blockIndex = blockIndex;
  CancelDataRequest::blocksCount = blocksCount;
 }

 /**
  * @brief Serialize the cancel data request message.
  * @param PreviousSize The previous size.
  * @return The serialized message.
  */
 [[nodiscard]] vector<uint8_t> serialize(const uint32_t PreviousSize = 0) const override {
  auto baseSerialized = TorrentMessageBase::serialize(
   PreviousSize + sizeof(pieceIndex) + sizeof(blockIndex) + sizeof(blocksCount));
  SerializeDeserializeUtils::copyToEnd(baseSerialized, pieceIndex);
  SerializeDeserializeUtils::copyToEnd(baseSerialized, blockIndex);
  SerializeDeserializeUtils::copyToEnd(baseSerialized, blocksCount);

  return baseSerialized;
 }

 /**
  * @brief Deserialize the cancel data request message.
  * @param data The data to deserialize.
  * @return The size of the deserialized data.
  */
 uint deserialize(const vector<uint8_t> &data) override {
  const auto offset = TorrentMessageBase::deserialize(data);
  memcpy(&pieceIndex, data.data() + offset, sizeof(pieceIndex));
  memcpy(&blockIndex, data.data() + offset + sizeof(pieceIndex), sizeof(blockIndex));
  memcpy(&blocksCount, data.data() + offset + sizeof(pieceIndex) + sizeof(blockIndex), sizeof(blocksCount));

  return offset + sizeof(pieceIndex) + sizeof(blockIndex) + sizeof(blocksCount);
 }
};


/**
 * @struct PieceOwnershipUpdate
 * @brief Structure for BitTorrent has piece update, missing requested piece and lost piece update messages.
 */
struct PieceOwnershipUpdate : TorrentMessageBase {
 uint32_t pieceIndex{}; ///< Piece index.

 /**
  * @brief Constructor with received message.
  * @param msg The received message.
  */
 explicit PieceOwnershipUpdate(const MessageBaseReceived &msg) : TorrentMessageBase(msg, true) {
  PieceOwnershipUpdate::deserialize(msg.data);
 }

 /**
  * @brief Constructor with file ID, initialization vector, and piece index.
  * @param fileId The file ID.
  * @param initVector The AES initialization vector.
  * @param pieceIndex The piece index.
  * @param code The message code.
  */
 PieceOwnershipUpdate(const ID &fileId, const AESKey &initVector, const uint32_t pieceIndex,
                      const uint8_t code = BitTorrentRequestCodes::hasPieceUpdate) : TorrentMessageBase(
  fileId,
  initVector,
  code) {
  PieceOwnershipUpdate::pieceIndex = pieceIndex;
 }

 /**
  * @brief Serialize the has piece update message.
  * @param PreviousSize The previous size.
  * @return The serialized message.
  */
 [[nodiscard]] vector<uint8_t> serialize(const uint32_t PreviousSize = 0) const override {
  auto baseSerialized = TorrentMessageBase::serialize(PreviousSize + sizeof(pieceIndex));
  SerializeDeserializeUtils::copyToEnd(baseSerialized, pieceIndex);

  return baseSerialized;
 }

 /**
  * @brief Deserialize the has piece update message.
  * @param data The data to deserialize.
  * @return The size of the deserialized data.
  */
 uint deserialize(const vector<uint8_t> &data) override {
  const auto offset = TorrentMessageBase::deserialize(data);
  memcpy(&pieceIndex, data.data() + offset, sizeof(pieceIndex));

  return offset + sizeof(pieceIndex);
 }
};


//////////////////////////////////////////////
///	BitTorrent responses //////////////////////
//////////////////////////////////////////////

/**
 * @struct BlockResponse
 * @brief Structure for BitTorrent block response messages.
 */
struct BlockResponse : TorrentMessageBase {
 uint32_t pieceIndex{}; ///< Piece index.
 uint16_t blockIndex{}; ///< Block index.
 vector<uint8_t> block; ///< Block data.

 /**
  * @brief Constructor with received message.
  * @param msg The received message.
  */
 explicit BlockResponse(const MessageBaseReceived &msg) : TorrentMessageBase(msg, true) {
  BlockResponse::deserialize(msg.data);
 }

 /**
  * @brief Constructor with file ID, initialization vector, piece index, block index, and block data.
  * @param fileId The file ID.
  * @param initVector The AES initialization vector.
  * @param pieceIndex The piece index.
  * @param blockIndex The block index.
  * @param block The block data.
  */
 BlockResponse(const ID &fileId, const AESKey &initVector, const uint32_t pieceIndex, const uint16_t blockIndex,
               const vector<uint8_t> &block) : TorrentMessageBase(fileId, initVector,
                                                                  BitTorrentResponseCodes::dataResponse) {
  BlockResponse::pieceIndex = pieceIndex;
  BlockResponse::blockIndex = blockIndex;
  BlockResponse::block = block;
 }

 /**
  * @brief Serialize the block response message.
  * @param PreviousSize The previous size.
  * @return The serialized message.
  */
 [[nodiscard]] vector<uint8_t> serialize(const uint32_t PreviousSize = 0) const override {
  auto baseSerialized = TorrentMessageBase::serialize(
   PreviousSize + sizeof(pieceIndex) + sizeof(blockIndex) + sizeof(uint) + block.size());
  SerializeDeserializeUtils::copyToEnd(baseSerialized, pieceIndex);
  SerializeDeserializeUtils::copyToEnd(baseSerialized, blockIndex);
  SerializeDeserializeUtils::copyToEnd(baseSerialized, static_cast<uint>(block.size()));
  SerializeDeserializeUtils::addToEnd(baseSerialized, block);

  return baseSerialized;
 }

 /**
  * @brief Deserialize the block response message.
  * @param data The data to deserialize.
  * @return The size of the deserialized data.
  */
 uint deserialize(const vector<uint8_t> &data) override {
  const auto offset = TorrentMessageBase::deserialize(data);
  memcpy(&pieceIndex, data.data() + offset, sizeof(pieceIndex));
  memcpy(&blockIndex, data.data() + offset + sizeof(pieceIndex), sizeof(blockIndex));
  uint32_t blockSize;
  memcpy(&blockSize, data.data() + offset + sizeof(pieceIndex) + sizeof(blockIndex), sizeof(blockSize));
  block = vector<uint8_t>(data.begin() + offset + sizeof(pieceIndex) + sizeof(blockIndex),
                          data.begin() + offset + sizeof(pieceIndex) + sizeof(blockIndex) + blockSize);
  return offset + sizeof(pieceIndex) + sizeof(blockIndex) + blockSize;
 }
};

struct FileBitField : TorrentMessageBase {
 vector<std::bitset<8> > field{}; ///< Bit field.

 explicit FileBitField(const MessageBaseReceived &msg) : TorrentMessageBase(msg, true) {
  FileBitField::deserialize(msg.data);
 }

 FileBitField(const ID &fileID, const AESKey &initVector, const vector<std::bitset<8> > &field) : TorrentMessageBase(
  fileID, initVector, BitTorrentResponseCodes::bitField) {
  FileBitField::field = field;
 }

 [[nodiscard]] vector<uint8_t> serialize(const uint32_t PreviousSize = 0) const override {
  auto baseSerialized = TorrentMessageBase::serialize(PreviousSize + field.size());
  for (const auto &byte: field) {
   baseSerialized.push_back(byte.to_ulong());
  }

  return baseSerialized;
 }

 uint deserialize(const vector<uint8_t> &data) override {
  const auto offset = TorrentMessageBase::deserialize(data);
  for (size_t i = 0; i < field.size(); ++i) {
   field[i] = std::bitset<8>(data[offset + i]);
  }

  return offset + field.size();
 }
};

#endif //BITTORRENTMESSAGES_H
