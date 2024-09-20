cmake_minimum_required(VERSION 3.12)

project(
    labwork11
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)


add_subdirectory(Searcher)
add_subdirectory(Indexer)


enable_testing()
add_subdirectory(tests)
