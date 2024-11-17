#ifndef AESHANDLER_H
#define AESHANDLER_H
#pragma once

#include <array>
#include <iostream>
#include <cstdint>
#include <vector>

#define BLOCK 16
#define N 4
#define R 10

using std::array;
using Word = array<uint8_t, 4>;
using Matrix4x4 = array<Word, 4>;

class AESHandler
{
public:
    AESHandler();
    ~AESHandler();

private:
    uint8_t rcon(uint8_t round);
    uint8_t sBox(uint8_t input);
    Word rotate(Word word, uint8_t by);
    Word subWord(Word word);
    void keyExpansion(array<uint8_t, BLOCK> key);

    std::array<std::array<uint8_t, BLOCK>, R + 1> keys;
};

#endif