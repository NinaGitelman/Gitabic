//
// Created by user on 25.11.2024
//
#include "VectorUint8Utils.h"

// Prints the contents of the vector as bytes
void VectorUint8Utils::printVectorUint8(const std::vector<uint8_t> &vec)
{
    ThreadSafeCout::cout(vectorUint8ToString(vec));
}

// Converts the vector to a C-style char array
void VectorUint8Utils::vectorUint8ToCharArray(const std::vector<uint8_t>& vec, char** charArr)
{
    // Allocate memory for the char array
    *charArr = new char[vec.size() + 1]; // +1 for null termination

    for (size_t i = 0; i < vec.size(); ++i)
    {
        (*charArr)[i] = static_cast<char>(vec[i]); // Access the allocated array correctly
    }
    (*charArr)[vec.size()] = '\0'; // Null-terminate if you treat it as a string
}

// Converts the vector to a std::string and prints it
std::string VectorUint8Utils::vectorUint8ToString(const std::vector<uint8_t>& vec)
{
    std::string str(vec.begin(), vec.end());

    return str;
}


std::vector<uint8_t> VectorUint8Utils::readFromCin()
{
    std::vector<uint8_t> buffer;

    // Read the input as a string to handle any kind of data
    std::string input;
    std::getline(std::cin, input);

    // Convert the string to a vector of uint8_t
    buffer.assign(input.begin(), input.end());

    return buffer;
}
