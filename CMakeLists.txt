cmake_minimum_required(VERSION 2.6)
project(raft)

enable_testing()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

if(NOT CMAKE_STUB_FORMATTER)
    set(CMAKE_STUB_FORMATTER echo)
endif()

set(CXX_FLAGS
        -fno-omit-frame-pointer # linux perf
        -Wall
        -Wextra
        -Werror
        -Wconversion
        -Wno-unused-parameter
        -Wold-style-cast
        -Woverloaded-virtual
        -Wpointer-arith
        -Wshadow
        -Wwrite-strings
        -std=c++17
        -march=native
        -rdynamic)
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

find_path(LEVELDB_INCLUDE_PATH NAMES leveldb/db.h)
find_library(LEVELDB_LIB NAMES leveldb)
if ((NOT LEVELDB_INCLUDE_PATH) OR (NOT LEVELDB_LIB))
    message(FATAL_ERROR "Fail to find leveldb")
endif()

include_directories(
        raft
        3rd/jrpc
        3rd/jrpc/3rd/tinyev
        3rd/jrpc/3rd/jackson
        ${LEVELDB_INCLUDE_PATH}
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_BINARY_DIR})
add_subdirectory(3rd/jrpc)
add_subdirectory(raft)
add_subdirectory(example)