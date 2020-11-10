add_executable(PlantGen plantgen.cpp)

target_link_libraries(PlantGen Common)
target_link_libraries(PlantGen Reader)
target_link_libraries(PlantGen Writer)

