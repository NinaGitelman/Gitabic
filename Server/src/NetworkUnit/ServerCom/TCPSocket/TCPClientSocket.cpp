#include "TCPSocket.h"

TCPClientSocket::TCPClientSocket(int existingSocket, const sockaddr_in &address)
: sockfd(existingSocket), clientAddress(address) {}

TCPClientSocket::~TCPClientSocket()
{
    if (sockfd != -1)
    {
        close(sockfd);
        sockfd = -1;
    }
}

void TCPClientSocket::sendRequest(const RequestMessageBase &msg)
{
    std::lock_guard<mutex> guard(socketMut); // Lock the resource

    vector<uint8_t> serialized = msg.serialize();
    uint32_t size = serialized.size();

    uint8_t code = msg.code;

    send(sockfd, &code, 1, 0);                // Send the code (1 byte)
    send(sockfd, &size, sizeof(size), 0);     // Send the size (4 bytes)
    send(sockfd, serialized.data(), size, 0); // Send the serialized data
}

ResponseMessageBase TCPClientSocket::receive(std::function<bool(uint8_t)> isRelevant)
{
    auto start_time = std::chrono::high_resolution_clock::now();
    while (true)
    {
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = now - start_time;
        if (elapsed >= std::chrono::seconds(5)) // If no relevant packet for 5 seconds - stop trying
        {
            break;
        }
        { // If there is a relevant message already recieved - return it.
            std::lock_guard<mutex> guard(socketMut);
            auto msg = std::find_if(messages.begin(), messages.end(), [&isRelevant](const ResponseMessageBase &msg)
                                    { return isRelevant(msg.code); });
            if (msg != messages.end())
            {
                auto res = *msg;
                messages.erase(msg);
                return res;
            }
        }
        // Recieve a message
        uint8_t code;
        uint32_t size;
        std::lock_guard<mutex> guard(socketMut);
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
        if (isRelevant(code))
        {
            return ResponseMessageBase(code, data);
        }
        else
        {
            messages.push_back(ResponseMessageBase(code, data));
        }
    }
    throw std::runtime_error("No relevant packets");
}

int TCPClientSocket::getSocketFd() const 
{
    return sockfd;
     
}
