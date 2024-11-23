#include <iostream>
#include "Utils/FileUtils/FileUtils.h"

int main()
{
    using namespace Utils;
    auto path = "./blah.txt";
    FileUtils::createFilePlaceHolder(path, 301);
    vector<uint8_t> v(30, 'a');
    vector<uint8_t> v1(30, 'b');
    FileUtils::writeChunkToFile(v, path, 10);
    FileUtils::writeChunkToFile(v1, path, 50);
    v = FileUtils::readFileToVector(path);
    for (auto &&i : v)
    {
        std::cout << i;
    }
}