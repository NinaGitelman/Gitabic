#pragma once

#include <iostream>
#include <vector>
#include <string>
#include "../../Encryptions/SHA256/sha256.h"
#include <climits>
#include "../../Utils/SerializeDeserializeUtils.h"

using std::vector;
using ID = HashResult;
using std::string;

// if a message with this code is received it means there isnt any message received by the ICE Connection
#define CODE_NO_MESSAGES_RECEIVED 255

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
enum ClientRequestCodes // 0 - 20 // 255
{
    // no message received
    NoMessageReceived = 0,

    // Signaling
    GetUserICEInfo = 1,

    // Bit torrent
    Store = 21,
    UserListReq = 23,

    // debugging
    DebuggingStringMessage = 255
};

enum ClientResponseCodes // 30 - 40
{
    // signaling
    AuthorizedICEConnection = 30
};

enum ServerRequestCodes
{
    AuthorizeICEConnection = 20
};

enum ServerResponseCodes
{
    // signaling
    UserAuthorizedICEData = 11,

    NewID = 55,

    // bit torrent
    StoreSuccess = 221,
    StoreFailure,
    UserListRes
};

enum ICEConnectionCodes // codes used by ice p2p connection  60-65
{
    Disconnect = 60
};

/// @brief A base struct for the messages sent
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

////////////////////
//// Signaling  ////
///////////////////

/// Message to send
/// TODO - Client request: get user ICE info
/// Message: 32 bytes user id | iceCandLen btyes iceCandInfo
// works!!!!
struct ClientRequestGetUserICEInfo : MessageBaseToSend
{
    ID userId;
    vector<uint8_t> iceCandidateInfo;

    ClientRequestGetUserICEInfo(ID userId, vector<uint8_t> iceCandidateInfo) : MessageBaseToSend(ClientRequestCodes::GetUserICEInfo), userId(userId), iceCandidateInfo(move(iceCandidateInfo)) {}

    vector<uint8_t> serialize(uint32_t PreviousSize = 0) const override
    {

        // if (iceCandidateInfo.size() > USHRT_MAX)
        // {
        //     throw std::runtime_error("Ice candidate is too long (max size - 2 bytes)");
        // }

        vector<uint8_t> serialized = MessageBaseToSend::serialize(PreviousSize + userId.size() + iceCandidateInfo.size());

        SerializeDeserializeUtils::addToEnd(serialized, userId);
        SerializeDeserializeUtils::addToEnd(serialized, iceCandidateInfo);
        return serialized;
    }
};

/// Client Response - ClientResponseAuthorizedICEConnection
/// lenIceCandidateInfo (2 bytes), iceCandidateInfo (lenStudData btyes), requestId (2 bytes)
struct ClientResponseAuthorizedICEConnection : MessageBaseToSend
{
    const int CONST_SIZE = 2;
    vector<uint8_t> iceCandidateInfo;
    uint16_t requestId;

    ClientResponseAuthorizedICEConnection(vector<uint8_t> iceCandidateInfo, uint16_t requestId) : MessageBaseToSend(ClientResponseCodes::AuthorizedICEConnection), iceCandidateInfo(move(iceCandidateInfo)), requestId(requestId) {}

    vector<uint8_t> serialize(uint32_t PreviousSize = 0) const override
    {

        if (iceCandidateInfo.size() > USHRT_MAX)
        {
            throw std::runtime_error("Ice candidate is too long (max size - 2 bytes)");
        }

        // Serialize `requestId` into two bytes vector
        vector<uint8_t> requestIdSerialized(sizeof(uint16_t));
        SerializeDeserializeUtils::serializeUint16IntoVector(requestIdSerialized, requestId);

        // serialize base struct
        vector<uint8_t> serialized = MessageBaseToSend::serialize(PreviousSize + CONST_SIZE + iceCandidateInfo.size());

        SerializeDeserializeUtils::addToEnd(serialized, iceCandidateInfo);
        SerializeDeserializeUtils::addToEnd(serialized, requestIdSerialized);

        return serialized;
    }
};

////////////////////
//// Bit torrent //
/////////////////

// just for start debugging, pretify later
struct DebuggingStringMessageToSend : MessageBaseToSend
{

    std::string message;

    DebuggingStringMessageToSend(string message) : message(message), MessageBaseToSend(ClientRequestCodes::DebuggingStringMessage) {}

