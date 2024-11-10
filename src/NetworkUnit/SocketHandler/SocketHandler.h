#pragma once
#include <thread>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include "../Connection/Connection.h"

using std::thread;

class SocketHandler
{
public:
    SocketHandler(bool isTcp);
    uint16_t getPort() const;
    Connection &registerConnection(const Address &address);
    void unregisterConnection(const Address &address);
    Connection &getConnection(const Address &address);
    Connection &getNewConnection(const Address &address);

private:
    Address myAddress;
    bool isTcp;
    bool isActive;
    int socketId;
    mutex sockMutex;
    vector<Connection &> connections;
    queue<Connection &> newConnections;
    thread recvAndSendThread;

    void handle();
    void sendTo(const Address &to, vector<uint8_t> data);
    uint16_t calculateUDPChecksum(vector<uint8_t> buffer,
                                  uint32_t src_addr, uint32_t dest_addr);
    void setupSocket();
    Address getSocketAddress() const;
};