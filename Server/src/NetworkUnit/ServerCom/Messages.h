#pragma once

#include <iostream>
#include <vector>
#include <string>
#include "../../Encryptions/SHA256/sha256.h"
#include <cstring> // for memcpy
#include <iterator>
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
    // no message received
    NoMessageReceived = 0,

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

enum ServerResponseCodes
{
    // signaling
    UserAuthorizedICEData = 11,

    // bit torrent 
    StoreSuccess = 221,
    StoreFailure,
    UserListRes
};

enum ServerRequestCodes
{
    // signaling
    AuthorizeICEConnection = 20

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



//////////////////
/// Signaling ///
//////////////////

/// @brief Response to a user with connect data of another user
/// Message data:  lenIceCandidateInfo (2 bytes), iceCandidateInfo (lenStudData btyes), 
struct ServerResponseUserAuthorizedICEData : MessageBaseToSend 
{
    const int CONST_SIZE = 2;

    vector<uint8_t> iceCandidateInfo;

    ServerResponseUserAuthorizedICEData(vector<uint8_t> iceCandidateInfo)
        : MessageBaseToSend(ServerResponseCodes::UserAuthorizedICEData), iceCandidateInfo(move(iceCandidateInfo)) {}

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
        
        // Serialize base struct
        vector<uint8_t> serialized = MessageBaseToSend::serialize(PreviousSize+CONST_SIZE+len);

        // Append `lenSerialized` and `iceCandidateInfo`
        serialized.insert(serialized.end(), lenSerialized.begin(), lenSerialized.end());
        serialized.insert(serialized.end(), iceCandidateInfo.begin(), iceCandidateInfo.end());

        return serialized;
    }
};


/// @brief Response to a user with connect data of another user
/// Message data:   lenIceCandidateInfo (2 bytes), iceCandidateInfo (lenStudData btyes), requestId (2 bytes) 
struct ServerRequestAuthorizeICEConnection : MessageBaseToSend 
{
    const int CONST_SIZE = 4;
    uint16_t requestId;
    vector<uint8_t> iceCandidateInfo;

    ServerRequestAuthorizeICEConnection(vector<uint8_t> iceCandidateInfo)
        : MessageBaseToSend(ServerRequestCodes::AuthorizeICEConnection), iceCandidateInfo(move(iceCandidateInfo)) {}

    vector<uint8_t> serialize(uint32_t PreviousSize = 0) const override 
    {
        uint16_t len = iceCandidateInfo.size();

        // Serialize `len` into two bytes
        vector<uint8_t> lenSerialized(2);
        SerializeDeserializeUtils::serializeUint16IntoVector(lenSerialized, len);

        // serialize requestId
        vector<uint8_t> requestIdSerialized(2);
        SerializeDeserializeUtils::serializeUint16IntoVector(requestIdSerialized, requestId);

        // Serialize base struct
        vector<uint8_t> serialized = MessageBaseToSend::serialize(PreviousSize + CONST_SIZE + len);

        // Append to serialized ken, ice candidaate info and request id
        serialized.insert(serialized.end(), lenSerialized.begin(), lenSerialized.end());
        serialized.insert(serialized.end(), iceCandidateInfo.begin(), iceCandidateInfo.end());
        serialized.insert(serialized.end(), requestIdSerialized.begin(), requestIdSerialized.end());

        return serialized;
    }
};


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

//////////////////
/// Signaling ///
//////////////////

/// @brief struct from client to request for another user's ice info (user is defiend by the id) and receive the client's ice info
/// Message: 32 bytes id | 2 bytes iceCandLen | iceCandLen btyes iceCandInfo 

struct ClientRequestGetUserICEInfo
{
    ID userId;
    std::vector<uint8_t> iceCandidateInfo;

    // Constructor to deserialize from received message
    ClientRequestGetUserICEInfo(const MessageBaseReceived& receivedMessage) {
        deserialize(receivedMessage.data);
    }

