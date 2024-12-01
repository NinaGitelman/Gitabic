//
// Created by user on 24.11.2024 
//
#include "ThreadSafeCout.h"
std::mutex ThreadSafeCout::_coutMutex;

void ThreadSafeCout::cout(const std::string& text)
{
    std::unique_lock<std::mutex> lock(ThreadSafeCout::_coutMutex);
    std::cout << text;

}
