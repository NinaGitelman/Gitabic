#pragma once
#include <thread>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

struct Address
{
    std::array<uint8_t, 4> ip;
    uint16_t port;

    bool operator==(Address &other)
    {
        return other.port == this->port &&
               other.ip == this->ip;
    }
};

class SocketHandler
{
public:
    SocketHandler();
};