 void deserialize(const std::vector<uint8_t>& buffer) 
 {
   
    // Validate minimum buffer size
    if (buffer.size() < 1 + 4 + SHA256_SIZE + 2) {
        throw std::runtime_error("Buffer too small");
    }

    // First byte is message code (you're checking this in your message handling already)
    size_t offset = 0;

    // Skip message code
    offset++;

    // Extract previous size (4 bytes)
    uint32_t previousSize;
    std::memcpy(&previousSize, buffer.data() + offset, sizeof(previousSize));
    offset += 4;

    // Extract user ID
    std::memcpy(userId.data(), buffer.data() + offset, SHA256_SIZE);
    offset += SHA256_SIZE;

    for (auto byte : userId) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << +byte << " ";  
    }   
    std::cout << std::endl;

    // Extract length (2 bytes, little endian)
    uint16_t length = buffer[offset] | (buffer[offset + 1] << 8);
    offset += 2;


    // Extract candidate info
    iceCandidateInfo.assign(
        buffer.begin() + offset, 
        buffer.begin() + offset + length
    );

    for (auto byte : iceCandidateInfo) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << +byte << " ";  
    }   
    std::cout << std::endl;
}
};

/// @brief client response to request of another user to authorize ice connections
/// lenIceCandidateInfo (2 bytes), iceCandidateInfo (lenStudData btyes), requestId (2 bytes)
struct ClientResponseAuthorizedICEConnection 
{
    std::vector<uint8_t> iceCandidateInfo;
    uint16_t requestId;

    // Constructor: Takes a received message and deserializes it
    ClientResponseAuthorizedICEConnection(const MessageBaseReceived& receivedMessage) 
    {
        deserialize(receivedMessage.data);

    }

    // Deserialize the buffer into member variables
    void deserialize(const std::vector<uint8_t>& buffer) 
    {
        if (buffer.size() < 4) 
        { // Minimum size: 2 bytes for length + 2 bytes for requestId
            throw std::invalid_argument("Buffer too small to deserialize");
        }

        // Extract length of iceCandidateInfo
        uint16_t length = 0;
        std::memcpy(&length, buffer.data(), sizeof(uint16_t));

        // Ensure buffer has enough data for iceCandidateInfo
        if (buffer.size() < sizeof(uint16_t) + length + sizeof(uint16_t)) 
        {
            throw std::invalid_argument("Buffer does not contain enough data for iceCandidateInfo and requestId");
        }

        // Extract iceCandidateInfo
        iceCandidateInfo.assign(buffer.begin() + sizeof(uint16_t),
                                buffer.begin() + sizeof(uint16_t) + length);

        // Extract requestId
        std::memcpy(&requestId, buffer.data() + sizeof(uint16_t) + length, sizeof(uint16_t));


        std::cout << "Deserializing Client Response Authorized ICE connection";
        std::cout << "Extracted iceCandidateInfo: ";
        for (uint8_t byte : iceCandidateInfo) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << +byte << " ";
        }
        std::cout << "   Extracted requestId: " << requestId << std::endl;

        std::cout << std::endl;

    }
};






struct DebuggingStringMessageReceived 
{
    std::vector<uint8_t> data;
    DebuggingStringMessageReceived(MessageBaseReceived messageBaseReceived)
    {

        deserialize(messageBaseReceived.data);

    }

    void deserialize(const std::vector<uint8_t>& buffer) 
    {

        data = std::vector<uint8_t>(buffer.begin() + sizeof(uint32_t), buffer.end());
    }

    /// @brief  Heper function to pritn the DATA
    void printDataAsASCII() const 
    {
        for (const auto &byte : data) 
        {
            if (std::isprint(byte)) 
            {
                std::cout << static_cast<char>(byte); // Printable characters
            } else {
                std::cout << '.'; // Replace non-printable characters with '.'
            }
        }
        std::cout << std::endl;
    }
};
