include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
    search_tests
    search_tests.cpp
)

target_link_libraries(
    search_tests
    Searcher
    GTest::gtest_main
)

# target_link_libraries(
#     search_tests
#     indexing
#     GTest::gtest_main
# )

target_include_directories(search_tests PUBLIC ${PROJECT_SOURCE_DIR})

include(GoogleTest)

gtest_discover_tests(search_tests)