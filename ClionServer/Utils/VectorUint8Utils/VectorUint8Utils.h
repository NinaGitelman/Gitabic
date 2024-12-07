//
// Created by user on 25.11.2024
//
#pragma once
#include <vector>
#include "../ThreadSafeCout/ThreadSafeCout.h"

using std::vector;
class VectorUint8Utils
{
public:
    /// @brief Function prints the given vector (thread safe with threadSafeCout)
    static void printVectorUint8(const vector<uint8_t> &vec);

    /// for debugging to input into vector uint8
    static vector<uint8_t> readFromCin();


    /// @brief Function takes the input vector and puts into the given charArray
    static void vectorUint8ToCharArray(const vector<uint8_t>& vec, char** charArr);

    /// @brief Function puts the given vector into a string
    /// @param vec the vector
    /// @return the strign from the vector
    static std::string vectorUint8ToString(const vector<uint8_t>& vec);


};