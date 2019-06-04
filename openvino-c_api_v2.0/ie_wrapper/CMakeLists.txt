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

cmake_minimum_required(VERSION 3.1)

set(TARGET_NAME "ie_wrapper")

file (GLOB MAIN_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/proc_factory/*.cpp
        )

file (GLOB MAIN_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/proc_factory/*.h
        )
        
add_library(${TARGET_NAME} SHARED ${MAIN_SRC} ${MAIN_HEADERS})
set_compile_flags(${TARGET_NAME})

set(CMAKE_CXX_FLAGS "-std=c++11 -fpermissive -fPIE ${CMAKE_CXX_FLAGS}")

target_include_directories(${TARGET_NAME}
PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/
        ${CMAKE_CURRENT_SOURCE_DIR}/include/
        ${CMAKE_CURRENT_SOURCE_DIR}/proc_factory/
        ${CMAKE_CURRENT_SOURCE_DIR}/../inference_backend/include/
)

target_link_libraries(${TARGET_NAME}
PRIVATE
        image_inference_openvino pre_proc opencv_pre_proc logger
)

target_link_libraries(${TARGET_NAME}
PRIVATE
        ${AVFORMAT_LIBRARIES} ${AVCODEC_LIBRARIES} ${AVFILTER_LIBRARIES} ${AVUTIL_LIBRARIES}
)

if(${HAVE_GAPI})
    target_link_libraries(${TARGET_NAME} PRIVATE gapi_pre_proc)
endif()

if(NOT ${DISABLE_VAAPI})
    target_link_libraries(${TARGET_NAME} PRIVATE vaapi_pre_proc)
endif()

set (install_headers ${CMAKE_CURRENT_SOURCE_DIR}/include/ie_wrapper.h
                     ${CMAKE_CURRENT_SOURCE_DIR}/include/ie_meta.h
                     )

install(FILES ${install_headers} DESTINATION include/ie_wrapper)
install(TARGETS "${TARGET_NAME}" DESTINATION lib)
