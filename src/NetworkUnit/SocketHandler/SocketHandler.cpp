#include "SocketHandler.h"

SocketHandler::SocketHandler(bool isTcp)
{
    this->isTcp = isTcp;
    this->isActive = true;
    this->setupSocket();
    this->recievedData = std::unordered_map<Address, std::queue<vector<uint8_t>>>();

    this->recvAndSendThread = thread(&SocketHandler::handle, this);
    this->recvAndSendThread.detach();
}

uint16_t SocketHandler::getPort() const
{
    return this->myAddress.port;
}

vector<uint8_t> SocketHandler::getPacket(const Address &address)
{
    std::lock_guard<mutex> lock(mut);
    try
    {
        auto packet = recievedData[address].front();
        recievedData[address].pop();
        if (recievedData[address].empty())
        {
            recievedData.erase(address);
        }
        return packet;
    }
    catch (const std::exception &ex)
    {
        return vector<uint8_t>();
    }
}

vector<PacketData> SocketHandler::getAllPackets()
{
    vector<PacketData> result;
    for (auto &it : recievedData)
    {
        while (!it.second.empty())
        {
            result.push_back(PacketData(it.second.front(), it.first));
            it.second.pop();
        }
    }
    recievedData.erase(recievedData.begin(), recievedData.end());
    return result;
}

PacketData SocketHandler::getSynned()
{
    if (synPackets.empty())
    {
        return PacketData();
    }
    auto packet = synPackets.front();
    synPackets.pop();
    return packet;
}

void SocketHandler::sendPacket(const PacketData &packet)
{
    std::lock_guard<mutex> lock(mut);
    this->toSend.push(packet);
}

void SocketHandler::handle()
{
    while (isActive)
    {
        if (!toSend.empty())
        {
            PacketData packet;
            {
                std::lock_guard<mutex> lock(mut);
                packet = toSend.front();
                toSend.pop();
            }
            sendTo(packet.other, packet.data);
        }
        recv();
    }
}

void SocketHandler::recv()
{
    vector<uint8_t> buffer(16 * 1024);
    struct sockaddr_in clientaddr;
    socklen_t addrLen = sizeof(clientaddr);
    std::lock_guard<mutex> lock(mut);
    int n = recvfrom(this->socketId, buffer.data(), MAX_BUFFER_SIZE, MSG_DONTWAIT,
                     (struct sockaddr *)&clientaddr, &addrLen);
    if (n > 0)
    {
        buffer.erase(buffer.begin(), buffer.begin() + sizeof(iphdr) + sizeof(udphdr));
        buffer.shrink_to_fit(); // TODO - verify it known it got written.

        Address from = Address(clientaddr);
        if (recievedData.find(from) == recievedData.end())
        {
            recievedData[from] = queue<vector<uint8_t>>();
        }
        recievedData[from].push(buffer);
        if (isTcp)
        {
            // TODO add to syn queue if syn after implementung tcp
        }
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

    this->myAddress = Address(getNonLoopbackIP(), getSocketPort());
}

uint16_t SocketHandler::getSocketPort() const
{
    sockaddr_in addr;
    socklen_t addrLen = sizeof(addr);

    // Retrieve the socket address using getsockname
    if (getsockname(this->socketId, reinterpret_cast<sockaddr *>(&addr), &addrLen) == -1)
    {
        return 0;
    }

    // Use the Address constructor to initialize the Address object with sockaddr_in data
    return ntohs(addr.sin_port);
}

string SocketHandler::getNonLoopbackIP() const
{
    struct ifaddrs *interfaces, *ifa;
    char ip[INET_ADDRSTRLEN];

    // Get list of network interfaces
    if (getifaddrs(&interfaces) == -1)
    {
        perror("getifaddrs");
        return "";
    }

    // Loop through each network interface
    for (ifa = interfaces; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == nullptr)
            continue;

        // Check if the interface is using IPv4 and is not a loopback address
        if (ifa->ifa_addr->sa_family == AF_INET && !(ifa->ifa_flags & IFF_LOOPBACK))
        {
            struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
            inet_ntop(AF_INET, &addr->sin_addr, ip, INET_ADDRSTRLEN);

            // Free the interface list
            freeifaddrs(interfaces);

            // Return the first non-loopback IP found
            return std::string(ip);
        }
    }

    // Free the interface list
    freeifaddrs(interfaces);

    // Return an empty string if no non-loopback address is found
    return "";
}