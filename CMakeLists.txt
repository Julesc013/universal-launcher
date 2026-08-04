# SPDX-FileCopyrightText: 2026 Jules C
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.16)
project(universal_launcher C)

option(ULK_BUILD_APPS "Build Universal Launcher app frontends" ON)
option(ULK_BUILD_TESTS "Build Universal Launcher native tests" ON)

add_library(ulk_headers INTERFACE)
target_include_directories(ulk_headers INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

set(ULK_KERNEL_SOURCES
  runtime/client/ulk_client.c
  runtime/client/ulk_operation.c
  runtime/client/ulk_owned_response.c
  runtime/launcher/kernel/ulk_api.c
  runtime/launcher/kernel/ulk_product_composition.c
  runtime/launcher/kernel/ulk_reference_model.c
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
  add_executable(ulk_client_smoke tests/native/ulk_client_smoke.c)
  target_link_libraries(ulk_client_smoke PRIVATE ulk_static)
  add_test(NAME ulk_client_smoke COMMAND ulk_client_smoke)
  add_executable(ulk_operation_outcome_smoke tests/native/ulk_operation_outcome_smoke.c)
  target_link_libraries(ulk_operation_outcome_smoke PRIVATE ulk_static)
  add_test(NAME ulk_operation_outcome_smoke COMMAND ulk_operation_outcome_smoke)
  add_executable(ulk_owned_response_smoke tests/native/ulk_owned_response_smoke.c)
  target_link_libraries(ulk_owned_response_smoke PRIVATE ulk_static)
  add_test(NAME ulk_owned_response_smoke COMMAND ulk_owned_response_smoke)
  add_executable(ulk_owned_response_export_smoke tests/native/ulk_owned_response_export_smoke.c)
  target_compile_definitions(ulk_owned_response_export_smoke PRIVATE ULK_USE_SHARED)
  target_link_libraries(ulk_owned_response_export_smoke PRIVATE ulk_shared)
  add_test(NAME ulk_owned_response_export_smoke COMMAND ulk_owned_response_export_smoke)
  add_executable(ulk_abi_layout_smoke tests/native/ulk_abi_layout_smoke.c)
  target_link_libraries(ulk_abi_layout_smoke PRIVATE ulk_headers)
  add_test(NAME ulk_abi_layout_smoke COMMAND ulk_abi_layout_smoke)
  add_executable(ulk_abi_1_6_client_compat_smoke tests/native/ulk_abi_1_6_client_compat_smoke.c)
  target_compile_definitions(ulk_abi_1_6_client_compat_smoke PRIVATE ULK_USE_SHARED)
  target_include_directories(
    ulk_abi_1_6_client_compat_smoke
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests/compat/ulk_abi_1_6"
  )
  target_link_libraries(ulk_abi_1_6_client_compat_smoke PRIVATE ulk_shared)
  add_test(NAME ulk_abi_1_6_client_compat_smoke COMMAND ulk_abi_1_6_client_compat_smoke)
  add_executable(ulk_reference_model_smoke tests/native/ulk_reference_model_smoke.c)
  target_link_libraries(ulk_reference_model_smoke PRIVATE ulk_static)
  add_test(NAME ulk_reference_model_smoke COMMAND ulk_reference_model_smoke)
  add_executable(ulk_product_composition_smoke tests/native/ulk_product_composition_smoke.c)
  target_link_libraries(ulk_product_composition_smoke PRIVATE ulk_static)
  add_test(NAME ulk_product_composition_smoke COMMAND ulk_product_composition_smoke)
endif()
