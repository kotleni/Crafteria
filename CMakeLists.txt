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
        client/World/World.cpp
        client/Player.cpp
        client/World/Chunk.cpp
        client/World/Block.cpp
        client/World/BakedChunkPart.cpp
        client/World/BakedChunk.cpp
        client/World/Generator/DefaultWorldGenerator.cpp
)
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES} ${GLEW_LIBRARIES} OpenGL::GL)