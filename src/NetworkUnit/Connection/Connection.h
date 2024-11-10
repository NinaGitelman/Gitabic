#pragma once

#include <iostream>
#include <array>
#include <queue>
#include <vector>
#include <mutex>
#include <ctime>
#include <string>
#include <stdexcept>
#include <cstring>

#include "../SocketHandler/SocketHandler.h"
class SocketHandler;

using std::array;
using std::mutex;
using std::ostream;
using std::queue;
using std::string;
using std::time_t;
using std::vector;

#define IP_LEN 4

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

    bool operator==(Address &other) const
    {
        return other.port == this->port &&
               other.ip == this->ip;
    }
    friend ostream &operator<<(ostream &os, const Address &address)
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

struct Message
{
    uint8_t code;
    uint32_t size;
    vector<uint8_t> data;
    Address other;
};

class Connection
{
protected:
    Address peerAdress;
    queue<Message> messafeRecvQueue;
    vector<uint8_t> receivedData;
    mutex recvMutex;

    queue<vector<uint8_t>> PacketsToSend;
    mutex sendMutex;

    bool isAlive;
    friend class SocketHandler;

    Connection(Address peer);
    virtual void onReceive(vector<uint8_t> data) = 0;
    virtual vector<uint8_t> getPacketToSend();

public:
    bool getIsAlive() const;
    virtual bool close() = 0;
    virtual void send(Message msg) = 0;
};