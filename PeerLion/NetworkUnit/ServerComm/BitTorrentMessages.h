//
// Created by user on 12/14/24.
//

#ifndef BITTORRENTMESSAGES_H
#define BITTORRENTMESSAGES_H
#include "Messages.h"
#include "../../Encryptions/AES/AESHandler.h"

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

#endif //BITTORRENTMESSAGES_H
