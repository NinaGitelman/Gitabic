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
    MultiThreadedServer(): _serverSocket(-1), _running(true){};

    ~MultiThreadedServer();

    void startHandleRequests();

    void stop() {_running = false;}

private:

    static int id;

    volatile bool _running;

    int _serverSocket;
   // std::map<ID, std::shared_ptr<TCPClientSocket>> _clients;
    std::map<int, std::shared_ptr<TCPClientSocket>> _clients;
    mutex _clientsMutex;
    
    void bindAndListen();
    
    void acceptClient();

    void handleNewClient(std::shared_ptr<TCPClientSocket> clientSocket);
   


};