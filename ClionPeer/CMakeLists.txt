cmake_minimum_required(VERSION 3.30)
project(ClionPeer)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g")

# Automatically find all source and header files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/*.cpp" "${CMAKE_SOURCE_DIR}/*.h")

# Add executable target
add_executable(ClionPeer ${SOURCES})

# Find libnice using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBNICE REQUIRED nice)

# Include libnice directories
include_directories(${LIBNICE_INCLUDE_DIRS})
link_directories(${LIBNICE_LIBRARY_DIRS})

# Link libnice libraries
target_link_libraries(ClionPeer PRIVATE ${LIBNICE_LIBRARIES} rt pthread)
