#pragma once

#include <iostream>
#include <vector>
#include <string>
#include "../../Encryptions/SHA256/sha256.h"
#include <climits>
#include "../../Utils/SerializeDeserializeUtils.h"

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
    AuthorizeICEConnection = 20
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





////////////////////
//// Signaling  ////
///////////////////

/// Message to send
/// TODO - Client request: get user ICE info
/// Message: 32 bytes user id | 2 bytes iceCandLen | iceCandLen btyes iceCandInfo 
// works!!!!
struct ClientRequestGetUserICEInfo : MessageBaseToSend
{
    const int CONST_SIZE = 6; 
    ID userId;
    vector<uint8_t> iceCandidateInfo;

    ClientRequestGetUserICEInfo(ID userId, vector<uint8_t> iceCandidateInfo) : MessageBaseToSend(ClientRequestCodes::GetUserICEInfo), userId(userId), iceCandidateInfo(move(iceCandidateInfo)){}

    vector<uint8_t> serialize(uint32_t PreviousSize = 0) const override
    {

        if(iceCandidateInfo.size() > USHRT_MAX)
        {
            throw std::runtime_error("Ice candidate is too long (max size - 2 bytes)");
        }
        uint16_t len = iceCandidateInfo.size();

        // Serialize `len` into two bytes
        vector<uint8_t> lenSerialized(2);
        SerializeDeserializeUtils::serializeUint16IntoVector(lenSerialized, len);
        


        vector<uint8_t> serialized = MessageBaseToSend::serialize(PreviousSize + CONST_SIZE + len);

        serialized.insert(serialized.end(), userId.begin(), userId.end());
        serialized.insert(serialized.end(), lenSerialized.begin(), lenSerialized.end());
        serialized.insert(serialized.end(), iceCandidateInfo.begin(), iceCandidateInfo.end());
        
        return serialized;
    
    }
}; 


/// Client Response - ClientResponseAuthorizedICEConnection
/// lenIceCandidateInfo (2 bytes), iceCandidateInfo (lenStudData btyes), requestId (2 bytes)
struct ClientResponseAuthorizedICEConnection : MessageBaseToSend
{
    const int CONST_SIZE = 4;
    vector<uint8_t> iceCandidateInfo;
    uint16_t requestId;

    ClientResponseAuthorizedICEConnection(vector<uint8_t> iceCandidateInfo, uint16_t requestId) : MessageBaseToSend(ClientResponseCodes::ClientResponseAuthorizedICEConnection), iceCandidateInfo(move(iceCandidateInfo)), requestId(requestId){}
    
    vector<uint8_t> serialize(uint32_t PreviousSize = 0) const override
    {

        if(iceCandidateInfo.size() > USHRT_MAX)
        {
            throw std::runtime_error("Ice candidate is too long (max size - 2 bytes)");
        }
        uint16_t len = iceCandidateInfo.size();

        // Serialize `len` into two bytes vector
        vector<uint8_t> lenSerialized(2);
        SerializeDeserializeUtils::serializeUint16IntoVector(lenSerialized, len);

        // Serialize `requestId` into two bytes vector
        vector<uint8_t> requestIdSerialized;
        SerializeDeserializeUtils::serializeUint16IntoVector(requestIdSerialized, requestId);

        // serialize base struct
        vector<uint8_t> serialized = MessageBaseToSend::serialize(PreviousSize+CONST_SIZE+len);

        serialized.insert(serialized.end(), lenSerialized.begin(), lenSerialized.end());
        serialized.insert(serialized.end(), iceCandidateInfo.begin(), iceCandidateInfo.end());
        serialized.insert(serialized.end(), requestIdSerialized.begin(), requestIdSerialized.end());

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
struct ServerResponseUserAuthorizedICEData
{
    vector<uint8_t> iceCandidateInfo;

    ServerResponseUserAuthorizedICEData(const MessageBaseReceived& receivedMessage)
    {
        deserailize(receivedMessage.data);
    }

    void deserailize(const std::vector<uint8_t>& buffer)
    {

        std::cout << "Starting deserialization of ServerResponseUserAuthorizedICEData..." << std::endl;

         // Extract the length of iceCandidateInfo
        uint16_t length = 0;
        std::memcpy(&length, buffer.data() + SHA256_SIZE, sizeof(uint16_t));
        std::cout << "Extracted iceCandidateInfo length: " << length << std::endl;
        
        // Extract iceCandidateInfo
          // Validate buffer size for remaining data
        size_t valueStart = sizeof(uint16_t);
        iceCandidateInfo.assign(buffer.begin() + valueStart, buffer.begin() + valueStart + length);
        std::cout << "Extracted iceCandidateInfo: ";
        for (auto byte : iceCandidateInfo) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << +byte << " ";
        }
        std::cout << std::endl;

    }


};

/// TODO - Server Request
/// Message data:   lenIceCandidateInfo (2 bytes), iceCandidateInfo (lenStudData btyes), requestId (2 bytes) 
struct ServerRequestAuthorizeICEConnection
{
    vector<uint8_t> iceCandidateInfo;
    uint16_t requestId;

    ServerRequestAuthorizeICEConnection(const MessageBaseReceived& receivedMessage)
    {
        deserailize(receivedMessage.data);
    }

    void deserailize(const std::vector<uint8_t>& buffer)
    {

        std::cout << "Starting deserialization of ServerResponseUserAuthorizedICEData..." << std::endl;

         // Extract the length of iceCandidateInfo
        uint16_t length = 0;
        std::memcpy(&length, buffer.data(), sizeof(uint16_t));
        std::cout << "Extracted iceCandidateInfo length: " << length << std::endl;
        
        // Extract iceCandidateInfo
          // Validate buffer size for remaining data
        size_t valueStart = sizeof(uint16_t);
        iceCandidateInfo.assign(buffer.begin() + valueStart, buffer.begin() + valueStart + length);
       
        std::cout << "Extracted iceCandidateInfo: ";
        for (auto byte : iceCandidateInfo) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << +byte << " ";
        }

            // Extract the requestId (2 bytes), after iceCandidateInfo
        valueStart+= length;
        std::memcpy(&requestId, buffer.data() + valueStart, sizeof(uint16_t));
        std::cout << "\nExtracted requestId: " << requestId << std::endl;

    
        std::cout << std::endl;

    }


};