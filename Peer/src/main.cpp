#include <iostream>
#include "Utils/MetaDataFile/MetaDataFile.h"

int main()
{
    auto a = MetaDataFile::createMetaData("../test.txt", "12345678", Address(), "Uri");
}