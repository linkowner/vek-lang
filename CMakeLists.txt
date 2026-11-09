cmake_minimum_required(VERSION 3.15)

project(VekLang LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The driver sources sit at the project root and include the library headers
# as "./headers/xxx.hpp", which resolves relative to the including file's own
# directory - so header lookup works from any build directory. Adding the
# source root to the include path also covers bare "headers/xxx.hpp" includes.
add_executable(vek vek.cpp)
target_include_directories(vek PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(vekvm vekvm.cpp)
target_include_directories(vekvm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT MSVC)
    add_compile_options(-Wall -Wextra)
else()
    add_compile_options(/W3)
endif()
