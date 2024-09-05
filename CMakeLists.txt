cmake_minimum_required(VERSION 3.28)
project(JSON)

set(CMAKE_CXX_STANDARD 20)

include_directories(src
        include
)

set(SRC_FILES
        include/json.hpp
        src/json.cpp)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

# add_subdirectory(ChunkedList)