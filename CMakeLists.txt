cmake_minimum_required(VERSION 3.22.2)

project(tci VERSION 0.1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(TCI_BUILD_GTESTS "Build GoogleTest unit tests" ON)

add_library(tci_lib INTERFACE)

target_include_directories(tci_lib INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# --------- Demo App --------- 
add_executable(tci_demo
    src/main.cpp
)

target_link_libraries(tci_demo PRIVATE tci_lib)

# --------- GoogleTest --------- 
if(TCI_BUILD_GTESTS)
    include(FetchContent)

    FetchContent_Declare(
    googletest
    URL
        https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip
    )

    # For Windows/MSVC: use the same runtime as the parent project
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    # --------- Unit Tests ---------
    add_executable(tci_gtests
        tests/gtest_tci.cpp
    )

    target_link_libraries(tci_gtests
        PRIVATE
        tci_lib
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(tci_gtests)
endif()