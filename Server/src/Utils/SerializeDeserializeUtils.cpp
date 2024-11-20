
#include "SerializeDeserializeUtils.h"


void SerializeDeserializeUtils::serializeUint16IntoVector(vector<uint8_t>& buffer, uint16_t num)
{
    if(buffer.size() < 2)
    {
        throw std::runtime_error("Invalid buffer sent to serializeUint16ToVector smaller than 2 bytes");
    }
    buffer[0] = static_cast<uint8_t>(num & 0xFF);       // Lower byte
    buffer[1] = static_cast<uint8_t>((num >> 8) & 0xFF); // Upper byte

}
