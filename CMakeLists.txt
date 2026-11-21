cmake_minimum_required(VERSION 3.20)

project(Flix
    VERSION 0.1.0
    DESCRIPTION "Flix Programming Language"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(flix
    src/main.cpp
    src/lexer/lexer.cpp
    src/parser/parser.cpp
)

target_include_directories(flix PRIVATE src)

if(MSVC)
    target_compile_options(flix PRIVATE /W4 /permissive-)
else()
    target_compile_options(flix PRIVATE -Wall -Wextra -Wpedantic)
endif()
