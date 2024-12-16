cmake_minimum_required(VERSION 3.16)
project(codegen)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS -Wall)
set(CMAKE_CXX_FLAGS -Wno-psabi)

include_directories(include)

add_library(uml_parser src/uml_parser.cpp)

add_executable(codegen
        src/codegen.cpp
        src/style.cpp
        src/writer.cpp)

target_link_libraries(codegen uml_parser)
