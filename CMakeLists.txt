cmake_minimum_required(VERSION 3.5)

project(crafteria)
set(CMAKE_CXX_STANDARD 20)

find_package(OpenGL REQUIRED COMPONENTS OpenGL)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})

find_package(glm REQUIRED)
include_directories(${glm_INCLUDE_DIRS})

add_executable(crafteria
        client/Math/Vec3i.cpp
        client/Shader.cpp
        client/Image.cpp
        client/main.cpp
        client/World.cpp
        client/Player.cpp
        client/Chunk.cpp
        client/Block.cpp
        client/BakedChunkPart.cpp
        client/BakedChunk.cpp
        client/World/Generator/AbstractWorldGenerator.h
        client/World/Generator/DefaultWorldGenerator.cpp
        client/World/Generator/DefaultWorldGenerator.h
)
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES} ${GLEW_LIBRARIES} OpenGL::GL)