#pragma once

#include <iostream>
#include <vector>
#include "Encryptions/SHA256/sha256.h"

using std::vector;
using ID = HashResult;
using EncryptedID = std::array<uint8_t, 48>;

enum RequestCodes
{
    Store = 21,
    UserList = 23
};
enum ResponseCodes
{
    StoreSuccess = 221,
    StoreFailure,
    UserList
};

struct RequestMessageBase
{
    uint8_t code;

    RequestMessageBase() {}

    RequestMessageBase(uint8_t code) : code(code) {}

    virtual vector<uint8_t> serialize(uint32_t PreviousSize)
    {
        vector<uint8_t> result;
        result.push_back(code);
        for (size_t i = 0; i < sizeof(PreviousSize); i++)
        {
            result.push_back(((uint8_t *)&PreviousSize)[i]);
        }
    }
};

struct UserListRequest : RequestMessageBase
{
    UserListRequest(ID fileId) : fileId(fileId), RequestMessageBase(RequestCodes::UserList) {}
    UserListRequest(ID fileId, uint8_t code) : fileId(fileId), RequestMessageBase(code) {}
    ID fileId;

    virtual vector<uint8_t> serialize(uint32_t PreviousSize) override
    {
        vector<uint8_t> thisSerialized(fileId.begin(), fileId.end());
        vector<uint8_t> serialized = RequestMessageBase::serialize(PreviousSize + thisSerialized.size());
        serialized.insert(serialized.end(), thisSerialized.begin(), thisSerialized.end());
        return serialized;
    }
};

struct StoreRequest : UserListRequest
{
    StoreRequest(ID fileId, EncryptedID myId) : UserListRequest(fileId, RequestCodes::Store), myId(myId) {}
    StoreRequest(ID fileId, EncryptedID myId, uint8_t code) : UserListRequest(fileId, code), myId(myId) {}
    EncryptedID myId;
    virtual vector<uint8_t> serialize(uint32_t) override
    {
        vector<uint8_t> serialized = UserListRequest::serialize(sizeof(myId));
        vector<uint8_t> thisSerialized(fileId.begin(), fileId.end());
        serialized.insert(serialized.end(), thisSerialized.begin(), thisSerialized.end());
        return serialized;
    }
};

struct ResponseMessageBase
{
    uint32_t size;
    uint8_t code;

    ResponseMessageBase() {}

    ResponseMessageBase(vector<uint8_t> data)
    {
        this->deserialize(data);
    }

    virtual uint16_t deserialize(vector<uint8_t> data)
    {
        uint8_t dataSize = sizeof(code) + sizeof(size);
        if (data.size() < dataSize)
        {
            throw std::runtime_error("Not enough data to deserialize");
        }
        code = data[0];
        for (size_t i = sizeof(code); i < dataSize; i++)
        {
            ((uint8_t *)&size)[i] = data[sizeof(code) + i];
        }
        return dataSize;
    }
};

struct UserListResponse : ResponseMessageBase
{
    vector<EncryptedID> data;

    virtual uint16_t deserialize(vector<uint8_t> data) override
    {
        auto previousSize = ResponseMessageBase::deserialize(data);
        for (size_t i = previousSize; i < this->size; i += sizeof(EncryptedID))
        {
            EncryptedID *currID = (EncryptedID *)(data.data() + i);
            this->data.push_back(*currID);
        }
    }
};