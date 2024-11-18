#include "MultiThreadedServer.h"
int MultiThreadedServer::id = 0;

MultiThreadedServer::~MultiThreadedServer()
{
    try
    {
        stop();
        close(_serverSocket);

        _clients.clear();
        
    
    }
    catch(...)
    {   }
}


void MultiThreadedServer::startHandleRequests()
{
        _serverSocket = socket(AF_INET, SOCK_STREAM, 0);

        if(_serverSocket == -1)
        {
            // TODO - may change this to our exception and have better error handling
            throw std::runtime_error("Failed to create server socket");
        }
        else
        {
            bindAndListen();
        }
    

}

void MultiThreadedServer::bindAndListen()
{

    struct sockaddr_in sa = {};
    sa.sin_family = AF_INET;
    sa.sin_port = htons(SERVER_PORT);
    sa.sin_addr.s_addr = INADDR_ANY; // change to the public address 

    if (bind(_serverSocket, (struct sockaddr*)&sa, sizeof(sa)) == -1)
    {
        throw std::runtime_error("Failed to bind socket");
    }    

    if (listen(_serverSocket, SOMAXCONN) == -1)
    {
        throw std::runtime_error("Failed to listen on socket");
    } 

    std::cout << "Listening on port " << SERVER_PORT << std::endl;

    while (_running) 
    {
        try 
        {
            acceptClient();
        } 
        catch (const std::exception& e)
        {
            std::cerr << "Exception in acceptClient(): " << e.what() << std::endl;
        }
    }
    
}

void MultiThreadedServer::acceptClient() 
{
    
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);

    int clientSocket = accept(_serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
       
    if (clientSocket == -1)
    {

        throw std::runtime_error("Failed to accept client connection");
    }

    auto tcpClientSocket = std::make_shared<TCPClientSocket>(clientSocket, clientAddr);
     
    std::cout << "Client accepted. Server and client can speak" << std::endl;
   
    std::thread([this, tcpClientSocket]() {
            handleClient(tcpClientSocket);
        }).detach();
    
}

void MultiThreadedServer::handleClient(std::shared_ptr<TCPClientSocket> clientSocket)
{
    int clientId = ++MultiThreadedServer::id;
    
    {
        std::lock_guard<std::mutex> lock(_clientsMutex);
        _clients[clientId] = clientSocket;
    }

    try 
    {
        while(_running)
        {

            // create id for client and add it to the map...
            auto msg = clientSocket->receive();
            // TODO check this tommorw - not compiling rn
            // if(msg != nullptr)
            // {
            //     break; // client disconnected
            // }
           if(msg.code == (uint8_t)ClientRequestCodes::DebuggingStringMessageToSend)
            {
                DebuggingStringMessageReceived dataMsg = DebuggingStringMessageReceived(msg);
                
                std::cout << "id: " << clientId << " - ";
                dataMsg.printDataAsASCII();
                
              }
          
            
        }
    }
    catch (const std::exception& e) 
    {
        std::cout << "handleClient exception: " << e.what() << std::endl;

    }

   // Clean up client
    {
        std::lock_guard<std::mutex> lock(_clientsMutex);
        _clients.erase(clientId);
    }

}

