//
// Created by user on 12/14/24.
//

#ifndef BITTORRENTMESSAGES_H
#define BITTORRENTMESSAGES_H
#include "Messages.h"
#include "../../Encryptions/AES/AESHandler.h"


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

enum BitTorrentResponseCodes {
	keepAliveRes = 102,
	bitField = 112,
	missingFile = 113,
	dataResponse = 123,
	missingDataResponse = 124
};

struct TorrentMessageBase : MessageBaseToSend, GeneralRecieve {
    ID fileId{};
    HashResult hash{};
    AESKey initVector{};
    static constexpr uint SIZE = SHA256_SIZE * 2 + BLOCK;

	explicit TorrentMessageBase(const MessageBaseReceived &msg) : GeneralRecieve(msg.from) {
		TorrentMessageBase::deserialize(msg.data);
	}

    [[nodiscard]] vector<uint8_t> serialize(const uint32_t PreviousSize = 0) const override {
        auto baseSerialized = MessageBaseToSend::serialize(PreviousSize + SIZE);
        SerializeDeserializeUtils::addToEnd(baseSerialized, fileId);
        SerializeDeserializeUtils::addToEnd(baseSerialized, hash);
        SerializeDeserializeUtils::addToEnd(baseSerialized, initVector);

        return baseSerialized;
    }


    virtual uint deserialize(const vector<uint8_t> &data) {
        memcpy(fileId.data(), data.data(), SHA256_SIZE);
        memcpy(hash.data(), data.data() + SHA256_SIZE, SHA256_SIZE);
        memcpy(initVector.data(), data.data() + SIZE - initVector.size(), initVector.size());
        return SIZE;
    }

	void UpdateHash(const vector<uint8_t> &fullData) {
		const vector<uint8_t> data(fullData.begin() + SIZE, fullData.end());
		hash = SHA256::toHashSha256(data);
		memcpy(hash.data(), data.data() + SHA256_SIZE, SHA256_SIZE);
	}

	bool verifyHash(const vector<uint8_t> &fullData) {
		const vector<uint8_t> data(fullData.begin() + SIZE, fullData.end());
		HashResult computedhash = SHA256::toHashSha256(data);
		memcpy(hash.data(), data.data() + SHA256_SIZE, SHA256_SIZE);
		return hash == computedhash;
	}
};

//////////////////////////////////////////////
///	BitTorrent requests //////////////////////
//////////////////////////////////////////////

struct DataRequest : TorrentMessageBase {
	uint32_t pieceIndex{};
	uint16_t blockIndex{};
	uint16_t blocksCount{};

	explicit DataRequest(const MessageBaseReceived &msg) : TorrentMessageBase(msg, true) {
		DataRequest::deserialize(msg.data);
	}

	DataRequest(const ID &fileId, const AESKey &initVector, const uint32_t pieceIndex, const uint16_t blockIndex,
	            const uint16_t blocksCount) : TorrentMessageBase(fileId, initVector,
	                                                             BitTorrentRequestCodes::dataRequest) {
		DataRequest::pieceIndex = pieceIndex;
		DataRequest::blockIndex = blockIndex;
		DataRequest::blocksCount = blocksCount;
	}

	[[nodiscard]] vector<uint8_t> serialize(const uint32_t PreviousSize = 0) const override {
		auto baseSerialized = TorrentMessageBase::serialize(
			PreviousSize + sizeof(pieceIndex) + sizeof(blockIndex) + sizeof(blocksCount));
		SerializeDeserializeUtils::addToEnd(baseSerialized, pieceIndex);
		SerializeDeserializeUtils::addToEnd(baseSerialized, blockIndex);
		SerializeDeserializeUtils::addToEnd(baseSerialized, blocksCount);

		return baseSerialized;
	}

