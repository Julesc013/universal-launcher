# SPDX-FileCopyrightText: 2026 Jules C
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.16)
project(universal_launcher C)

option(ULK_BUILD_APPS "Build Universal Launcher app frontends" ON)
option(ULK_BUILD_TESTS "Build Universal Launcher native tests" ON)

add_library(ulk_headers INTERFACE)
target_include_directories(ulk_headers INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

set(ULK_KERNEL_SOURCES
  runtime/launcher/kernel/ulk_api.c
  runtime/launcher/kernel/ulk_setup_handoff.c
)

add_library(ulk_static STATIC ${ULK_KERNEL_SOURCES})
target_include_directories(ulk_static PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_library(ulk_shared SHARED ${ULK_KERNEL_SOURCES})
target_include_directories(ulk_shared PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_definitions(ulk_shared PRIVATE ULK_BUILD_SHARED)
set_target_properties(ulk_shared PROPERTIES OUTPUT_NAME ulk WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_library(ulk ALIAS ulk_static)

if(ULK_BUILD_TESTS)
  enable_testing()
  add_executable(ulk_command_graph_smoke tests/native/ulk_command_graph_smoke.c)
  target_link_libraries(ulk_command_graph_smoke PRIVATE ulk_static)
  add_test(NAME ulk_command_graph_smoke COMMAND ulk_command_graph_smoke)
  add_executable(ulk_setup_handoff_smoke tests/native/ulk_setup_handoff_smoke.c)
  target_link_libraries(ulk_setup_handoff_smoke PRIVATE ulk_static)
  add_test(NAME ulk_setup_handoff_smoke COMMAND ulk_setup_handoff_smoke)
endif()
