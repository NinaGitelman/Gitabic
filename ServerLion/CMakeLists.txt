cmake_minimum_required(VERSION 3.20)
project(ServerLion)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Exclude CMake's compiler identification file
file(GLOB_RECURSE SOURCES
        CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/*.cpp"
        "${CMAKE_SOURCE_DIR}/*.h"
)

list(FILTER SOURCES EXCLUDE REGEX "CMakeFiles/.*")

# Create the executable
add_executable(ServerLion ${SOURCES})

# Link necessary libraries
target_link_libraries(ServerLion pthread)