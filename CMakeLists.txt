cmake_minimum_required(VERSION 3.5)

project(crafteria)
set(CMAKE_CXX_STANDARD 20)

set(USE_EXT_DEPENDENCIES_FROM_PACKAGE_MANAGER false)
if (UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif ()

if (LINUX)
    find_package(OpenGL REQUIRED COMPONENTS OpenGL)
else ()
    message(FATAL_ERROR "Unsupported OS!")
endif ()

if (USE_EXT_DEPENDENCIES_FROM_PACKAGE_MANAGER)
    find_package(SDL3 REQUIRED)
    find_package(glm REQUIRED)
else ()
    add_subdirectory(ThirdParty/SDL/ EXCLUDE_FROM_ALL)
    add_subdirectory(ThirdParty/glm/ EXCLUDE_FROM_ALL)
endif ()

include_directories(${SDL3_INCLUDE_DIRS})
include_directories(${glm_INCLUDE_DIRS})

add_executable(crafteria
        client/Math/Vec3i.cpp
        client/Math/Ray.cpp
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
        client/Render/ChunksRenderer.cpp

        client/GL/glad.c

        # imgui
        client/GUI/imgui.cpp
        client/GUI/imgui_impl_sdl3.cpp
        client/GUI/imgui_impl_opengl3.cpp
        client/GUI/imgui_widgets.cpp
        client/GUI/imgui_tables.cpp
        client/GUI/imgui_draw.cpp
)
target_link_libraries(${PROJECT_NAME} SDL3::SDL3 OpenGL::GL glm::glm)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
include(CPack)