#ifndef TCPSOCKET_H
#define TCPSOCKET_H

#pragma once

#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <functional>
#include "../Messages.h"
#include "../../SocketHandler/SocketHandler.h"
class TCPSocket
{
public:
    TCPSocket(const Address &serverAddress);
    ~TCPSocket();

    void sendRequest(const RequestMessageBase &msg);
    ResponseMessageBase recieve(std::function<bool(uint8_t)> isRelevant);

private:
    void connectToServer(const Address &serverAddress);
    vector<ResponseMessageBase> messages;
    mutex socketMut;
    int sockfd;
};

#endif