#include "TCPClientSocket.h"

TCPClientSocket::TCPClientSocket(int existingSocket, const sockaddr_in &address) : clientAddress(address), sockfd(existingSocket) {}

TCPClientSocket::~TCPClientSocket()
{
    if (sockfd != -1)
    {
        close(sockfd);
        sockfd = -1;
    }
}

void TCPClientSocket::send(const MessageBaseToSend& msg)
{

    std::vector<uint8_t> serialized = msg.serialize();
    uint32_t size = static_cast<uint32_t>(serialized.size());

    std::lock_guard<mutex> guard(socketMut); // Lock the resource
    // Send the message size and serialized data
    if (::send(sockfd, &size, sizeof(size), 0) != sizeof(size)) 
    {
        throw std::runtime_error("Failed to send message size");
    }

    if (::send(sockfd, serialized.data(), size, 0) != size) 
    {
        throw std::runtime_error("Failed to send message data");
    }
}

MessageBaseReceived TCPClientSocket::receive() 
{
   std::lock_guard<mutex> guard(socketMut);

    uint8_t code;
    uint32_t size;

    // Use MSG_PEEK to check if there's data available
    ssize_t peekResult = recv(sockfd, &code, sizeof(code), MSG_PEEK);
    
    if (peekResult <= 0)
    {
        if (peekResult == 0) 
        {
           
        throw std::runtime_error("Client disconnected");

        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) 
        {
            // No data available
            vector<uint8_t> emptyVector;
            return MessageBaseReceived(0, emptyVector);
        }
        throw std::runtime_error("Failed to peek message");
    }

    // Actually read the code
    if (recv(sockfd, &code, sizeof(code), 0) != sizeof(code)) 
    {
        throw std::runtime_error("Failed to read message code");
    }

    if (recv(sockfd, &size, sizeof(size), 0) != sizeof(size)) {
        throw std::runtime_error("Failed to read response size");
    }

    vector<uint8_t> data(size);
    size_t total_received = 0;
    while (total_received < size) 
    {
        ssize_t received = recv(sockfd, data.data() + total_received,size - total_received,0);
        if (received <= 0) 
        {
            throw std::runtime_error("Failed to read response data");
        }
        total_received += received;
    }

    
    return MessageBaseReceived(code, data);
   
}

int TCPClientSocket::getSocketFd() const
{
    return sockfd;
     
}

