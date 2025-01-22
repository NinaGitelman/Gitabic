#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include "../../Encryptions/SHA256/sha256.h"
#include <cstring> // for memcpy
#include <iterator>
#include <climits>
#include <array>
#include "../../Utils/SerializeDeserializeUtils.h"
#include <ostream>
#include <arpa/inet.h>

using std::vector;
using ID = HashResult;
using std::shared_ptr;
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

struct Address
{
    static constexpr uint8_t IP_LEN = 4;
    std::array<uint8_t, IP_LEN> ip;
    uint16_t port;

    Address()
    {
        this->port = 0;
        this->ip = ipStringToArray("0.0.0.0");
    }

    Address(string ip, uint16_t port)
    {
        this->port = port;
        this->ip = ipStringToArray(ip);
    }

    Address(std::array<uint8_t, IP_LEN> ip, uint16_t port)
    {
        this->port = port;
        this->ip = ip;
    }

    Address(const sockaddr_in &sockaddr)
    {
        port = ntohs(sockaddr.sin_port);
        uint32_t ipAddress = ntohl(sockaddr.sin_addr.s_addr);

        // Convert IP address to array format
        ip[0] = (ipAddress >> 24) & 0xFF;
        ip[1] = (ipAddress >> 16) & 0xFF;
        ip[2] = (ipAddress >> 8) & 0xFF;
        ip[3] = ipAddress & 0xFF;
    }

    Address(const Address &other)
    {
        this->ip = other.ip;
        this->port = other.port;
    }

    Address &operator=(const Address &other)
    {
        this->ip = other.ip;
        this->port = other.port;
        return *this;
    }

    // Function to return sockaddr_in
    sockaddr_in toSockAddr() const
    {
        sockaddr_in sockaddr;
        std::memset(&sockaddr, 0, sizeof(sockaddr));
        sockaddr.sin_family = AF_INET;
        sockaddr.sin_port = htons(port);

        // Convert IP array to uint32_t and set in sockaddr_in
        uint32_t ipAddress = (ip[0] << 24) | (ip[1] << 16) | (ip[2] << 8) | ip[3];
        sockaddr.sin_addr.s_addr = htonl(ipAddress);

        return sockaddr;
    }

    bool operator==(const Address &other) const
    {
        return other.port == this->port &&
               other.ip == this->ip;
    }

    friend std::ostream &operator<<(std::ostream &os, const Address &address)
    {
        os << "IP: " << address.ipString();
        os << " Port: " << address.port;
        return os;
    }

    std::array<uint8_t, IP_LEN> ipStringToArray(const std::string &ip) const
    {
        std::array<uint8_t, 4> ipArray;
        size_t start = 0;
        size_t end;
        int i = 0;

        while ((end = ip.find('.', start)) != std::string::npos)
        {
            if (i >= 4)
            {
                throw std::invalid_argument("Invalid IP address format");
            }

            int byte = std::stoi(ip.substr(start, end - start));
            if (byte < 0 || byte > 255)
            {
                throw std::out_of_range("IP segment out of range (0-255)");
            }

            ipArray[i++] = static_cast<uint8_t>(byte);
            start = end + 1;
        }

        // Capture the last segment after the last '.'
        if (i != 3)
        {
            throw std::invalid_argument("Invalid IP address format");
        }

        int byte = std::stoi(ip.substr(start));
        if (byte < 0 || byte > 255)
        {
            throw std::out_of_range("IP segment out of range (0-255)");
        }

        ipArray[i] = static_cast<uint8_t>(byte);
        return ipArray;
    }

    string ipString() const
    {
        return std::to_string(ip[0]) + "." +
               std::to_string(ip[1]) + "." +
               std::to_string(ip[2]) + "." +
               std::to_string(ip[3]);
    }

    uint32_t ipUint() const
    {
        return (ip[0] << 24) +
               (ip[1] << 16) +
               (ip[2] << 8) +
               ip[3];
    }
};
namespace std
{
    template <>
    struct hash<Address>
    {
        size_t operator()(const Address &addr) const
        {
            size_t h1 = hash<uint16_t>()(addr.port);
            size_t h2 = hash<uint32_t>()(addr.ipUint());
            return h1 ^ (h2 << 1);
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
    Store = 5,
    UserListReq = 6,

    // debugging
    DebuggingStringMessage = 10
};

enum ClientResponseCodesToServer // 11 - 19
{
    // signaling
    AuthorizedICEConnection = 11,
    AlreadyConnected = 12,
    FullCapacity = 13
};
enum ServerResponseCodes
{
    // signaling
    UserAuthorizedICEData = 11,
    UserAlreadyConnected = 12,
    UserFullCapacity = 13,

    NewID = 55,

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
struct ResultMessage
{
    ID id;
    std::shared_ptr<MessageBaseToSend> msg;
};
struct ServerResponseNewId : MessageBaseToSend

{
    ID id;

    ServerResponseNewId(const ID &id) : id(id), MessageBaseToSend(ServerResponseCodes::NewID) {}

