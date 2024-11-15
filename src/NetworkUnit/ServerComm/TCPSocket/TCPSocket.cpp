#include "TCPSocket.h"

TCPSocket::TCPSocket(const Address &serverAddress) : sockfd(-1)
{
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        throw std::runtime_error("Failed to create socket");
    }
}

TCPSocket::~TCPSocket()
{
    if (sockfd != -1)
    {
        close(sockfd);
    }
}

void TCPSocket::sendRequest(const RequestMessageBase &msg)
{
    vector<uint8_t> serialized = msg.serialize(0); // Previous size = 0 for now
    uint32_t size = serialized.size();

    uint8_t code = msg.code;

    send(sockfd, &code, 1, 0);                // Send the code (1 byte)
    send(sockfd, &size, sizeof(size), 0);     // Send the size (4 bytes)
    send(sockfd, serialized.data(), size, 0); // Send the serialized data
}

ResponseMessageBase TCPSocket::recieve()
{
    uint8_t code;
    uint32_t size;
    if (recv(sockfd, &code, sizeof(code), 0) != sizeof(code)) // Read the code (1 byte)
    {
        throw std::runtime_error("Failed to read response code");
    }

    if (recv(sockfd, &size, sizeof(size), 0) != sizeof(size)) // Read the size (4 bytes)
    {
        throw std::runtime_error("Failed to read response size");
    }

    vector<uint8_t> data(size);
    if (recv(sockfd, data.data(), size, 0) != size)
    {
        throw std::runtime_error("Failed to read response data");
    }

    return ResponseMessageBase(code, data);
}

void TCPSocket::connectToServer(const Address &serverAddress)
{
    if (connect(sockfd, (struct sockaddr *)&serverAddress.toSockAddr(), sizeof(serverAddress.toSockAddr())) == -1)
    {
        throw std::runtime_error("Failed to connect to server");
    }
}
