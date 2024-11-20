#pragma once

#include <iostream>
#include <vector>
#include <string>
#include "../../Encryptions/SHA256/sha256.h"

using std::vector;
using ID = HashResult;
using EncryptedID = std::array<uint8_t, 48>;
using std::string;

namespace std // Hash method for ID to allow hash map key usage
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

// Codes fro protocol
enum ClientRequestCodes
{
     // Signaling
    GetUserICEInfo = 1,

    // Bit torrent
    Store = 21,
    UserListReq = 23,

    // debugging
    DebuggingStringMessageToSend = 255 
};

enum ClientResponseCodes
{
 // signaling
    ClientResponseAuthorizedICEConnection = 30
};

enum ServerRequestCodes
{

};

enum ServerResponseCodes
{
    // signaling
    UserAuthorizedICEData = 11,

    // bit torrent 
    StoreSuccess = 221,
    StoreFailure,
    UserListRes
};

/// @brief A base struct to send over the internet. Good for status messages, can be inherited for more data
struct MessageBaseToSend
{
    uint8_t code;

    MessageBaseToSend() {}

    MessageBaseToSend(uint8_t code) : code(code) {}

    /// @brief Serializes the data to a vector of bytes
    /// @param PreviousSize the previous size already serialized
    /// @return A byte vector
    virtual vector<uint8_t> serialize(uint32_t PreviousSize = 0) const
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


/// Message to send
/// TODO - Client request: get user ICE info
/// Message: 4 bytes id | 2 bytes iceCandLen | iceCandLen btyes iceCandInfo 


/// TODO - Client Response - ClientResponseAuthorizedICEConnection
/// lenIceCandidateInfo (2 bytes), iceCandidateInfo (lenStudData btyes), requestId (2 bytes)

// just for start debugging, pretify later
struct DebuggingStringMessageToSend : MessageBaseToSend
{


    std::string message;

    DebuggingStringMessageToSend(string message) : message(message), MessageBaseToSend(ClientRequestCodes::DebuggingStringMessageToSend) {}
    
    virtual vector<uint8_t> serialize(uint32_t PreviousSize = 0) const override 
    {
        vector<uint8_t> thisSerialized(this->message.begin(), this->message.end());
        vector<uint8_t> serialized = MessageBaseToSend::serialize(PreviousSize + thisSerialized.size());
        serialized.insert(serialized.end(), thisSerialized.begin(), thisSerialized.end()); // Put the base struct serialization in the start of the vector
        return serialized;
    }

};

/// @brief A request for user list
struct UserListRequest : MessageBaseToSend
{
    UserListRequest(ID fileId) : fileId(fileId), MessageBaseToSend(ClientRequestCodes::UserListReq) {}
    UserListRequest(ID fileId, uint8_t code) : fileId(fileId), MessageBaseToSend(code) {}
    /// @brief The file we want to download
    ID fileId;

    virtual vector<uint8_t> serialize(uint32_t PreviousSize = 0) const override
    {
        vector<uint8_t> thisSerialized(fileId.begin(), fileId.end());
        vector<uint8_t> serialized = MessageBaseToSend::serialize(PreviousSize + thisSerialized.size());
        serialized.insert(serialized.end(), thisSerialized.begin(), thisSerialized.end()); // Put the base struct serialization in the start of the vector
        return serialized;
    }
};

/// @brief A request to anounce you have a file
struct StoreRequest : MessageBaseToSend
{
    StoreRequest(ID fileId, EncryptedID myId) : MessageBaseToSend(ClientRequestCodes::Store), myId(myId), fileId(fileId) {}
    StoreRequest(ID fileId, EncryptedID myId, uint8_t code) : MessageBaseToSend(code), myId(myId), fileId(fileId) {}

    /// @brief Your encrypted ID
    EncryptedID myId;
    /// @brief The file id
    ID fileId;

    virtual vector<uint8_t> serialize(uint32_t previousSize = 0) const override
    {
        vector<uint8_t> serialized = MessageBaseToSend::serialize(previousSize + sizeof(myId));
        vector<uint8_t> thisSerialized(myId.begin(), myId.end());
        vector<uint8_t> thisSerialized2(fileId.begin(), fileId.end());
        // Concatenate all the data int a vector
        serialized.insert(serialized.end(), thisSerialized.begin(), thisSerialized.end());
        serialized.insert(serialized.end(), thisSerialized2.begin(), thisSerialized2.end());
        return serialized;
    }
};

/// @brief A base struct to store a response Packet. good for status response
struct MessageBaseReceived
{
    uint8_t code;
    vector<uint8_t> data;
    MessageBaseReceived() {}

    MessageBaseReceived(uint8_t code, vector<uint8_t> data)
    {
        this->code = code;
        this->data = data;
    }
};



/// @brief A list of users that has a file
struct UserListResponse
{
    vector<EncryptedID> data;
    /// @brief Construct from a MessageBaseReceived data
    /// @param msg the recieved message
    UserListResponse(MessageBaseReceived msg)
    {
        deserialize(msg.data);
    }

    /// @brief Deserializes the data
    /// @param data The data
    void deserialize(vector<uint8_t> data)
    {
        for (size_t i = 0; i < data.size(); i += sizeof(EncryptedID))
        {
            EncryptedID currID = *(EncryptedID *)(data.data() + i);
            this->data.push_back(currID);
        }
    }
};

/// @brief Your assigned id
struct NewIdResponse

{
    ID id;

    NewIdResponse(MessageBaseReceived msg)
    {
        deserialize(msg.data);
    }

    void deserialize(vector<uint8_t> data)
    {
        this->id = *((ID *)data.data());
    }
};




/// MEssage received:
/// TODO Server Response:  UserAuthorizedICEData = 11,
/// Message data:  lenIceCandidateInfo (2 bytes), iceCandidateInfo (lenStudData btyes), 


/// TODO - Server Request
/// Message data:   lenIceCandidateInfo (2 bytes), iceCandidateInfo (lenStudData btyes), requestId (2 bytes) 

