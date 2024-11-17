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
    Store = 21,
    UserListReq = 23,

    DebuggingStringMessageToSend = 255 
};
enum ServerResponseCodes
{
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

// just for start debugging, pretify later
struct DebuggingStringMessageToSend : MessageBaseToSend
{


    std::string message;

    DebuggingStringMessageToSend(string message) : message(message), MessageBaseToSend(RequestCodes::DebuggingStringMessageToSend) {}
    
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



struct DebuggingStringMessageReceived 
{
    std::vector<uint8_t> data;
    DebuggingStringMessageReceived(MessageBaseReceived messageBaseReceived)
    {

        deserialize(messageBaseReceived.data);

    }

    void deserialize(const std::vector<uint8_t>& buffer) override 
    {

        data = std::vector<uint8_t>(buffer.begin() + sizeof(uint32_t), buffer.end());
    }

    /// @brief  Helper function to pritn the DATA
    void printDataAsASCII() const 
    {
        std::cout << "Code: " << static_cast<int>(code) << "\nData (ASCII): ";
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
