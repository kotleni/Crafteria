# project(crafteria)
cmake_minimum_required(VERSION 3.2)

add_subdirectory(client/platform)

add_executable(crafteria)
link_libraries(crafteria platform)