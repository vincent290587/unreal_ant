cmake_minimum_required(VERSION 3.19)
project(ant_plus_plugin LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_EXPORT_COMPILE_COMMANDS				ON)

if(NOT MSVC)
    add_compile_options(-O0)
    add_compile_options(-Werror -Wno-attributes -Wenum-compare -Wno-deprecated-declarations)
else()
    message("Using MSVC compiler")
endif()

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/
)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ANT-SDK_PC.3.5/ANT_LIB ./libs/ant_plus)

set(SOURCES
        ue5_lib.cpp
        ant_receive_ch.cpp
        )

add_library(${PROJECT_NAME} STATIC ${SOURCES})
 
target_include_directories(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/
)

target_link_libraries(${PROJECT_NAME} LINK_PUBLIC ANT_LIB)
