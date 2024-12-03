#include <iostream>
#include "Communication/MultiThreadedServer.h"
#include "Encryptions/SHA256/sha256.h"
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// Signal handler function
void signalHandler(int signum)
{
    if (signum == SIGINT)
    {
        std::cout << "\nCtrl+C received. Shutting down server..." << std::endl;

        // Call destructor if server instance exists
        if (MultiThreadedServer::g_serverInstance)
        {
            delete MultiThreadedServer::g_serverInstance;
            MultiThreadedServer::g_serverInstance = nullptr;
        }

        exit(signum);
    }
}

int main()
{
    // Register signal handler for Ctrl+C
    signal(SIGINT, signalHandler);

    try
    {
        MultiThreadedServer server;
        MultiThreadedServer::g_serverInstance = &server;
        server.startHandleRequests();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
    }

    return 0;
}