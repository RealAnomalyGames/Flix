
cmake_minimum_required(VERSION 3.20)

project(Flix
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Statically link the MSVC runtime so users do not
# need to install the Visual C++ Redistributable.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY
        "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
endif()

# Shared Flix core
add_library(flix_core STATIC
    src/core/version.cpp
)

target_include_directories(flix_core PUBLIC
    src/core/include
)

# Interpreter
add_executable(flix
    src/interpreter/main.cpp
)

target_link_libraries(flix PRIVATE flix_core)

# Compiler
add_executable(flixc
    src/compiler/main.cpp
)

target_link_libraries(flixc PRIVATE flix_core)

# Output directory
set_target_properties(flix flixc PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(MSVC)
    target_compile_options(flix_core PRIVATE /W4)
    target_compile_options(flix PRIVATE /W4)
    target_compile_options(flixc PRIVATE /W4)
endif()
