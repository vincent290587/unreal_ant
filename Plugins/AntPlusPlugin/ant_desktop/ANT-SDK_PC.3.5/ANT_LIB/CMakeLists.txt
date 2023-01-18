cmake_minimum_required(VERSION 3.7.2)

project(ANT_LIB VERSION 3.5.0 LANGUAGES C CXX)

set(SDK_INCLUDES
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/common
        ${CMAKE_CURRENT_SOURCE_DIR}/libraries
        ${CMAKE_CURRENT_SOURCE_DIR}/software/system
        ${CMAKE_CURRENT_SOURCE_DIR}/software/serial
        ${CMAKE_CURRENT_SOURCE_DIR}/software/USB
        ${CMAKE_CURRENT_SOURCE_DIR}/software/USB/devices
        ${CMAKE_CURRENT_SOURCE_DIR}/software/USB/device_handles
)

include_directories(${SDK_INCLUDES})

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/common    SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/libraries    SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/software/system    SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/software/serial    SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/software/USB/devices    SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/software/USB/device_handles    SRCS)

add_library(ANT_LIB STATIC
        ${SRCS})

target_include_directories(ANT_LIB PUBLIC ${SDK_INCLUDES})

set_target_properties(ANT_LIB PROPERTIES
#        PUBLIC_HEADER ${SDK_INCLUDES}
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        OUTPUT_NAME "ant_lib"
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Hex_Identity_ID_Goes_Here"
        )
