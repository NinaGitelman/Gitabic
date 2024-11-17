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
class TCPClientSocket
{
public:
    /// @brief Construct and connect to the given address
    /// @param serverAddress The address to connect
    TCPClientSocket(const Address &serverAddress);
    /// @brief closes the socket
    ~TCPClientSocket();

    /// @brief Sends a request
    /// @param msg the request
    void sendRequest(const RequestMessageBase &msg);
    /// @brief Recieves a message
    /// @param isRelevant a callback function to verify the packet is relevant
    /// @return The message
    ResponseMessageBase receive(std::function<bool(uint8_t)> isRelevant);

private:
    /// @brief Connects to the server
    /// @param serverAddress Its address
    void connectToServer(const sockaddr_in &serverAddress);
    /// @brief Messages recieved but irelevant for requester
    vector<ResponseMessageBase> messages;
    mutex socketMut;
    int sockfd;
};

#endif