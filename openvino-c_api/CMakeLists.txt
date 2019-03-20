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

cmake_minimum_required (VERSION 2.8)

set (TARGET_NAME "inference_engine_c_wrapper")

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${Project_BINARY_DIR}/${CMAKE_BUILD_TYPE}/lib)

file (GLOB MAIN_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
        )

file (GLOB MAIN_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
        )

source_group("src" FILES ${MAIN_SRC})
source_group("include" FILES ${MAIN_HEADERS})

add_library(${TARGET_NAME} SHARED ${MAIN_SRC} ${MAIN_HEADERS})

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

# Find InferenceEngine
find_package(InferenceEngine 1.0)
if (NOT InferenceEngine_FOUND)
    message(FATAL_ERROR "")
endif()

target_include_directories(${TARGET_NAME} SYSTEM PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

set_target_properties(${TARGET_NAME} PROPERTIES "CMAKE_CXX_FLAGS" "${CMAKE_CXX_FLAGS} -fPIE" COMPILE_PDB_NAME ${TARGET_NAME})
target_link_libraries(${TARGET_NAME} IE::ie_cpu_extension ${InferenceEngine_LIBRARIES})
