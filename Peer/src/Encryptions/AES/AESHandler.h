#ifndef AESHANDLER_H
#define AESHANDLER_H
#pragma once

#include <array>
#include <iostream>
#include <cstdint>
#include <vector>
#include <span>

#define BLOCK 16
#define N 4
#define WORD 4
#define R 10

using std::array;
using std::vector;
using Word = array<uint8_t, WORD>;
using Matrix4x4 = array<Word, 4>;

class AESHandler
{
public:
    AESHandler();
    ~AESHandler();

    Matrix4x4 encrypt(vector<uint8_t> &data, bool toPad);
    void decrypt(vector<uint8_t> &data, Matrix4x4 iv, bool isPadded);

    Matrix4x4 generateRandomMat();

private:
    uint8_t rcon(uint8_t round);
    uint8_t sBox(uint8_t input);
    uint8_t sBoxInv(uint8_t input);
    void rotate(Word &word, uint8_t by);
    void subWord(Word &word);
    void subBytes(Matrix4x4 &mat, bool isEnc);
    void shiftRows(Matrix4x4 &mat, bool isEnc);
    void mixColumns(Matrix4x4 &mat);
    void mixColumnsInv(Matrix4x4 &mat);
    void addRoundKey(Matrix4x4 &mat, uint8_t round);
    void xorEqual(Matrix4x4 &to, Matrix4x4 &with);

    void padData(vector<uint8_t> &data);
    void dePadData(vector<uint8_t> &data);

    void keyExpansion(array<uint8_t, BLOCK> key);

    void encryptMat(Matrix4x4 &mat);
    void decryptMat(Matrix4x4 &mat);

    Matrix4x4 &convertToMat(uint8_t *dataStart);

    std::array<std::array<uint8_t, BLOCK>, R + 1> keys;

    // Lookup table for x • 2 in GF(2^8)
    static const array<uint8_t, 256> mul2;
    // Lookup table for x • 3 in GF(2^8)
    static const array<uint8_t, 256> mul3;
    // Lookup table for x • 9 in GF(2^8)
    static const array<uint8_t, 256> mul9;
    // Lookup table for x • 11 in GF(2^8)
    static const array<uint8_t, 256> mul11;
    // Lookup table for x • 13 in GF(2^8)
    static const array<uint8_t, 256> mul13;
    // Lookup table for x • 14 in GF(2^8)
    static const array<uint8_t, 256> mul14;

    static bool randInit;
};
#endif