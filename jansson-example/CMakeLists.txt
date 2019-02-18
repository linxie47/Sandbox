cmake_minimum_required(VERSION 3.10)

project (jansson-example)

set (TARGET_NAME "meta_json")

find_package(PkgConfig REQUIRED)

pkg_check_modules(JANSSON REQUIRED jansson)

add_executable(${TARGET_NAME} meta_json.c)

target_link_libraries(${TARGET_NAME} PRIVATE jansson)
