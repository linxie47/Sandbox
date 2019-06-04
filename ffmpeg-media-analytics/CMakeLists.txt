# Copyright (c) 2019 Intel Corporation

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#      http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.8)

project(openvino_ie_wrapper)

include(GNUInstallDirs)

set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 1)
set(PROJECT_VERSION_PATCH 0)

set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

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

if (NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-std=c++11 -fPIC ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")

if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    add_definitions(-fvisibility=default)
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/library_config/ie_wrapper.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ie_wrapper.pc" @ONLY)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/ie_wrapper.pc" DESTINATION lib/pkgconfig)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/inference_backend/image_inference/openvino/ie_cpu_extension/libcpu_extension.so" DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}")

function (set_compile_flags TARGET)
    target_compile_options(${TARGET} PRIVATE $<$<COMPILE_LANGUAGE:C>:${C_FLAGS}> $<$<COMPILE_LANGUAGE:CXX>:${CXX_FLAGS}>)
endfunction(set_compile_flags)

configure_file(cmake/config.h.in configs/config.h @ONLY)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/configs)

add_subdirectory(inference_backend)
add_subdirectory(ie_wrapper)
add_subdirectory(tests)
