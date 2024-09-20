cmake_minimum_required(VERSION 3.26)

project(
    searcher
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)

add_executable(searcher SearchEngine.cpp "SearchEngine.h")

add_library(Searcher SearchEngine.cpp "SearchEngine.h")
