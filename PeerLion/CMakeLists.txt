cmake_minimum_required(VERSION 3.20)
project(PeerLion)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Cross-platform Homebrew detection
if(APPLE)
    # Check for Apple Silicon vs Intel Mac
    execute_process(
            COMMAND uname -m
            OUTPUT_VARIABLE ARCH
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(ARCH STREQUAL "arm64")
        set(HOMEBREW_PREFIX "/opt/homebrew")
    else()
        set(HOMEBREW_PREFIX "/usr/local")
    endif()

    # Add Homebrew paths to PKG_CONFIG_PATH if they exist
    if(EXISTS "${HOMEBREW_PREFIX}/opt/libnice/lib/pkgconfig")
        set(ENV{PKG_CONFIG_PATH} "${HOMEBREW_PREFIX}/opt/libnice/lib/pkgconfig:${HOMEBREW_PREFIX}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    endif()
endif()

# Find libnice and related packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(NICE REQUIRED nice)
pkg_check_modules(GLIB REQUIRED glib-2.0)

# Debug: Print what pkg-config found
message(STATUS "NICE_LIBRARIES: ${NICE_LIBRARIES}")
message(STATUS "NICE_LIBRARY_DIRS: ${NICE_LIBRARY_DIRS}")
message(STATUS "NICE_INCLUDE_DIRS: ${NICE_INCLUDE_DIRS}")

# Exclude CMake's compiler identification file
file(GLOB_RECURSE SOURCES
        CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/*.cpp"
        "${CMAKE_SOURCE_DIR}/*.h"
)

list(FILTER SOURCES EXCLUDE REGEX "CMakeFiles/.*")

# Create the executable
add_executable(PeerLion ${SOURCES}
        Torrent/TorrentManager/TorrentManager.cpp
        Torrent/TorrentManager/TorrentManager.h)

# Include directories
target_include_directories(PeerLion PRIVATE
        ${NICE_INCLUDE_DIRS}
        ${GLIB_INCLUDE_DIRS}
)

# Add library directories - THIS IS KEY
target_link_directories(PeerLion PRIVATE
        ${NICE_LIBRARY_DIRS}
        ${GLIB_LIBRARY_DIRS}
)

# Link necessary libraries
target_link_libraries(PeerLion
        pthread
        ${NICE_LIBRARIES}
        ${GLIB_LIBRARIES}
)

# Add compiler flags
target_compile_options(PeerLion PRIVATE
        ${NICE_CFLAGS_OTHER}
        ${GLIB_CFLAGS_OTHER}
)