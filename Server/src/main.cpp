#include <iostream>
#include "Communication/MultiThreadedServer.h"
#include "Encryptions/SHA256/sha256.h"

int main()
{
    MultiThreadedServer server = MultiThreadedServer();
    
    server.startHandleRequests();

    std::cout << "Hello world" << std::endl;
}