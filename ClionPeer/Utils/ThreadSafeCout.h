//
// Created by user on 24.11.2024 
//
#pragma once
#include <mutex>
#include <string>
#include <stdio.h>
#include <iostream>

class ThreadSafeCout
{
    private:

    static std::mutex _coutMutex;

    public:
    static void cout(const std::string& text);

};