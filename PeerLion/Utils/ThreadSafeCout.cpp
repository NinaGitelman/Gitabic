#include "ThreadSafeCout.h"

// Initialize the static mutex
std::mutex ThreadSafeCout::_coutMutex;

// Initialize the static instance
ThreadSafeCout ThreadSafeCout::print;

void ThreadSafeCout::cout(const std::string &text) {
	std::unique_lock<std::mutex> lock(_coutMutex);
	std::cout << text << std::endl;
}
