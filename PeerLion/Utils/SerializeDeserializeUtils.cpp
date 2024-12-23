
#include "SerializeDeserializeUtils.h"


template<SafeToCopy T>
void SerializeDeserializeUtils::addToFront(vector<uint8_t> to, T from) {
    to.resize(to.size() + sizeof(T));
    for (size_t i = sizeof(T); i < to.size(); ++i) {
        to[i] = to[i - (to.size() - sizeof(T))];
    }
    memcpy(to.data(), &from, sizeof(T));
}

template<SafeToCopy T>
void SerializeDeserializeUtils::addToEnd(vector<uint8_t> to, T from) {
    to.resize(to.size() + sizeof(T));
    memcpy(to.data() + to.size() - sizeof(T), &from, sizeof(T));
}

void SerializeDeserializeUtils::serializeUint16IntoVector(vector<uint8_t> &buffer, uint16_t num) {
    if (buffer.size() < 2) {
        throw std::runtime_error("Invalid buffer sent to serializeUint16ToVector smaller than 2 bytes");
    }
    buffer[0] = static_cast<uint8_t>(num & 0xFF); // Lower byte
    buffer[1] = static_cast<uint8_t>((num >> 8) & 0xFF); // Upper byte
}

void SerializeDeserializeUtils::addToEnd(vector<uint8_t> &to, const vector<uint8_t> &from) {
    to.insert(to.end(), from.begin(), from.end());
}

void SerializeDeserializeUtils::addToFront(vector<uint8_t> &to, const vector<uint8_t> &from) {
    to.insert(to.begin(), from.begin(), from.end());
}
