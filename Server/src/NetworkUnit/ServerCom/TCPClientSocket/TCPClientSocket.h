#ifndef TCPSOCKET_H
#define TCPSOCKET_H

#pragma once

#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <mutex>
#include <memory>
#include <chrono>
#include "../Messages.h"

using std::mutex;

class TCPClientSocket
{
public:
    
   /// @brief Constructor an alread exisitng socket that sent message to server
    TCPClientSocket(int existingSocket, const sockaddr_in &address);
    int getSocketFd() const;

    /// @brief closes the socket
    ~TCPClientSocket();

    /// @brief Sends a request
    /// @param msg the request
    void send(const BaseMessage &msg);
   
    /// @brief Recieves a message
    /// @param isRelevant a callback function to verify the packet is relevant
    /// @return The message
     std::shared_ptr<BaseMessage> receive();


private:
 
    sockaddr_in clientAddress;
    mutex socketMut;
    int sockfd;
};


#endif