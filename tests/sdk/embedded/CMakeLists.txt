# SPDX-FileCopyrightText: 2026 Jules C
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.16)
project(ulk_sdk_embedded_defaults LANGUAGES C)

if(NOT DEFINED ULK_SOURCE_DIR)
  message(FATAL_ERROR "ULK_SOURCE_DIR is required")
endif()
add_subdirectory("${ULK_SOURCE_DIR}" ulk-provider)

if(ULK_BUILD_APPS OR ULK_BUILD_TESTS)
  message(FATAL_ERROR "embedded consumers must not build ULK apps or tests by default")
endif()
if(TARGET ulk_command_graph_smoke)
  message(FATAL_ERROR "embedded default unexpectedly created provider tests")
endif()
if(NOT TARGET UniversalLauncher::Headers OR
   NOT TARGET UniversalLauncher::CoreStatic OR
   NOT TARGET UniversalLauncher::CoreShared)
  message(FATAL_ERROR "embedded provider targets are incomplete")
endif()
