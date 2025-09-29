cmake_minimum_required(VERSION 3.15)
project(LibTokaMapRustBridge CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find libtokamap
find_library(LIBTOKAMAP_LIBRARY
    NAMES tokamap libtokamap
    PATHS
        ${CMAKE_SOURCE_DIR}/../build/lib
        ${CMAKE_SOURCE_DIR}/../install/lib
        /usr/local/lib
        /usr/lib
)

if(NOT LIBTOKAMAP_LIBRARY)
    message(FATAL_ERROR "libtokamap library not found. Please build libtokamap first.")
endif()

# Include directories
set(LIBTOKAMAP_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/../src
    ${CMAKE_SOURCE_DIR}/../ext_include
    ${CMAKE_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/../build/include
)

# Test if we can compile the bridge files
add_library(rust_bridge_test STATIC
    src/bridge.cpp
    src/bridge.hpp
)

target_include_directories(rust_bridge_test PRIVATE
    ${LIBTOKAMAP_INCLUDE_DIRS}
)

target_link_libraries(rust_bridge_test
    ${LIBTOKAMAP_LIBRARY}
)

target_compile_options(rust_bridge_test PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra>
)

# Disable specific warnings for the bridge code
target_compile_options(rust_bridge_test PRIVATE
    -Wno-unused-parameter
    -Wno-unused-variable
)

message(STATUS "LibTokaMap library: ${LIBTOKAMAP_LIBRARY}")
message(STATUS "Include directories: ${LIBTOKAMAP_INCLUDE_DIRS}")
