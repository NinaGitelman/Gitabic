#pragma once

#include <map>
#include <mutex>
#include <memory>
#include <thread>
#include <iostream>
#include "../NetworkUnit/ServerCom/TCPClientSocket/TCPClientSocket.h"


using std::map;
using std::mutex;
using std::shared_ptr;

#define SERVER_PORT 4789

class MultiThreadedServer 
{

public:

    //friend class PacketHandler;
    
    MultiThreadedServer(): _running(true){};

    ~MultiThreadedServer();

    /// @brief Function starts the server 
    /// Throws exception if there is an error with ti
    void startHandleRequests();

    void stop() {_running = false;}

private:

    static int id;

    volatile bool _running;

    int _serverSocket;
   // std::map<ID, std::shared_ptr<TCPClientSocket>> _clients;
    std::map<int, std::shared_ptr<TCPClientSocket>> _clients;
    mutex _clientsMutex;
    
    /// @brief Function binds the server socket, listens for new clients and handles if there is one
    void bindAndListen();
    
    /// @brief Function accepts/connects a new client and creates a thread for it
    void acceptClient();

    /// @brief Function handles clients sockets - listens for messages and takes care of them
    /// @param clientSocket socket of the client to handle
    void handleClient(std::shared_ptr<TCPClientSocket> clientSocket);
   


};