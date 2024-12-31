#include "TCPClientSocket.h"

TCPClientSocket::TCPClientSocket(int existingSocket, const Address &address) : clientAddress(address), sockfd(existingSocket) {}

TCPClientSocket::~TCPClientSocket()
{
    if (sockfd != -1)
    {
        close(sockfd);
        sockfd = -1;
    }
}

void TCPClientSocket::send(const MessageBaseToSend &msg)
{

    std::vector<uint8_t> serialized = msg.serialize();

    std::lock_guard<mutex> guard(socketMut); // Lock the resource
    // Send the message size and serialized data
    if (::send(sockfd, serialized.data(), serialized.size(), 0) != serialized.size())
    {
        throw std::runtime_error("Failed to send message data");
    }
}
MessageBaseReceived TCPClientSocket::receive()
{
    uint8_t code = 0;  // Default code value
    uint32_t size = 0; // Default size value
    std::lock_guard<mutex> guard(socketMut);

    // Use MSG_PEEK with a timeout mechanism to check for available data
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(sockfd, &read_fds);

    struct timeval timeout;
    timeout.tv_sec = 0;  // No wait time
    timeout.tv_usec = 0; // Immediate return

    int select_result = select(sockfd + 1, &read_fds, nullptr, nullptr, &timeout);

    if (select_result == 0)
    {
        // No data available immediately
        return MessageBaseReceived(0, vector<uint8_t>());
    }

    if (select_result < 0)
    {
        // Select error
        throw std::runtime_error("Select error while checking socket");
    }

    // Attempt to peek the code
    ssize_t peekResult = recv(sockfd, &code, sizeof(code), MSG_PEEK);
    if (peekResult == 0)
    {
        throw std::runtime_error("Client disconnected");
    }
    if (peekResult < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // No data available
            return MessageBaseReceived(0, vector<uint8_t>());
        }
        throw std::runtime_error("Failed to peek message");
    }

    // Actually read the code
    if (recv(sockfd, &code, sizeof(code), 0) != sizeof(code))
    {
        // If we can't read the code, return default values
        return MessageBaseReceived(0, vector<uint8_t>());
    }

    // Try to read size, return defaults if fails
    if (recv(sockfd, &size, sizeof(size), 0) != sizeof(size))
    {
        return MessageBaseReceived(0, vector<uint8_t>());
    }

    // If size is zero, return early
    if (size == 0)
    {
        return MessageBaseReceived(code, vector<uint8_t>());
    }

    vector<uint8_t> data(size);
    size_t total_received = 0;
    while (total_received < size)
    {
        ssize_t received = recv(sockfd, data.data() + total_received, size - total_received, 0);
        if (received <= 0)
        {
            // If data reception fails, return what we have or default
            return MessageBaseReceived(code, vector<uint8_t>());
        }
        total_received += received;
    }

    return MessageBaseReceived(code, data);
}

int TCPClientSocket::getSocketFd() const
{
    return sockfd;
}
