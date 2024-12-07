#include <iostream>
#include "Communication/MultiThreadedServer/MultiThreadedServer.h"
#include "Encryptions/SHA256/sha256.h"

int main() {
    MultiThreadedServer server{};

    server.startHandleRequests();

    std::cout << "Hello world" << std::endl;
}