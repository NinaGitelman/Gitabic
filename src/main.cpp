#include <iostream>
#include "NetworkUnit/ServerComm/DataRepublish/DataRepublish.h"
#include "Encryptions/SHA256/sha256.h"

int main()
{
    DataRepublish dr(nullptr);
    HashResult res = SHA256::toHashSha256(vector<uint8_t>{1});
    HashResult res2 = SHA256::toHashSha256(vector<uint8_t>{2});

    dr.saveData(res, EncryptedID());
    std::this_thread::sleep_for(std::chrono::seconds(2));
    dr.saveData(res2, EncryptedID());
    std::string input;

    while (true)
    {
        std::getline(std::cin, input);
        if (input == "exit")
        {
            break;
        }
        if (input == "stop")
        {
            dr.stopRepublish(res);
        }
    }
}