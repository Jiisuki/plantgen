add_library(Reader reader.cpp)
target_include_directories(Reader INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(Reader Common)
