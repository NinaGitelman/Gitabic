//
// Created by user on 12/9/24.
//

#ifndef ADDRESS_H
#define ADDRESS_H
#define IP_LEN 4

#include <iostream>
#include <array>
#include <string>
#include <sstream>
#include <memory>
#include <netinet/in.h>
#include <string.h>


using std::array;
using std::string;
struct Address
{
    array<uint8_t, IP_LEN> ip;
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

    Address(array<uint8_t, IP_LEN> ip, uint16_t port)
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

sockaddr_in toSockAddr() const
{
    sockaddr_in sockaddr;
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;

    // Debug prints
    std::cout << "Port: " << port << std::endl;
    std::cout << "IP: "
              << static_cast<int>(ip[0]) << "."
              << static_cast<int>(ip[1]) << "."
              << static_cast<int>(ip[2]) << "."
              << static_cast<int>(ip[3]) << std::endl;

    sockaddr.sin_port = htons(port);

    uint32_t ipAddress = (ip[0] << 24) | (ip[1] << 16) | (ip[2] << 8) | ip[3];

    // Additional debug
    std::cout << "Converted IP Address: " << std::hex << ipAddress << std::dec << std::endl;

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

    std::array<uint8_t, IP_LEN> ipStringToArray(const std::string &strIp) const
    {
        std::array<uint8_t, 4> strIpArray;
        size_t start = 0;
        size_t end;
        int i = 0;

        while ((end = strIp.find('.', start)) != std::string::npos)
        {
            if (i >= 4)
            {
                throw std::invalid_argument("Invalid strIp address format");
            }

            int byte = std::stoi(strIp.substr(start, end - start));
            if (byte < 0 || byte > 255)
            {
                throw std::out_of_range("strIp segment out of range (0-255)");
            }

            strIpArray[i++] = static_cast<uint8_t>(byte);
            start = end + 1;
        }

        // Capture the last segment after the last '.'
        if (i != 3)
        {
            throw std::invalid_argument("Invalid strIp address format");
        }

        int byte = std::stoi(strIp.substr(start));
        if (byte < 0 || byte > 255)
        {
            throw std::out_of_range("strIp segment out of range (0-255)");
        }

        strIpArray[i] = static_cast<uint8_t>(byte);
        return strIpArray;
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
#endif //ADDRESS_H
