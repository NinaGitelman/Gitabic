#ifndef TCPSOCKET_H
#define TCPSOCKET_H

#pragma once

#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include "../Messages.h"
#include "../../SocketHandler/SocketHandler.h"
class TCPSocket
{
public:
    TCPSocket(const Address &serverAddress);
    ~TCPSocket();

    void sendRequest(const RequestMessageBase &msg);
    ResponseMessageBase recieve();

private:
    void connectToServer(const Address &serverAddress);
    int sockfd;
};

#endif