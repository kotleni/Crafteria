project(platform)

if(UNIX)
    set(SOURCES
        linux.cpp
    )
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

add_library(platform SHARED ${SOURCES})
target_include_directories(platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})