	uint deserialize(const vector<uint8_t> &data) override {
		const auto offset = TorrentMessageBase::deserialize(data);
		memcpy(&pieceIndex, data.data() + offset, sizeof(pieceIndex));
		memcpy(&blockIndex, data.data() + offset + sizeof(pieceIndex), sizeof(blockIndex));
		memcpy(&blocksCount, data.data() + offset + sizeof(pieceIndex) + sizeof(blockIndex), sizeof(blocksCount));

		return offset + sizeof(pieceIndex) + sizeof(blockIndex) + sizeof(blocksCount);
	}
};

struct CancelDataRequest : TorrentMessageBase {
	uint32_t pieceIndex{};
	uint16_t blockIndex{};
	uint16_t blocksCount{};

	explicit CancelDataRequest(const MessageBaseReceived &msg) : TorrentMessageBase(msg, true) {
		CancelDataRequest::deserialize(msg.data);
	}

	CancelDataRequest(const ID &fileId, const AESKey &initVector, const uint32_t pieceIndex, const uint16_t blockIndex,
	                  const uint16_t blocksCount) : TorrentMessageBase(fileId, initVector,
	                                                                   BitTorrentRequestCodes::cancelDataRequest) {
		CancelDataRequest::pieceIndex = pieceIndex;
		CancelDataRequest::blockIndex = blockIndex;
		CancelDataRequest::blocksCount = blocksCount;
	}

	[[nodiscard]] vector<uint8_t> serialize(const uint32_t PreviousSize = 0) const override {
		auto baseSerialized = TorrentMessageBase::serialize(
			PreviousSize + sizeof(pieceIndex) + sizeof(blockIndex) + sizeof(blocksCount));
		SerializeDeserializeUtils::addToEnd(baseSerialized, pieceIndex);
		SerializeDeserializeUtils::addToEnd(baseSerialized, blockIndex);
		SerializeDeserializeUtils::addToEnd(baseSerialized, blocksCount);

		return baseSerialized;
	}

	uint deserialize(const vector<uint8_t> &data) override {
		const auto offset = TorrentMessageBase::deserialize(data);
		memcpy(&pieceIndex, data.data() + offset, sizeof(pieceIndex));
		memcpy(&blockIndex, data.data() + offset + sizeof(pieceIndex), sizeof(blockIndex));
		memcpy(&blocksCount, data.data() + offset + sizeof(pieceIndex) + sizeof(blockIndex), sizeof(blocksCount));

		return offset + sizeof(pieceIndex) + sizeof(blockIndex) + sizeof(blocksCount);
	}
};

//////////////////////////////////////////////
///	BitTorrent requests //////////////////////
//////////////////////////////////////////////


struct BlockResponse : TorrentMessageBase {
	uint32_t pieceIndex{};
	uint16_t blockIndex{};
	vector<uint8_t> block;

	explicit BlockResponse(const MessageBaseReceived &msg) : TorrentMessageBase(msg, true) {
		BlockResponse::deserialize(msg.data);
	}

	BlockResponse(const ID &fileId, const AESKey &initVector, const uint32_t pieceIndex, const uint16_t blockIndex,
	              const vector<uint8_t> &block) : TorrentMessageBase(fileId, initVector,
	                                                                 BitTorrentResponseCodes::dataResponse) {
		BlockResponse::pieceIndex = pieceIndex;
		BlockResponse::blockIndex = blockIndex;
		BlockResponse::block = block;
	}

	[[nodiscard]] vector<uint8_t> serialize(const uint32_t PreviousSize = 0) const override {
		auto baseSerialized = TorrentMessageBase::serialize(
			PreviousSize + sizeof(pieceIndex) + sizeof(blockIndex) + sizeof(uint) + block.size());
		SerializeDeserializeUtils::addToEnd(baseSerialized, pieceIndex);
		SerializeDeserializeUtils::addToEnd(baseSerialized, blockIndex);
		SerializeDeserializeUtils::addToEnd(baseSerialized, static_cast<uint>(block.size()));
		SerializeDeserializeUtils::addToEnd(baseSerialized, block);

		return baseSerialized;
	}

	uint deserialize(const vector<uint8_t> &data) override {
		//implement deserialize
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


#endif //BITTORRENTMESSAGES_H
