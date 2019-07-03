#*******************************************************************************
#  Copyright (C) 2018-2019 Intel Corporation
#
#  SPDX-License-Identifier: MIT
#*******************************************************************************

cmake_minimum_required(VERSION 2.8.5)

project(ImageInference)

find_package(PkgConfig REQUIRED)
include(FindPkgConfig)

pkg_check_modules(SWSCALE REQUIRED "libswscale")
include_directories(${SWSCALE_INCLUDE_DIRS})
link_directories(${SWSCALE_LIBRARY_DIRS})

file (GLOB SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
    )

file (GLOB HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
    )

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_FFMPEG")

set(TARGET_NAME image_inference_test)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(${TARGET_NAME} ${HEADERS} ${SOURCES})

# TODO: remove when it becomes a library
add_subdirectory(dldt_c_api)

target_link_libraries(${TARGET_NAME} swscale pthread dldt_ie_c_wrapper)