    vector<uint8_t> serialize(uint32_t PreviousSize = 0) const override
    {
        auto serialized = MessageBaseToSend::serialize(id.size());
        SerializeDeserializeUtils::addToEnd(serialized, id);
        return serialized;
    }
};
/// @brief A response to a UserListRequest
struct ServerResponseUserList : MessageBaseToSend
{
    ID _fileId;
    vector<ID> userList;

    ServerResponseUserList(const ID &fileID, const vector<ID> &userList)
        : MessageBaseToSend(ServerResponseCodes::UserListRes), _fileId(fileID), userList(userList) {}

    /// @brief Serializes the data to a vector of bytes
    /// @return A byte vector
    virtual vector<uint8_t> serialize(uint32_t PreviousSize = 0) const override
    {
        vector<uint8_t> serialized = MessageBaseToSend::serialize(userList.size() * SHA256_SIZE + _fileId.size());
        SerializeDeserializeUtils::addToEnd(serialized, vector<uint8_t>(_fileId.begin(), _fileId.end()));
        for (const ID &id : userList)
        {
            SerializeDeserializeUtils::addToEnd(serialized, vector<uint8_t>(id.begin(), id.end()));
        }
        return serialized;
    }
};

//////////////////
/// Signaling ///
//////////////////

/// @brief Response to a user with connect data of another user
/// Message data:  lenIceCandidateInfo (2 bytes), iceCandidateInfo (lenStudData btyes),
struct ServerResponseUserAuthorizedICEData : MessageBaseToSend
{

    vector<uint8_t> iceCandidateInfo;

    ServerResponseUserAuthorizedICEData(vector<uint8_t> iceCandidateInfo)
        : MessageBaseToSend(ServerResponseCodes::UserAuthorizedICEData), iceCandidateInfo(move(iceCandidateInfo)) {}

    vector<uint8_t> serialize(uint32_t PreviousSize = 0) const override
    {
        // if (iceCandidateInfo.size() > USHRT_MAX)
        // {
        //     throw std::runtime_error("Ice candidate is too long (max size - 2 bytes)");
        // }

        // Serialize base struct
        vector<uint8_t> serialized = MessageBaseToSend::serialize(PreviousSize + iceCandidateInfo.size());

        // Append `iceCandidateInfo`
        SerializeDeserializeUtils::addToEnd(serialized, iceCandidateInfo);

        return serialized;
    }
};



/// @brief Response to a user with connect data of another user
/// Message data:   lenIceCandidateInfo (2 bytes), iceCandidateInfo (lenStudData btyes), requestId (2 bytes)
struct ServerRequestAuthorizeICEConnection : MessageBaseToSend
{
    static constexpr uint8_t CONST_SIZE = 2;
    uint16_t requestId;
    vector<uint8_t> iceCandidateInfo;
    ID from;

    ServerRequestAuthorizeICEConnection(vector<uint8_t> iceCandidateInfo, uint16_t requestId, const ID& from)
        : MessageBaseToSend(ServerRequestCodes::AuthorizeICEConnection), requestId(requestId), iceCandidateInfo(move(iceCandidateInfo)), from(from) {}

    vector<uint8_t> serialize(uint32_t PreviousSize = 0) const override
    {
        uint16_t len = iceCandidateInfo.size();

        // serialize requestId
        vector<uint8_t> requestIdSerialized(2);
        SerializeDeserializeUtils::serializeUint16IntoVector(requestIdSerialized, requestId);

        // Serialize base struct
        vector<uint8_t> serialized = MessageBaseToSend::serialize(PreviousSize + CONST_SIZE + len + sizeof(from));

        // Append to serialized ken, ice candidaate info and request id
        SerializeDeserializeUtils::addToEnd(serialized, from);
        SerializeDeserializeUtils::addToEnd(serialized, iceCandidateInfo);
        SerializeDeserializeUtils::addToEnd(serialized, requestIdSerialized);
        return serialized;
    }
};

// just for start debugging, pretify later
struct DebuggingStringMessageToSend : MessageBaseToSend
{

    std::string message;

    DebuggingStringMessageToSend(string message) : message(message), MessageBaseToSend(ClientRequestCodes::DebuggingStringMessage) {}

    virtual vector<uint8_t> serialize(uint32_t PreviousSize = 0) const override
    {
        vector<uint8_t> thisSerialized(this->message.begin(), this->message.end());
        vector<uint8_t> serialized = MessageBaseToSend::serialize(PreviousSize + thisSerialized.size());
        SerializeDeserializeUtils::addToEnd(serialized, thisSerialized); // Add this serailaized to the end of the vector
        return serialized;
    }
};

/// @brief A base struct to store a response Packet. good for status response
struct MessageBaseReceived
{
    uint8_t code;
    vector<uint8_t> data;
    ID from;
    MessageBaseReceived() {}

