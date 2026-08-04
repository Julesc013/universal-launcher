# SPDX-FileCopyrightText: 2026 Jules C
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.21)
project(universal_launcher VERSION 1.8.0 LANGUAGES C)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(ULK_BUILD_APPS "Build Universal Launcher app frontends" "${PROJECT_IS_TOP_LEVEL}")
option(ULK_BUILD_TESTS "Build Universal Launcher native tests" "${PROJECT_IS_TOP_LEVEL}")
option(ULK_BUILD_STATIC "Build the Universal Launcher static library" ON)
option(ULK_BUILD_SHARED "Build the Universal Launcher shared library" ON)

if(NOT ULK_BUILD_STATIC AND NOT ULK_BUILD_SHARED)
  message(FATAL_ERROR "At least one of ULK_BUILD_STATIC or ULK_BUILD_SHARED must be enabled")
endif()
if(ULK_BUILD_TESTS AND (NOT ULK_BUILD_STATIC OR NOT ULK_BUILD_SHARED))
  message(FATAL_ERROR "ULK_BUILD_TESTS requires both static and shared libraries")
endif()

function(ulk_enable_strict_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /WX)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
endfunction()

add_library(ulk_headers INTERFACE)
set_target_properties(ulk_headers PROPERTIES EXPORT_NAME Headers)
target_include_directories(
  ulk_headers
  INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
add_library(UniversalLauncher::Headers ALIAS ulk_headers)

set(ULK_KERNEL_SOURCES
  runtime/client/ulk_client.c
  runtime/client/ulk_operation.c
  runtime/client/ulk_owned_response.c
  runtime/launcher/kernel/ulk_api.c
  runtime/launcher/kernel/ulk_product_composition.c
  runtime/launcher/kernel/ulk_reference_model.c
  runtime/launcher/kernel/ulk_setup_handoff.c
)

set(ULK_INSTALL_TARGETS ulk_headers)

if(ULK_BUILD_STATIC)
  add_library(ulk_static STATIC ${ULK_KERNEL_SOURCES})
  set_target_properties(ulk_static PROPERTIES EXPORT_NAME CoreStatic)
  target_link_libraries(ulk_static PUBLIC ulk_headers)
  ulk_enable_strict_warnings(ulk_static)
  add_library(UniversalLauncher::CoreStatic ALIAS ulk_static)
  add_library(ulk ALIAS ulk_static)
  list(APPEND ULK_INSTALL_TARGETS ulk_static)
endif()

if(ULK_BUILD_SHARED)
  add_library(ulk_shared SHARED ${ULK_KERNEL_SOURCES})
  set_target_properties(ulk_shared PROPERTIES EXPORT_NAME CoreShared OUTPUT_NAME ulk)
  target_link_libraries(ulk_shared PUBLIC ulk_headers)
  target_compile_definitions(ulk_shared PRIVATE ULK_BUILD_SHARED INTERFACE ULK_USE_SHARED)
  ulk_enable_strict_warnings(ulk_shared)
  add_library(UniversalLauncher::CoreShared ALIAS ulk_shared)
  list(APPEND ULK_INSTALL_TARGETS ulk_shared)
endif()

set(ULK_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/UniversalLauncher")
set(ULK_INSTALL_CONTRACTS_DIR "${CMAKE_INSTALL_DATADIR}/universal-launcher/contracts/schema")
set(ULK_INSTALL_ABI_MANIFEST "${CMAKE_INSTALL_DATADIR}/universal-launcher/contracts/abi/ulk_c_abi.v1.toml")

configure_package_config_file(
  cmake/UniversalLauncherConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/cmake-package/UniversalLauncherConfig.cmake"
  INSTALL_DESTINATION "${ULK_INSTALL_CMAKEDIR}"
  PATH_VARS ULK_INSTALL_CONTRACTS_DIR ULK_INSTALL_ABI_MANIFEST
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/cmake-package/UniversalLauncherConfigVersion.cmake"
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY ExactVersion
)

install(
  TARGETS ${ULK_INSTALL_TARGETS}
  EXPORT UniversalLauncherTargets
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
install(EXPORT UniversalLauncherTargets
  FILE UniversalLauncherTargets.cmake
  NAMESPACE UniversalLauncher::
  DESTINATION "${ULK_INSTALL_CMAKEDIR}"
)
install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/cmake-package/UniversalLauncherConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake-package/UniversalLauncherConfigVersion.cmake"
  DESTINATION "${ULK_INSTALL_CMAKEDIR}"
)
install(DIRECTORY include/ulk include/ulu DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(
  DIRECTORY contracts/schema/
  DESTINATION "${ULK_INSTALL_CONTRACTS_DIR}"
  FILES_MATCHING PATTERN "*.json"
)
install(
  FILES contracts/abi/ulk_c_abi.v1.toml
  DESTINATION "${CMAKE_INSTALL_DATADIR}/universal-launcher/contracts/abi"
)
install(
  FILES LICENSE release/license.v1.toml
  DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses/universal-launcher"
)
install(
  FILES cmake/README-SDK.md
  DESTINATION "${CMAKE_INSTALL_DATADIR}/universal-launcher"
)

if(ULK_BUILD_TESTS)
  enable_testing()
  find_package(Python3 COMPONENTS Interpreter REQUIRED)

  function(ulk_add_native_test name source target)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE ${target})
    ulk_enable_strict_warnings(${name})
    add_test(NAME ${name} COMMAND ${name})
  endfunction()

  ulk_add_native_test(ulk_command_graph_smoke tests/native/ulk_command_graph_smoke.c ulk_static)
  ulk_add_native_test(ulk_setup_handoff_smoke tests/native/ulk_setup_handoff_smoke.c ulk_static)
  ulk_add_native_test(ulk_client_smoke tests/native/ulk_client_smoke.c ulk_static)
  ulk_add_native_test(ulk_operation_outcome_smoke tests/native/ulk_operation_outcome_smoke.c ulk_static)
  add_executable(ulk_owned_response_smoke tests/native/ulk_owned_response_smoke.c)
  target_link_libraries(ulk_owned_response_smoke PRIVATE ulk_static)
  ulk_enable_strict_warnings(ulk_owned_response_smoke)
  add_test(NAME ulk_owned_response_smoke COMMAND ulk_owned_response_smoke)

  add_executable(ulk_owned_response_export_smoke tests/native/ulk_owned_response_export_smoke.c)
  target_link_libraries(ulk_owned_response_export_smoke PRIVATE ulk_shared)
  ulk_enable_strict_warnings(ulk_owned_response_export_smoke)
  add_test(NAME ulk_owned_response_export_smoke COMMAND ulk_owned_response_export_smoke)
  ulk_add_native_test(ulk_abi_layout_smoke tests/native/ulk_abi_layout_smoke.c ulk_headers)
  add_executable(ulk_abi_1_6_client_compat_smoke tests/native/ulk_abi_1_6_client_compat_smoke.c)
  target_link_libraries(ulk_abi_1_6_client_compat_smoke PRIVATE ulk_shared)
  ulk_enable_strict_warnings(ulk_abi_1_6_client_compat_smoke)
  add_test(NAME ulk_abi_1_6_client_compat_smoke COMMAND ulk_abi_1_6_client_compat_smoke)
  target_include_directories(
    ulk_abi_1_6_client_compat_smoke
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests/compat/ulk_abi_1_6"
  )
  ulk_add_native_test(ulk_reference_model_smoke tests/native/ulk_reference_model_smoke.c ulk_static)
  ulk_add_native_test(ulk_product_composition_smoke tests/native/ulk_product_composition_smoke.c ulk_static)

  foreach(ULK_SDK_PHASE IN ITEMS source static shared combined)
    set(ULK_SDK_CONFORMANCE_COMMAND
      "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/tools/cmake_sdk_conformance.py"
      --work-dir "${CMAKE_CURRENT_BINARY_DIR}/sdk-conformance/${ULK_SDK_PHASE}"
      --config "$<CONFIG>"
      --phase "${ULK_SDK_PHASE}"
    )
    if(CMAKE_GENERATOR_PLATFORM)
      list(APPEND ULK_SDK_CONFORMANCE_COMMAND --platform "${CMAKE_GENERATOR_PLATFORM}")
    endif()
    add_test(
      NAME "ulk_cmake_sdk_${ULK_SDK_PHASE}"
      COMMAND ${ULK_SDK_CONFORMANCE_COMMAND}
    )
    set_tests_properties("ulk_cmake_sdk_${ULK_SDK_PHASE}" PROPERTIES TIMEOUT 900)
  endforeach()
endif()
