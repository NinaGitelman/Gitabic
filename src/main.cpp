#include <iostream>
#include "Encryptions/SHA256/sha256.h"
int main()
{
    std::cout << "hello\n";
    std::vector<uint8_t> data = {
        '1',
        '2',
        '3',
        '4',
        '5',
        '6',
        '7',
        '1'};
    HashResult res = SHA256::toHashSha256(data);
    SHA256::printHashAsString(res);
}