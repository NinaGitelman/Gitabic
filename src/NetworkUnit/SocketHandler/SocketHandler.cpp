#include "SocketHandler.h"

SocketHandler::SocketHandler(bool isTcp)
{
    this->isTcp = isTcp;
    this->isActive = true;
    this->setupSocket();

    this->recvAndSendThread = thread(SocketHandler::handle, this);
    this->recvAndSendThread.detach();
}

uint16_t SocketHandler::getPort() const
{
    return this->myAddress.port;
}

void SocketHandler::handle()
{
    while (isActive)
    {
    }
}

void SocketHandler::sendTo(const Address &to, vector<uint8_t> data)
{
    udphdr udpHeader;
    udpHeader.source = htons(myAddress.port);            // Source port
    udpHeader.dest = htons(to.port);                     // Destination port
    udpHeader.len = htons(sizeof(udphdr) + data.size()); // Length of the UDP header + data
    udpHeader.check = 0;

    // Create a vector for the UDP header
    std::vector<uint8_t> udpHeaderVec(sizeof(udphdr));
    std::memcpy(udpHeaderVec.data(), &udpHeader, sizeof(udphdr));

    // Prepend the UDP header to the data vector
    data.insert(data.begin(), udpHeaderVec.begin(), udpHeaderVec.end());

    ((udphdr *)data.data())->check = calculateUDPChecksum(data, this->myAddress.ipUint(), to.ipUint());

    auto clientaddr = to.toSockAddr();

    if (sendto(this->socketId, data.data(), data.size(), 0, (struct sockaddr *)&clientaddr, sizeof(clientaddr)) < 0)
    {
        perror("Send failed");
        return;
    }
}

uint16_t SocketHandler::calculateUDPChecksum(vector<uint8_t> buffer, uint32_t src_addr, uint32_t dest_addr)
{
    uint32_t sum = 0;
    size_t length = buffer.size();
    const uint16_t *buf = reinterpret_cast<const uint16_t *>(buffer.data());

    // Pseudo-header
    sum += (src_addr >> 16) & 0xFFFF;
    sum += src_addr & 0xFFFF;
    sum += (dest_addr >> 16) & 0xFFFF;
    sum += dest_addr & 0xFFFF;
    sum += htons(IPPROTO_UDP);
    sum += htons(length);

    // UDP header and data
    while (length > 1)
    {
        sum += *buf++;
        length -= 2;
    }

    // Add left-over byte, if any
    if (length > 0)
    {
        sum += *reinterpret_cast<const uint8_t *>(buf);
    }

    // Fold 32-bit sum to 16 bits
    while (sum >> 16)
    {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return static_cast<uint16_t>(~sum);
}

void SocketHandler::setupSocket()
{
    this->socketId = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);

    int optval = 0;
    if (setsockopt(this->socketId, IPPROTO_IP, IP_HDRINCL, &optval, sizeof(optval)) < 0)
    {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    Address address;
    auto sockAddress = address.toSockAddr();
    if (bind(this->socketId, (const struct sockaddr *)&sockAddress, sizeof(sockAddress)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    this->myAddress = getSocketAddress();
}

Address SocketHandler::getSocketAddress() const
{
    sockaddr_in addr;
    socklen_t addrLen = sizeof(addr);

    // Retrieve the socket address using getsockname
    if (getsockname(this->socketId, reinterpret_cast<sockaddr *>(&addr), &addrLen) == -1)
    {
        return Address();
    }

    // Use the Address constructor to initialize the Address object with sockaddr_in data
    return Address(addr);
}
