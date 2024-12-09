cmake_minimum_required(VERSION 3.30)
project(ClionPeer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find libnice and related packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(NICE REQUIRED nice)
pkg_check_modules(GLIB REQUIRED glib-2.0)

# Exclude CMake's compiler identification file
file(GLOB_RECURSE SOURCES
        CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/*.cpp"
        "${CMAKE_SOURCE_DIR}/*.h"
)

list(FILTER SOURCES EXCLUDE REGEX "CMakeFiles/.*")

# Create the executable
add_executable(ClionPeer ${SOURCES})

# Include directories
target_include_directories(ClionPeer PRIVATE
        ${NICE_INCLUDE_DIRS}
        ${GLIB_INCLUDE_DIRS}
)

# Link necessary libraries
target_link_libraries(ClionPeer
        pthread
        ${NICE_LIBRARIES}
        ${GLIB_LIBRARIES}
)

# Add compiler flags
target_compile_options(ClionPeer PRIVATE
        ${NICE_CFLAGS}
        ${GLIB_CFLAGS}
)