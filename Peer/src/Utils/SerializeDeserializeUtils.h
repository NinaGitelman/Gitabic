
#pragma once
#include <iostream>
#include <vector>
#include <stdio.h>

using std::vector;

class SerializeDeserializeUtils
{
public:
   /// @brief Function to serailize a short (uint16_t) into a vector to then copy into the serialized vector
   /// @param num the unit16_t to serialize
   /// @return the serialized vector (size 2)
   static void serializeUint16IntoVector(vector<uint8_t> &buffer, uint16_t num);

   static void addToFront(vector<uint8_t> &to, const vector<uint8_t> &from);
   static void addToEnd(vector<uint8_t> &to, const vector<uint8_t> &from);

   template <size_t N>
   static void addToFront(std::vector<uint8_t> &to, const std::array<uint8_t, N> &from);
   template <size_t N>
   static void addToEnd(std::vector<uint8_t> &to, const std::array<uint8_t, N> &from);
};