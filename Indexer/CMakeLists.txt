cmake_minimum_required(VERSION 3.26)
project(
    indexer
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)

add_executable(indexer
        main.cpp
        Indexer.cpp)

add_library(Indexer Indexer.cpp "Indexer.h")