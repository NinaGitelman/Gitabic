#pragma once
#include <mutex>
#include <string>
#include <iostream>

class ThreadSafeCout {
private:
    static std::mutex _coutMutex;

public:
    // Public static instance for easy access
    static ThreadSafeCout print;

    // Thread-safe cout function
    static void cout(const std::string &text);

    // Thread-safe << operator
    template<typename T>
    friend ThreadSafeCout &operator<<(ThreadSafeCout &tsCout, const T &value) {
        std::unique_lock<std::mutex> lock(ThreadSafeCout::_coutMutex);
        std::cout << value;
        return tsCout;
    }

    // Handle std::endl and other manipulators
    friend ThreadSafeCout &operator<<(ThreadSafeCout &tsCout, std::ostream & (*manip)(std::ostream &)) {
        std::unique_lock<std::mutex> lock(ThreadSafeCout::_coutMutex);
        std::cout << manip;
        return tsCout;
    }
};
