#include <iostream>
#include "NetworkUnit/Connection/Connection.h"
int main()
{

    // Create an Address object from IP string and port
    Address address("192.168.1.1", 8080);

    // Convert Address to sockaddr_in
    sockaddr_in sockaddr = address.toSockAddr();

    // Output the Address using the overloaded << operator
    std::cout << address << std::endl;

    // Create an Address object from sockaddr_in
    Address addressFromSockaddr(sockaddr);

    // Output the Address created from sockaddr_in
    std::cout << addressFromSockaddr << std::endl;

    return 0;
}
