//
// Created by user on 25.11.2024 
//
#pragma once
#include <vector>
#include <iterator>
#include "ThreadSafeCout.h"

using std::vector;
class VectorUint8Utils
{
public:
    /// @brief Function prints the given vector (thread safe with threadSafeCout)
    static void printVectorUint8(vector<uint8_t> vec);



    /// @brief Function takes the input vector and puts into the given charArray    
    static void vectorUint8ToCharArray(vector<uint8_t> vec, char** charArr);

    /// @brief Function puts the given vector into a string
    /// @param vec the vector
    /// @return the strign from the vector
    static std::string vectorUint8ToString(vector<uint8_t> vec);


};