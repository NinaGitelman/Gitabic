//
// Created by user on 24.11.2024 
//
#include "ThreadSafeCout.h"
void ThreadSafeCout::threadSafeCout(std::string text)
{
    std::unique_lock<std::mutex> lock(_coutMutex);
    std::cout << text;

}
