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
#include "/media/user/OS/LinuxExtraSpace/gitabic/emek-yizrael-1701-gitabic/Server/src/NetworkUnit/ServerCom/Messages.h"

using std::mutex;

class TCPClientSocket
{
public:
    
   /// @brief Constructor that accepts an existing socket and uses it as the tcp socket - also its address
    TCPClientSocket(int existingSocket, const sockaddr_in& address);
    int getSocketFd() const;

    /// @brief closes the socket
    ~TCPClientSocket();

    /// @brief Sends a request
    /// @param msg the request
    void send(const MessageBaseToSend &msg);
   
    /// @brief Recieves a message
    /// @return The message
    MessageBaseReceived receive();


private:
    
    sockaddr_in clientAddress;
    mutex socketMut;
    int sockfd;
};


#endif