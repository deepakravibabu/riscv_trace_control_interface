cmake_minimum_required(VERSION 3.22.2)

project(tci)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME}
    main.cpp
    traceEncoder.h
    traceFunnel.h
    TraceRamSink.h
    IMmioDevice.h
    MmioBus.h
)

