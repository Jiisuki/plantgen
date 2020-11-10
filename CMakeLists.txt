cmake_minimum_required(VERSION 3.5)
project(plantgen)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS -O3)
set(CMAKE_CXX_FLAGS -Wall)
set(CMAKE_CXX_FLAGS -Wno-psabi)
set(CMAKE_CXX_FLAGS -Wno-unused-parameter)
set(CMAKE_CXX_FLAGS -Wno-unused-function)
set(CMAKE_CXX_FLAGS -Wextra)
set(CMAKE_CXX_FLAGS -Wmissing-include-dirs)
set(CMAKE_CXX_FLAGS -Wswitch-default)

# my $ldflags = "-static-libgcc -static-libstdc++";
add_subdirectory(src/application)
add_subdirectory(src/common)
add_subdirectory(src/lib/reader)
add_subdirectory(src/lib/writer)