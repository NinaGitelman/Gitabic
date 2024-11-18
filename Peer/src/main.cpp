#include <iostream>
#include "Encryptions/AES/AESHandler.h"

int main()
{
    AESHandler aes(array<uint8_t, BLOCK>{0xb2, 0x93, 0x76, 0x3a, 0xd7, 0x27, 0x76, 0xa8, 0x68, 0x70, 0xf9, 0xa1, 0x19, 0xa0, 0x61, 0x99});
    vector<uint8_t> d{'1'};
    auto iv = aes.encrypt(d, true, false);
    aes.printHexDump(aes.getKey());

    aes.decrypt(d, true, nullptr);

    aes.printHexDump(*reinterpret_cast<array<uint8_t, BLOCK> *>(d.data()));
}