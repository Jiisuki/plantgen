add_library(Writer writer.cpp)
target_include_directories(Writer INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(Writer Common)