    MessageBaseReceived(uint8_t code, vector<uint8_t> data)
    {
        this->code = code;
        this->data = data;
    }
};

struct GeneralRecieve
{
    ID from;
    GeneralRecieve(const ID &from)
    {
        this->from = from;
    }
};

struct ClientRequestStore : GeneralRecieve
{
    /// @brief Your encrypted ID
    ID myId;
    /// @brief The file id
    ID fileId;
    ClientRequestStore(const MessageBaseReceived &receivedMessage) : GeneralRecieve(receivedMessage.from)
    {
        deserialize(receivedMessage.data);
    }

    void deserialize(const std::vector<uint8_t> &buffer)
    {
        myId = *(ID *)(buffer.data());
        fileId = *(ID *)(buffer.data() + SHA256_SIZE);
    }
};
struct ClientRequestUserList : GeneralRecieve
{
    /// @brief The file id
    ID fileId;
    ClientRequestUserList(const MessageBaseReceived &receivedMessage) : GeneralRecieve(receivedMessage.from)
    {
        deserialize(receivedMessage.data);
    }

    void deserialize(const std::vector<uint8_t> &buffer)
    {
        fileId = *(ID *)(buffer.data());
    }
};

//////////////////
/// Signaling ///
//////////////////

/// @brief struct from client to request for another user's ice info (user is defiend by the id) and receive the client's ice info
/// Message: 32 bytes id | 2 bytes iceCandLen | iceCandLen btyes iceCandInfo

struct ClientRequestGetUserICEInfo : GeneralRecieve
{
    ID RequestedUserId;
    std::vector<uint8_t> iceCandidateInfo;

    // Constructor to deserialize from received message
    ClientRequestGetUserICEInfo(const MessageBaseReceived &receivedMessage) : GeneralRecieve(receivedMessage.from)
    {
        deserialize(receivedMessage.data);
    }

    void deserialize(const std::vector<uint8_t> &buffer)
    {

        // Validate minimum buffer size
        if (buffer.size() < SHA256_SIZE)
        {
            throw std::runtime_error("Buffer too small");
        }

        // First byte is message code (you're checking this in your message handling already)
        size_t offset = 0;

        // Extract user ID
        std::memcpy(RequestedUserId.data(), buffer.data() + offset, SHA256_SIZE);
        offset += SHA256_SIZE;

        // Extract candidate info
        iceCandidateInfo.assign(
            buffer.begin() + offset,
            buffer.end());
    }
};

/// @brief client response to request of another user to authorize ice connections
/// lenIceCandidateInfo (2 bytes), iceCandidateInfo (lenStudData btyes), requestId (2 bytes)
struct ClientResponseAuthorizedICEConnection : GeneralRecieve
{
    std::vector<uint8_t> iceCandidateInfo;
    uint16_t requestId;

    // Constructor: Takes a received message and deserializes it
    ClientResponseAuthorizedICEConnection(const MessageBaseReceived &receivedMessage) : GeneralRecieve(receivedMessage.from)
    {
        deserialize(receivedMessage.data);
    }

    // Deserialize the buffer into member variables
    void deserialize(const std::vector<uint8_t> &buffer)
    {
        if (buffer.size() < 2)
        { // Minimum size: 2 bytes for length + 2 bytes for requestId
            throw std::invalid_argument("Buffer too small to deserialize");
        }

        // Extract iceCandidateInfo
        iceCandidateInfo.assign(buffer.begin(),
                                buffer.end() - sizeof(uint16_t));

        // Extract requestId
        std::memcpy(&requestId, buffer.data() + buffer.size() - sizeof(uint16_t), sizeof(uint16_t));

        std::cout << "Deserializing Client Response Authorized ICE connection";
        std::cout << "Extracted iceCandidateInfo: ";
        for (uint8_t byte : iceCandidateInfo)
        {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << +byte << " ";
        }
        std::cout << "   Extracted requestId: " << requestId << std::endl;

        std::cout << std::endl;
    }
};

struct ClientResponseAlreadyConnected : GeneralRecieve {
    uint16_t requestID;

    explicit ClientResponseAlreadyConnected(const MessageBaseReceived &receivedMessage) : GeneralRecieve(receivedMessage.from)
    {
        deserialize(receivedMessage.data);
    }

    void deserialize(const std::vector<uint8_t> &buffer) {
        memcpy(&requestID, buffer.data(), sizeof(requestID));
    }

};
struct ClientResponseFullCapacity : GeneralRecieve {
    uint16_t requestID;

    explicit ClientResponseFullCapacity(const MessageBaseReceived &receivedMessage) : GeneralRecieve(receivedMessage.from)
    {
        deserialize(receivedMessage.data);
    }

    void deserialize(const std::vector<uint8_t> &buffer) {
        memcpy(&requestID, buffer.data(), sizeof(requestID));
    }

};

struct DebuggingStringMessageReceived : GeneralRecieve
{
    std::string data;
    DebuggingStringMessageReceived(MessageBaseReceived messageBaseReceived) : GeneralRecieve(messageBaseReceived.from)
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
