#include "TCPSocket.h"

TCPSocket::TCPSocket(const Address& serverAddress) : sockfd(-1)
{
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        throw std::runtime_error("Failed to create socket");
    }

    sockaddr_in addrIn = serverAddress.toSockAddr();

    try
    {
        connectToServer(addrIn);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    std::cout << "end of constr";
}

TCPSocket::~TCPSocket()
{
    if (sockfd != -1)
    {
        close(sockfd);
        sockfd = -1;
    }
}

void TCPSocket::sendRequest(const MessageBaseToSend& msg)
{
    std::lock_guard<mutex> guard(socketMut); // Lock the resource

    vector<uint8_t> serialized = msg.serialize();
    send(sockfd, serialized.data(), serialized.size(), 0); // Send the serialized data
}

MessageBaseReceived TCPSocket::receive(std::function<bool(uint8_t)> isRelevant)
{
    std::cout << "in receive function";
    auto start_time = std::chrono::high_resolution_clock::now();
    while (true)
    {
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = now - start_time;
        {
            // If there is a relevant message already recieved - return it.
            std::lock_guard<mutex> guard(socketMut);
            auto msg = std::find_if(messages.begin(), messages.end(), [&isRelevant](const MessageBaseReceived& msg)
            {
                return isRelevant(msg.code);
            });
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
        std::cout << "Received " << (int)code << std::endl;

        if (recv(sockfd, &size, sizeof(size), 0) != sizeof(size)) // Read the size (4 bytes)
        {
            throw std::runtime_error("Failed to read response size");
        }

        vector<uint8_t> data(size);
        if (size)
        {
            if (recv(sockfd, data.data(), size, 0) != size)
            {
                throw std::runtime_error("Failed to read response data");
            }
        }
        if (isRelevant(code))
        {
            return MessageBaseReceived(code, data);
        }
        else
        {
            messages.push_back(MessageBaseReceived(code, data));
        }
    }
    throw std::runtime_error("No relevant packets");
}

void TCPSocket::connectToServer(sockaddr_in& serverAddress)
{
    // this doesnt even print even if debugger tells me it gets here
    std::cout << "In connect to server...";
    std::cout << inet_ntoa(serverAddress.sin_addr);
    std::lock_guard<mutex> guard(socketMut);
    if (connect(sockfd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1)
    {
        std::cerr << "Connection failed. Error: " << strerror(errno) << std::endl;
        throw std::runtime_error("Failed to connect to server");
    }
}
