cmake_minimum_required(VERSION 3.16)
project(universal_launcher C)

option(ULK_BUILD_APPS "Build Universal Launcher app frontends" ON)

add_library(ulk_headers INTERFACE)
target_include_directories(ulk_headers INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_library(ulk_static STATIC runtime/launcher/kernel/ulk_api.c)
target_include_directories(ulk_static PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_library(ulk_shared SHARED runtime/launcher/kernel/ulk_api.c)
target_include_directories(ulk_shared PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
set_target_properties(ulk_shared PROPERTIES OUTPUT_NAME ulk WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_library(ulk ALIAS ulk_static)
