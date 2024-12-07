#pragma once
#include <mutex>
#include <string>
#include <iostream>

class ThreadSafeCout
{
private:

    static std::mutex _coutMutex;

public:
    static void cout(const std::string& text);

};