#include "MultiThreadedServer.h"

MultiThreadedServer::~MultiThreadedServer()
{
    try
    {
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

    while (true) 
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
            handleNewClient(tcpClientSocket);
        }).detach();
    
}

void MultiThreadedServer::handleNewClient(std::shared_ptr<TCPClientSocket> clientSocket)
{

    try 
    {
        while(true)
        {

            // create id for client and add it to the map...
        
            auto isRelevant = [](uint8_t code)
            {
                return true;
                
            };
            auto msg = clientSocket->receive(isRelevant);
            
            try
            {   
                if(msg && msg->code == 0x01)
                {

                    shared_ptr<DataMessage> dataMsg = std::dynamic_pointer_cast<DataMessage> (msg);
                    dataMsg->printDataAsASCII();
                }
            }
            catch(const std::exception& e)
            {
                std::cerr  << e.what() << '\n';
            }
            
        }
    }
    catch (const std::exception& e) 
    {
        std::cerr << "handleNewClient exception: " << e.what() << std::endl;

    }
    // later also take out from map....
    // {
    //     std::lock_guard<std::mutex> l(_clients);
    //     _clients.erase(clientSocket); // this already closes the socket (through the TCPsocket)
    // }
  //  delete clientSocket;

}

