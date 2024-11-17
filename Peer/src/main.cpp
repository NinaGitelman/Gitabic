#include <iostream>
#include <string>
#include <unistd.h>
#include "NetworkUnit/ServerComm/DataRepublish/DataRepublish.h"
#include "NetworkUnit/ServerComm/TCPSocket/TCPSocket.h"
#include "NetworkUnit/SocketHandler/SocketHandler.h"
#include "NetworkUnit/ServerComm/Messages.h"
#include "Encryptions/SHA256/sha256.h"


int main()
{
    Address serverAdd = Address("0.0.0.0", 4789);
    TCPSocket socket = TCPSocket(serverAdd);

    DataMessage msg = DataMessage("Hello world!");
    
    socket.sendRequest(msg);
    socket.sendRequest(msg);
    socket.sendRequest(msg);
    sleep(10);
    socket.sendRequest(msg);
    socket.sendRequest(msg);





    // DataRepublish dr(nullptr);
    // HashResult res = SHA256::toHashSha256(vector<uint8_t>{1});
    // HashResult res2 = SHA256::toHashSha256(vector<uint8_t>{2});

    // dr.saveData(res, EncryptedID());
    // std::this_thread::sleep_for(std::chrono::seconds(2));
    // dr.saveData(res2, EncryptedID());
    // std::string input;

    // while (true)
    // {
    //     std::getline(std::cin, input);
    //     if (input == "exit")
    //     {
    //         break;
    //     }
    //     if (input == "stop")
    //     {
    //         dr.stopRepublish(res);
    //     }
    // }
}