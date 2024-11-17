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
    
   /// @brief Constructor an alread exisitng socket that sent message to server
    TCPClientSocket(int existingSocket, const sockaddr_in &address) 
    : sockfd(existingSocket), clientAddress(address) {}

    int getSocketFd() const;

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
 
    vector<ResponseMessageBase> messages;
    mutex socketMut;
    sockaddr_in clientAddress;
    int sockfd;
};

#endif