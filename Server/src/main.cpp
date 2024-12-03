#include <iostream>
#include "Communication/MultiThreadedServer.h"
#include "Encryptions/SHA256/sha256.h"
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// Optional signal handler for clean exit
void signalHandler(int signum)
{
    if (signum == SIGINT)
    {
        std::cout << "\nCtrl+C received. Shutting down..." << std::endl;
        exit(signum);
    }
}

int main()
{
    // Register signal handler for Ctrl+C (optional)
    signal(SIGINT, signalHandler);

    try
    {
        MultiThreadedServer server;
        server.startHandleRequests();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
    }

    return 0;
}