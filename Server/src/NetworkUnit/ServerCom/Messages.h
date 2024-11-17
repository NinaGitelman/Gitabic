#pragma once

#include <iostream>
#include <vector>
#include "../../Encryptions/SHA256/sha256.h"

using std::vector;
using ID = HashResult;
using EncryptedID = std::array<uint8_t, 48>;

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

// Codes from protocol
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

struct BaseMessage
{
    uint8_t code;

    BaseMessage(uint8_t code) : code(code) {}
    virtual ~BaseMessage() = default;

    // Serialize the message into a byte vector
    virtual std::vector<uint8_t> serialize() const = 0;

    // Deserialize the message from a byte vector
    virtual void deserialize(const std::vector<uint8_t>& buffer) = 0;

    // Factory method to create a specific message based on code
    static std::unique_ptr<BaseMessage> create(uint8_t code);
};


struct CodeMessage : BaseMessage
{
    CodeMessage(uint8_t code) : BaseMessage(code) {}

    std::vector<uint8_t> serialize() const override 
    {
        return { code }; // Just serialize the code
    }

    void deserialize(const std::vector<uint8_t>& buffer) override 
    {
        
        if (buffer.size() != 1) {
            throw std::runtime_error("Invalid buffer size for CodeMessage");
        }
        code = buffer[0];
    }
};

struct DataMessage : BaseMessage 
{
    std::vector<uint8_t> data;

    DataMessage(uint8_t code, const std::vector<uint8_t>& data = {})
        : BaseMessage(code), data(data) {}

    std::vector<uint8_t> serialize() const override 
    {
        std::vector<uint8_t> result = { code };
        
        uint32_t size = static_cast<uint32_t>(data.size());
       
        result.insert(result.end(), reinterpret_cast<const uint8_t*>(&size),
                      reinterpret_cast<const uint8_t*>(&size) + sizeof(size));
       
        result.insert(result.end(), data.begin(), data.end());
       
        return result;
    }

    void deserialize(const std::vector<uint8_t>& buffer) override 
    {
        if (buffer.size() < 1 + sizeof(uint32_t)) {
            throw std::runtime_error("Buffer too small for DataMessage");
        }

        code = buffer[0];
        uint32_t size = *reinterpret_cast<const uint32_t*>(&buffer[1]);
        if (buffer.size() != 1 + sizeof(uint32_t) + size) {
            throw std::runtime_error("Invalid buffer size for DataMessage");
        }

        data = std::vector<uint8_t>(buffer.begin() + 1 + sizeof(uint32_t), buffer.end());
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

/// @brief Helper Function that receives a code and returns message according to the code 
/// @param code 
/// @return 
inline std::unique_ptr<BaseMessage> create(uint8_t code, const std::vector<uint8_t>& data)
{
   
    switch (code)
    {   
    case 0x01: // Example code for DataMessage
        return std::make_unique<DataMessage>(code, data);
    default:
        throw std::runtime_error("Unknown message code");
    }
}