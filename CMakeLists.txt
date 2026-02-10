cmake_minimum_required(VERSION 3.22.2)

project(tci VERSION 0.1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(tci
    src/main.cpp
)

target_include_directories(tci PRIVATE 
   ${CMAKE_CURRENT_SOURCE_DIR}/src)

   