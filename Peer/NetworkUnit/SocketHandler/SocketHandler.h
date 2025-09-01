#pragma once
#include <thread>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <ifaddrs.h>
#include <cstring>
#include <net/if.h>
#include <memory>
#include <iostream>
#include <array>
#include <queue>
#include <vector>
#include <mutex>
#include <ctime>
#include <string>
#include <stdexcept>
#include <cstring>
#include <unordered_map>

using std::array;
using std::mutex;
using std::ostream;
using std::queue;
using std::string;
using std::thread;
using std::time_t;
using std::unordered_map;
using std::vector;

#define IP_LEN 4
#define MAX_BUFFER_SIZE 16 * 1024

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
    std::memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    
    // // Debug prints
    // std::cout << "Port: " << port << std::endl;
    // std::cout << "IP: "
    //           << static_cast<int>(ip[0]) << "."
    //           << static_cast<int>(ip[1]) << "."
    //           << static_cast<int>(ip[2]) << "."
    //           << static_cast<int>(ip[3]) << std::endl;

    sockaddr.sin_port = htons(port);

    uint32_t ipAddress = (ip[0] << 24) | (ip[1] << 16) | (ip[2] << 8) | ip[3];
    
    // Additional debug
    // std::cout << "Converted IP Address: " << std::hex << ipAddress << std::dec << std::endl;

    sockaddr.sin_addr.s_addr = htonl(ipAddress);

    return sockaddr;
}
    bool operator==(const Address &other) const
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

struct PacketData
{
    vector<uint8_t> data;
    Address other;

    PacketData &operator=(const PacketData &other)
    {
        this->data = other.data;
        this->other = other.other;
        return *this;
    }

    PacketData()
    {
        data = vector<uint8_t>();
        other = Address();
    }
    PacketData(const PacketData &other)
    {
        this->data = other.data;
        this->other = other.other;
    }
    PacketData(vector<uint8_t> data, Address other)
    {
        this->data = data;
        this->other = other;
    }
};

class SocketHandler
{
public:
    SocketHandler(bool isTcp);
    uint16_t getPort() const;
    vector<uint8_t> getPacket(const Address &address);
    vector<PacketData> getAllPackets();
    PacketData getSynned();
    void sendPacket(const PacketData &packet);

private:
    Address myAddress;
    bool isTcp;
    bool isActive;
    int socketId;
    mutex mut;
    thread recvAndSendThread;
    queue<PacketData> toSend;
    unordered_map<Address, queue<vector<uint8_t>>> recievedData;
    queue<PacketData> synPackets;

    void handle();
    void recv();
    void sendTo(const Address &to, vector<uint8_t> data);
    uint16_t calculateUDPChecksum(vector<uint8_t> buffer,
                                  uint32_t src_addr, uint32_t dest_addr);
    void setupSocket();
    uint16_t getSocketPort() const;
    std::string getNonLoopbackIP() const;
};