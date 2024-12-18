cmake_minimum_required(VERSION 3.30)
project(Lib)

set(CMAKE_CXX_STANDARD 20)

# Recursively find all .cpp files in Utils and Encryptions directories
file(GLOB_RECURSE LIB_SOURCES
        "Utils/*.cpp"
        "Encryptions/*.cpp"
        "Utils/*.h"
        "Encryptions/*.h"
)

add_library(Lib STATIC ${LIB_SOURCES})

# Optionally, add include directories
target_include_directories(Lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Utils
        ${CMAKE_CURRENT_SOURCE_DIR}/Encryptions
)