
#pragma once
#include <iostream>
#include <vector>
#include <array>
#include <cstdint>
#include <type_traits>

// Trait to check if a type holds only inline data
template<typename T>
struct is_safe_to_copy {
   static constexpr bool value =
         std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;
};

// Helper variable template
template<typename T>
constexpr bool is_safe_to_copy_v = is_safe_to_copy<T>::value;

// Concept for safe-to-copy types
template<typename T>
concept SafeToCopy = is_safe_to_copy_v<T>;

using std::vector;

class SerializeDeserializeUtils {
public:
   /// @brief Function to serailize a short (uint16_t) into a vector to then copy into the serialized vector
   /// @param buffer the buffer to serialize into
   /// @param num the unit16_t to serialize
   /// @return the serialized vector (size 2)
   static void serializeUint16IntoVector(vector<uint8_t> &buffer, uint16_t num);

   static void addToFront(vector<uint8_t> &to, const vector<uint8_t> &from);

   static void addToEnd(vector<uint8_t> &to, const vector<uint8_t> &from);

   template<typename T>
   static void copyToFront(vector<uint8_t> to, T from);

   template<typename T>
   static void copyToEnd(vector<uint8_t> to, T from) {
      to.resize(to.size() + sizeof(T));
      memcpy(to.data() + to.size() - sizeof(T), &from, sizeof(T));
   }

   template<size_t S>
   static void addToFront(std::vector<uint8_t> &to, const std::array<uint8_t, S> &from) {
      to.insert(to.begin(), from.begin(), from.end());
   }

   template<size_t S>
   static void addToEnd(std::vector<uint8_t> &to, const std::array<uint8_t, S> &from) {
      to.insert(to.end(), from.begin(), from.end());
   }
};


