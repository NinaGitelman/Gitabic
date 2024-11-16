#pragma once

#include <iostream>
#include <vector>
#include "../../Encryptions/SHA256/sha256.h"

using std::vector;
using ID = HashResult;
using EncryptedID = std::array<uint8_t, 48>;

namespace std
{
    template <>
    struct hash<ID>
    {
        size_t operator()(const ID &id) const
        {
            size_t res = 0;
            for (size_t i = 0; i < id.size(); i++)
            {
                res ^= hash<uint8_t>()(id[i]) << i;
            }
            return res;
        }
    };
}

enum RequestCodes
{
    Store = 21,
    UserListReq = 23
};
enum ResponseCodes
{
    StoreSuccess = 221,
    StoreFailure,
    UserListRes
};

struct RequestMessageBase
{
    uint8_t code;

    RequestMessageBase() {}

    RequestMessageBase(uint8_t code) : code(code) {}

    virtual vector<uint8_t> serialize(uint32_t PreviousSize) const
    {
        vector<uint8_t> result;
        result.push_back(code);
        for (size_t i = 0; i < sizeof(PreviousSize); i++)
        {
            result.push_back(((uint8_t *)&PreviousSize)[i]);
        }
        return result;
    }
};

struct UserListRequest : RequestMessageBase
{
    UserListRequest(ID fileId) : fileId(fileId), RequestMessageBase(RequestCodes::UserListReq) {}
    UserListRequest(ID fileId, uint8_t code) : fileId(fileId), RequestMessageBase(code) {}
    ID fileId;

    virtual vector<uint8_t> serialize(uint32_t PreviousSize) const override
    {
        vector<uint8_t> thisSerialized(fileId.begin(), fileId.end());
        vector<uint8_t> serialized = RequestMessageBase::serialize(PreviousSize + thisSerialized.size());
        serialized.insert(serialized.end(), thisSerialized.begin(), thisSerialized.end());
        return serialized;
    }
};

struct StoreRequest : RequestMessageBase
{
    StoreRequest(ID fileId, EncryptedID myId) : RequestMessageBase(RequestCodes::Store), myId(myId), fileId(fileId) {}
    StoreRequest(ID fileId, EncryptedID myId, uint8_t code) : RequestMessageBase(code), myId(myId), fileId(fileId) {}

    EncryptedID myId;
    ID fileId;

    virtual vector<uint8_t> serialize(uint32_t previousSize) const override
    {
        vector<uint8_t> serialized = RequestMessageBase::serialize(previousSize + sizeof(myId));
        vector<uint8_t> thisSerialized(myId.begin(), myId.end());
        vector<uint8_t> thisSerialized2(fileId.begin(), fileId.end());
        serialized.insert(serialized.end(), thisSerialized.begin(), thisSerialized.end());
        serialized.insert(serialized.end(), thisSerialized2.begin(), thisSerialized2.end());
        return serialized;
    }
};

struct ResponseMessageBase
{
    uint8_t code;
    vector<uint8_t> data;
    ResponseMessageBase() {}

    ResponseMessageBase(uint8_t code, vector<uint8_t> data)
    {
        this->code = code;
        this->data = data;
    }
};

struct UserListResponse
{
    vector<EncryptedID> data;

    UserListResponse(ResponseMessageBase msg)
    {
        deserialize(msg.data);
    }

    void deserialize(vector<uint8_t> data)
    {
        for (size_t i = 0; i < data.size(); i += sizeof(EncryptedID))
        {
            EncryptedID currID = *(EncryptedID *)(data.data() + i);
            this->data.push_back(currID);
        }
    }
};

struct NewIdResponse
{
    ID id;

    NewIdResponse(ResponseMessageBase msg)
    {
        deserialize(msg.data);
    }

    void deserialize(vector<uint8_t> data)
    {
        this->id = *((ID *)data.data());
    }
};