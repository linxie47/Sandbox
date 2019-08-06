#*******************************************************************************
#  Copyright (C) 2018-2019 Intel Corporation
#
#  SPDX-License-Identifier: MIT
#*******************************************************************************

cmake_minimum_required(VERSION 2.8.5)

project(ImageInference)

find_package(PkgConfig REQUIRED)
include(FindPkgConfig)

# ffmpeg
pkg_check_modules(AVFORMAT REQUIRED "libavformat")
include_directories(${AVFORMAT_INCLUDE_DIRS})
link_directories(${AVFORMAT_LIBRARY_DIRS})

pkg_check_modules(AVCODEC REQUIRED "libavcodec")
include_directories(${AVCODEC_INCLUDE_DIRS})
link_directories(${AVCODEC_LIBRARY_DIRS})

pkg_check_modules(AVFILTER REQUIRED "libavfilter")
include_directories(${AVFILTER_INCLUDE_DIRS})
link_directories(${AVFILTER_LIBRARY_DIRS})

pkg_check_modules(AVUTIL REQUIRED "libavutil")
include_directories(${AVUTIL_INCLUDE_DIRS})
link_directories(${AVUTIL_LIBRARY_DIRS})

pkg_check_modules(SWSCALE REQUIRED "libswscale")
include_directories(${SWSCALE_INCLUDE_DIRS})
link_directories(${SWSCALE_LIBRARY_DIRS})

# json-c
pkg_check_modules(JSONC REQUIRED "json-c")
include_directories(${JSONC_INCLUDE_DIRS})
link_directories(${JSONC_LIBRARY_DIRS})

# dldt_c_api
pkg_check_modules(DLDT_C_API REQUIRED "dldt_c_api")
include_directories(${DLDT_C_API_INCLUDE_DIRS})
link_directories(${DLDT_C_API_LIBRARY_DIRS})

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

target_link_libraries(${TARGET_NAME} pthread m ${DLDT_C_API_LIBRARIES} ${JSONC_LIBRARIES})

target_link_libraries(${TARGET_NAME} ${AVFORMAT_LIBRARIES} ${AVCODEC_LIBRARIES} ${AVFILTER_LIBRARIES} ${AVUTIL_LIBRARIES} ${SWSCALE_LIBRARIES})
