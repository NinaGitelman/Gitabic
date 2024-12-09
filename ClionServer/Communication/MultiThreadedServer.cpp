#include "MultiThreadedServer.h"
int MultiThreadedServer::id = 0;

MultiThreadedServer::~MultiThreadedServer()
{
    try
    {
        cleanupSocket();

        _clients.clear();
    }
    catch (...)
    {
    }
}

void MultiThreadedServer::cleanupSocket()
{

    if (_serverSocket != -1)
    {
        // Unbind the port
        struct sockaddr_in sa = {};
        sa.sin_family = AF_INET;
        sa.sin_port = htons(SERVER_PORT);
        sa.sin_addr.s_addr = INADDR_ANY;

        // Attempt to release the socket address
        int optval = 1;
        setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

        // Close the socket
        close(_serverSocket);
        _serverSocket = -1;
    }
}

void MultiThreadedServer::startHandleRequests()
{
    _serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (_serverSocket == -1)
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
    sa.sin_addr.s_addr = INADDR_ANY;

    if (bind(_serverSocket, (struct sockaddr *)&sa, sizeof(sa)) == -1)
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
        catch (const std::exception &e)
        {
            std::cerr << "Exception in acceptClient(): " << e.what() << std::endl;
        }
    }
}

void MultiThreadedServer::acceptClient()
{

    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);

    int clientSocket = accept(_serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);

    if (clientSocket == -1)
    {

        throw std::runtime_error("Failed to accept client connection");
    }

    auto tcpClientSocket = std::make_shared<TCPClientSocket>(clientSocket, clientAddr);

    std::cout << "Client accepted. Server and client can speak" << std::endl;

    std::thread([this, tcpClientSocket]()
                { handleClient(tcpClientSocket); })
        .detach();
}

void MultiThreadedServer::handleClient(std::shared_ptr<TCPClientSocket> clientSocket)
{
    int clientId = ++MultiThreadedServer::id;
    ID id;
    {
        std::lock_guard<std::mutex> lock(_clientsMutex);
        _clients[clientId] = clientSocket;
    }

    try
    {
        id = generateRandomId();
        ids[id] = clientSocket;
        clientSocket->send(ServerResponseNewId(id));
        if (ids.size() == 2)
        {
            ID i123 = ids.begin()->first == id ? (++ids.begin())->first : ids.begin()->first;
            clientSocket->send(ServerResponseNewId(i123));
        }
        while (_running)
        {
            if (!messagesToSend[id].empty())
            {
                auto msg = messagesToSend[id].front();
                messagesToSend[id].pop();
                clientSocket->send(*msg);
            }

            // create id for client and add it to the map...
            auto msg = clientSocket->receive();
            if (msg.code == 0)
            {
                continue;
            }
            msg.from = id;

            ResultMessage response = messageHandler->handle(msg);
            if (response.id == ID())
            {
                continue;
            }
            messagesToSend[response.id].push(response.msg);
            std::cout << "";
        }
    }
    catch (const std::exception &e)
    {
        std::cout << "handleClient exception: " << e.what() << std::endl;
    }

    // Clean up client
    {
        std::lock_guard<std::mutex> lock(_clientsMutex);
        _clients.erase(clientId);
        ids.erase(id);
    }
}

/// @brief  Heper function to pritn the DATA
void MultiThreadedServer::printDataAsASCII(vector<uint8_t> data)
{
    for (const auto &byte : data)
    {
        if (std::isprint(byte))
        {
            std::cout << static_cast<char>(byte); // Printable characters
        }
        else
        {
            std::cout << '.'; // Replace non-printable characters with '.'
        }
    }
    std::cout << std::endl;
}

ID MultiThreadedServer::generateRandomId()
{
    ID id;
    srand(time(0));
    do
    {
        for (auto &it : id)
        {
            it = rand() % 256;
        }
    } while (std::find_if(ids.begin(), ids.end(),
                          [&id](const auto &pair)
                          {
                              return pair.first == id;
                          }) != ids.end() ||
             id == ID());
    return id;
}
