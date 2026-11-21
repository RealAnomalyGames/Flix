
cmake_minimum_required(VERSION 3.20)

project(Flix
    VERSION 0.1.0
    DESCRIPTION "Flix Programming Language"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Interpreter
add_executable(flix-interpreter
    src/interpreter/main.cpp
)

# Compiler
add_executable(flix-compiler
    src/compiler/main.cpp
)

# Include directories
target_include_directories(flix-interpreter PRIVATE include)
target_include_directories(flix-compiler PRIVATE include)

# Windows executable settings
if(WIN32)
    set_target_properties(
        flix-interpreter
        flix-compiler
        PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
endif()

# Release output directory
set_target_properties(
    flix-interpreter
    flix-compiler
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

message(STATUS "Flix version: ${PROJECT_VERSION}")
message(STATUS "Target architecture: x86_64 (initial target)")
