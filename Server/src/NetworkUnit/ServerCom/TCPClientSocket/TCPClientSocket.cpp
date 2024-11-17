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

void TCPClientSocket::send(const BaseMessage& msg)
{
    std::lock_guard<mutex> guard(socketMut); // Lock the resource

    std::vector<uint8_t> serialized = msg.serialize();
    uint32_t size = static_cast<uint32_t>(serialized.size());

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


std::shared_ptr<BaseMessage> TCPClientSocket::receive(std::function<bool(uint8_t)> isRelevant) 
{
    auto start_time = std::chrono::high_resolution_clock::now();

    while (true)
    {
        // what is the point from here
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = now - start_time;
        
        if (elapsed >= std::chrono::seconds(5)) // If no relevant packet for 5 seconds - stop trying
        {
            break;
        }
        { // If there is a relevant message already received - return it.
            std::lock_guard<mutex> guard(socketMut);
           
           
            auto msg = std::find_if(messages.begin(), messages.end(), [&isRelevant](std::unique_ptr<BaseMessage> &msg)
                                    { return isRelevant(msg->code); });
           
            if (msg != messages.end())
            {
                auto res = std::move(*msg); // Take the message
                messages.erase(msg);
                return res;
            }
        }


        // Recieve a message
        uint8_t code;
        uint32_t size;

        std::lock_guard<mutex> guard(socketMut);
        if (recv(sockfd, &code, sizeof(code), 0) == sizeof(code)) // Read the code (1 byte)
        {
           
            if (recv(sockfd, &size, sizeof(size), 0) != sizeof(size)) // Read the size (4 bytes) // if cant throw error
            {
                 throw std::runtime_error("Failed to read response size");
            
            }

            vector<uint8_t> data(size);
            if (recv(sockfd, data.data(), size, 0) != size)
            {
                    throw std::runtime_error("Failed to read response data");
            }

            // Create and deserialize the message based on the received code
            auto message = create(code, data);
            message->deserialize(data);

            if (isRelevant(message->code))
            {
                return message;
            }
            else
            {
                messages.push_back(std::move(message));
            }

        }
        else
        {
            continue; // no message received - no code
        }

        
    }

    return NULL;
   
}

int TCPClientSocket::getSocketFd() const
{
    return sockfd;
     
}

