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

cmake_minimum_required(VERSION 2.8.5)

project(InferenceEngine_C_API)

include(GNUInstallDirs)

set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 1)
set(C_API_VERSION_SO 1)

set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

# Find InferenceEngine
find_package(InferenceEngine 1.0)
if (NOT InferenceEngine_FOUND)
    message(FATAL_ERROR "")
endif()

option(BUILD_SHARED_LIBS "Build shared libraries" ON)

#inference_engine_c_wrapper
set(C_API_LIB inference_engine_c_wrapper)

#set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${Project_BINARY_DIR}/${CMAKE_BUILD_TYPE}/lib)

file (GLOB SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ie_c_wrapper/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ie_c_wrapper/*.c
        )

file (GLOB HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
        )

if (BUILD_SHARED_LIBS)
    set(C_API_LIBRARY_TYPE SHARED)
else()
    set(C_API_LIBRARY_TYPE STATIC)
endif()

add_library("${C_API_LIB}" "${C_API_LIBRARY_TYPE}" "${HEADERS}" "${SOURCES}")
if (NOT WIN32)
    target_link_libraries("${C_API_LIB}" m)
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

target_include_directories(${C_API_LIB} SYSTEM PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# tricky way to generate a dldt pkgconfig file to make compatible with open source dldt
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/library_config/dldt.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/dldt.pc" @ONLY)

install(FILES "include/ie_api_wrapper.h" DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}/dldt")
install(FILES "include/ie_common_wrapper.h" DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}/dldt")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/dldt.pc" DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig")
install(TARGETS "${C_API_LIB}" DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}")

set_target_properties("${C_API_LIB}"
    PROPERTIES
    "CMAKE_CXX_FLAGS" "${CMAKE_CXX_FLAGS} -fPIE" COMPILE_PDB_NAME ${C_API_LIB}
        SOVERSION "${C_API_VERSION_SO}"
        VERSION "${PROJECT_VERSION}")

target_link_libraries("${C_API_LIB}" IE::ie_cpu_extension ${InferenceEngine_LIBRARIES})

