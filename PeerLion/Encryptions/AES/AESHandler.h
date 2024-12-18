#ifndef AESHANDLER_H
#define AESHANDLER_H
#pragma once

#include <array>
#include <iostream>
#include <cstdint>
#include <cstring>
#include <vector>
#include <span>
#include <iomanip>

#define BLOCK 16 // Block size in bytes (128 bits)
#define N 4      // Number of columns (state size)
#define WORD 4   // Word size in bytes
#define R 10     // Number of rounds for AES-128

using std::array;
using std::vector;
using Word = array<uint8_t, WORD>;
using Matrix4x4 = array<Word, 4>;
using AESKey = array<uint8_t, BLOCK>;

/// @brief AES encryption and decryption handler for AES-128 in ECB or CBC mode
class AESHandler {
public:
    /// @brief Default constructor. Initializes random seed if needed.
    AESHandler();

    /// @brief Constructor with a specific key.
    /// @param key 128-bit key used for AES encryption and decryption.
    AESHandler(AESKey key);

    /// @brief Default destructor.
    ~AESHandler() = default;

    /// @brief Encrypts a vector of data in place.
    /// @param data Input data to be encrypted. Modified in-place.
    /// @param toPad If true, data is padded to a multiple of the block size.
    /// @param cbc If true, encrypts in CBC mode; otherwise, uses ECB mode.
    /// @return The IV used in CBC mode. Ignored in ECB mode.
    Matrix4x4 encrypt(vector<uint8_t> &data, bool toPad = true, bool cbc = true);

    /// @brief Decrypts a vector of data.
    /// @param data Encrypted data to be decrypted. Modified in-place.
    /// @param iv IV used for CBC mode. If nullptr, ECB mode.
    /// @param isPadded If true, removes padding after decryption.
    void decrypt(vector<uint8_t> &data, Matrix4x4 *iv = nullptr, bool isPadded = true);

    /// @brief Generates a random 4x4 matrix used for IV or other purposes.
    /// @return A random 4x4 matrix.
    Matrix4x4 generateRandomMat();

    /// @brief Generates a random 128-bit AES key.
    /// @return A random 128-bit key.
    static AESKey generateRandomKey();

    /// @brief Gets the currently active AES key.
    /// @return The current AES key.
    AESKey getKey();

    /// @brief Prints a hexadecimal dump of a 4x4 matrix.
    /// @param matrix The matrix to be printed.
    void printHexDump(const Matrix4x4 &matrix);

    /// @brief Prints a hexadecimal dump of a 128-bit key.
    /// @param key The key to be printed.
    void printHexDump(const AESKey &key);

private:
    /// @brief Returns the round constant for a specific round (log in GF of the round).
    /// @param round The round number.
    /// @return The round constant.
    uint8_t rcon(uint8_t round);

    /// @brief Applies the AES S-box substitution.
    /// @param input Byte to substitute.
    /// @return Substituted byte.
    uint8_t sBox(uint8_t input);

    /// @brief Applies the inverse AES S-box substitution.
    /// @param input Byte to substitute.
    /// @return Substituted byte.
    uint8_t sBoxInv(uint8_t input);

    /// @brief Rotates the bytes in a word.
    /// @param word The word to rotate.
    /// @param by Number of positions to rotate by.
    void rotate(Word &word, int8_t by);

    /// @brief Substitutes each byte in a word using the AES S-box.
    /// @param word The word to substitute.
    void subWord(Word &word);

    /// @brief Substitutes bytes in the matrix (state).
    /// @param mat The matrix to substitute.
    /// @param isEnc True for encryption, false for decryption.
    void subBytes(Matrix4x4 &mat, bool isEnc);

    /// @brief Shifts rows in the matrix (state).
    /// @param mat The matrix to shift rows.
    /// @param isEnc True for encryption, false for decryption.
    void shiftRows(Matrix4x4 &mat, bool isEnc);

    /// @brief Applies the MixColumns transformation to the matrix.
    /// @param mat The matrix to transform.
    void mixColumns(Matrix4x4 &mat);

    /// @brief Applies the inverse MixColumns transformation to the matrix.
    /// @param mat The matrix to transform.
    void mixColumnsInv(Matrix4x4 &mat);

    /// @brief Adds the round key to the matrix (state).
    /// @param mat The matrix to modify.
    /// @param round The round number.
    void addRoundKey(Matrix4x4 &mat, uint8_t round);

    /// @brief Performs an XOR operation between two matrices.
    /// @param to The target matrix to modify.
    /// @param with The matrix to XOR with.
    void xorEqual(Matrix4x4 &to, Matrix4x4 &with);

    /// @brief Pads data to a multiple of the block size.
    /// @param data The data to pad.
    void padData(vector<uint8_t> &data);

    /// @brief Removes padding from data.
    /// @param data The data to unpad.
    void dePadData(vector<uint8_t> &data);

    /// @brief Expands the key into round keys.
    /// @param key The key to expand.
    void keyExpansion(AESKey key);

    /// @brief Encrypts a single 4x4 matrix (block).
    /// @param mat The matrix to encrypt.
    void encryptMat(Matrix4x4 &mat);

    /// @brief Decrypts a single 4x4 matrix (block).
    /// @param mat The matrix to decrypt.
    void decryptMat(Matrix4x4 &mat);

    /// @brief Converts raw data to a 4x4 matrix reference.
    /// @param dataStart Pointer to the start of the raw data.
    /// @return Reference to a 4x4 matrix.
    Matrix4x4 &convertToMat(uint8_t *dataStart);

    /// @brief Expanded round keys.
    std::array<AESKey, R + 1> keys;

    /// Lookup table for x • 2 in GF(2^8)
    static const array<uint8_t, 256> mul2;
    /// Lookup table for x • 3 in GF(2^8)
    static const array<uint8_t, 256> mul3;
    /// Lookup table for x • 9 in GF(2^8)
    static const array<uint8_t, 256> mul9;
    /// Lookup table for x • 11 in GF(2^8)
    static const array<uint8_t, 256> mul11;
    /// Lookup table for x • 13 in GF(2^8)
    static const array<uint8_t, 256> mul13;
    /// Lookup table for x • 14 in GF(2^8)
    static const array<uint8_t, 256> mul14;

    /// @brief Indicates whether the random seed has been initialized.
    static bool randInit;
};

#endif
