#pragma once
#include <mutex>
#include <iostream>
#include <string>

class ThreadSafeCout {
private:
    static std::mutex _coutMutex;

    // Private constructor to prevent instantiation
    ThreadSafeCout() = default;

public:
    // Static method for simple output
    template<typename T>
    static void cout(const T &value, const std::string &end = "\n") {
        std::lock_guard<std::mutex> lock(_coutMutex);
        std::cout << value << end;
    }

    // Overload operator<< for chained output
    template<typename T>
    friend ThreadSafeCout &operator<<(ThreadSafeCout &tsCout, const T &value);

    // Provide a static instance
    static ThreadSafeCout &instance() {
        static ThreadSafeCout instance;
        return instance;
    }
};
