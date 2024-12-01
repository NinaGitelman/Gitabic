#include <iostream>
#include <string>
#include <unistd.h>
#include "NetworkUnit/ServerComm/DataRepublish/DataRepublish.h"
#include "NetworkUnit/ServerComm/TCPSocket/TCPSocket.h"
#include "NetworkUnit/SocketHandler/SocketHandler.h"
#include "NetworkUnit/ServerComm/Messages.h"
#include "Encryptions/SHA256/sha256.h"

/// @brief  Heper function to pritn the DATA
void printDataAsASCII(vector<uint8_t> data)
{
    for (const auto &byte : data)
    {
        if (std::isprint(byte))
        {
            std::cout << static_cast<char>(byte); // Printable characters
        }
        else
        {
            std::cout << '.'; // Replace non-printable characters with '.'
        }
    }
    std::cout << std::endl;
}

int main()
{
    Address serverAdd = Address("0.0.0.0", 4789);
    TCPSocket socket = TCPSocket(serverAdd);

    ServerResponseNewId newId(socket.receive([](uint8_t code)
                                             { return code == ServerResponseCodes::NewID; }));

    ID id = newId.id;

    ServerRequestAuthorizeICEConnection authIceReq(socket.receive([](uint8_t code)
                                                                  { return code == ServerRequestCodes::AuthorizeICEConnection; }));

    std::cout << "Received!!\n";
    printDataAsASCII(authIceReq.iceCandidateInfo);

    std::string message = "123456 654321!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
    vector<uint8_t> iceTestData(message.begin(), message.end());

    ClientResponseAuthorizedICEConnection requestIce = ClientResponseAuthorizedICEConnection(iceTestData, authIceReq.requestId);
    socket.sendRequest(requestIce);
    std::cout << "Done!!\n";

    // ServerResponseUserAuthorizedICEData response = socket.receive(isRelevant);
    // printDataAsASCII(response.iceCandidateInfo);

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