    virtual vector<uint8_t> serialize(uint32_t PreviousSize = 0) const override
    {
        vector<uint8_t> thisSerialized(this->message.begin(), this->message.end());
        vector<uint8_t> serialized = MessageBaseToSend::serialize(PreviousSize + thisSerialized.size());
        SerializeDeserializeUtils::addToEnd(serialized, thisSerialized); // Put the base struct serialization in the start of the vector
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
        SerializeDeserializeUtils::addToEnd(serialized, thisSerialized); // Put the base struct serialization in the start of the vector
        return serialized;
    }
};

/// @brief A request to anounce you have a file
struct StoreRequest : MessageBaseToSend
{
    StoreRequest(ID fileId, ID myId) : MessageBaseToSend(ClientRequestCodes::Store), myId(myId), fileId(fileId) {}
    StoreRequest(ID fileId, ID myId, uint8_t code) : MessageBaseToSend(code), myId(myId), fileId(fileId) {}

    /// @brief Your encrypted ID
    ID myId;
    /// @brief The file id
    ID fileId;

    virtual vector<uint8_t> serialize(uint32_t previousSize = 0) const override
    {
        vector<uint8_t> serialized = MessageBaseToSend::serialize(previousSize + sizeof(myId));
        vector<uint8_t> thisSerialized(myId.begin(), myId.end());
        vector<uint8_t> thisSerialized2(fileId.begin(), fileId.end());
        // Concatenate all the data int a vector
        SerializeDeserializeUtils::addToEnd(serialized, thisSerialized);
        SerializeDeserializeUtils::addToEnd(serialized, thisSerialized2);
        return serialized;
    }
};

/// @brief A base struct to store a response Packet. good for status response
struct MessageBaseReceived
{
    vector<uint8_t> data;
    uint8_t code;
    MessageBaseReceived() {}

    MessageBaseReceived(uint8_t code, vector<uint8_t> data)
    {
        this->code = code;
        this->data = data;
    }
};

struct DebuggingStringMessageReceived
{
    std::string data;

    DebuggingStringMessageReceived(MessageBaseReceived messageBaseReceived)
    {
        deserialize(messageBaseReceived.data);
    }

    void deserialize(const std::vector<uint8_t> &buffer)
    {
        data = string(buffer.begin(), buffer.end());
    }

    /// @brief  Heper function to pritn the DATA
    void printDataAsASCII() const
    {
        for (const auto &byte : data)
        {
            if (std::isprint(byte))
            {
                std::cout << static_cast<char>(byte); // Printable characters
            }
            else
            {
                std::cout << '.'; // Replace non-printable characters with '.'
            }
        }
        std::cout << std::endl;
    }
};

/// @brief A list of users that has a file
struct UserListResponse
{
    vector<ID> data;
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
        for (size_t i = 0; i < data.size(); i += sizeof(ID))
        {
            ID currID = *(ID *)(data.data() + i);
            this->data.push_back(currID);
        }
    }
};

/// @brief Your assigned id
struct ServerResponseNewId

{
    ID id;

    ServerResponseNewId(MessageBaseReceived msg)
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
struct ServerResponseUserAuthorizedICEData
{
    vector<uint8_t> iceCandidateInfo;

    ServerResponseUserAuthorizedICEData(const MessageBaseReceived &receivedMessage)
    {
        deserailize(receivedMessage.data);
    }

    void deserailize(const std::vector<uint8_t> &buffer)
    {

        std::cout << "Starting deserialization of ServerResponseUserAuthorizedICEData..." << std::endl;

        iceCandidateInfo = std::move(buffer);
    }
};

/// TODO - Server Request
/// Message data:   lenIceCandidateInfo (2 bytes), iceCandidateInfo (lenStudData btyes), requestId (2 bytes)
struct ServerRequestAuthorizeICEConnection
{
    vector<uint8_t> iceCandidateInfo;
    uint16_t requestId;

    ServerRequestAuthorizeICEConnection(const MessageBaseReceived &receivedMessage)
    {
        deserailize(receivedMessage.data);
    }

    void deserailize(const std::vector<uint8_t> &buffer)
    {

        std::cout << "Starting deserialization of ServerResponseUserAuthorizedICEData..." << std::endl;

        // Extract the length of iceCandidateInfo

        // Extract iceCandidateInfo
        // Validate buffer size for remaining data
        iceCandidateInfo.assign(buffer.begin(), buffer.end() - sizeof(uint16_t));

        std::memcpy(&requestId, buffer.data() + buffer.size() - sizeof(uint16_t), sizeof(uint16_t));
    }
};