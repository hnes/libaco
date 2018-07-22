# Copyright 2018 Sen Han 00hnes@gmail.com
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(libaco VERSION 1.2.2 LANGUAGES C ASM)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

##
# Options
##
option(ACO_CHECK        "Enable libaco testing"       ON)
option(ACO_BENCH        "Enable libaco testing"       ON)
option(ACO_SHARE_FPENV  "Enable share FPU MXCSR env"  OFF)
option(ACO_VALGRIND     "Enable the use of valgrind"  OFF)
option(ACO_M32          "Enable m32 compilation"      OFF)
option(ACO_ASAN         "Enable address sanitizer"    OFF)
option(ACO_COVERAGE     "Enable code coverage"        OFF)

##
# Project directories definition
##
set(__INC_DIR     ${CMAKE_CURRENT_LIST_DIR}/include)
set(__SRC_DIR     ${CMAKE_CURRENT_LIST_DIR}/src)
set(__TEST_DIR    ${CMAKE_CURRENT_LIST_DIR}/test)
set(__BENCH_DIR   ${CMAKE_CURRENT_LIST_DIR}/bench)
set(__INC_INSDIR  ${CMAKE_INSTALL_INCLUDEDIR})
set(__LIB_INSDIR  ${CMAKE_INSTALL_LIBDIR})
set(__CONF_INSDIR ${__LIB_INSDIR}/cmake/${PROJECT_NAME})

##
# Target
##
set(__TARGET_NAME aco)
set(__TARGET lib${__TARGET_NAME})

file(GLOB_RECURSE __HDRS ${__HDRS} ${__INC_DIR}/*.h)

add_library(${__TARGET} ${__HDRS}
  ${__SRC_DIR}/aco.c
  ${__SRC_DIR}/acosw.S)
add_library(${PROJECT_NAME}::${__TARGET_NAME} ALIAS ${__TARGET})
set_target_properties(${__TARGET} PROPERTIES
  C_STANDARD 90 OUTPUT_NAME "${__TARGET_NAME}")

target_include_directories(${__TARGET}
  PRIVATE ${__SRC_DIR}
  PUBLIC
    $<INSTALL_INTERFACE:${__INC_INSDIR}>
    $<BUILD_INTERFACE:${__INC_DIR}>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>)

install(FILES ${__HDRS} DESTINATION ${__INC_INSDIR})

function (aco_target_configure target)
  if (MSVC)
    target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(${target} PRIVATE /Oy /W3)
  else ()
    target_compile_definitions(${target}
      PRIVATE
        $<$<BOOL:${ACO_SHARE_FPENV}>:ACO_CONFIG_SHARE_FPU_MXCSR_ENV>
        $<$<BOOL:${ACO_VALGRIND}>:ACO_USE_VALGRIND>)
    target_compile_options(${target}
      PRIVATE
        -Wall -Werror -Wextra
        -fomit-frame-pointer
        -Wno-missing-field-initializers
        -Wno-type-limits #TODO: fix the code
        $<$<CONFIG:Debug>:-g3 -O0>
        $<$<BOOL:${ACO_M32}>:-m32>
        $<$<BOOL:${ACO_COVERAGE}>:--coverage>
        $<$<BOOL:${ACO_ASAN}>:-fsanitize=address>)
    target_link_libraries(${target}
      PRIVATE
        ${ARGN}
        $<$<BOOL:${ACO_M32}>:-m32>
        $<$<BOOL:${ACO_COVERAGE}>:--coverage>
        $<$<BOOL:${ACO_ASAN}>:-fsanitize=address>
        $<$<BOOL:${ACO_ASAN}>:-fno-omit-frame-pointer>)
  endif ()
endfunction ()

aco_target_configure(${__TARGET})

##
# Testing
##
if (ACO_CHECK AND EXISTS ${__TEST_DIR})
  if (NOT TARGET check)
    enable_testing()
    add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --timeout 2)
  endif ()
  add_subdirectory(${__TEST_DIR})
endif ()

##
# Benchmarking
##
if (ACO_BENCH AND EXISTS ${__BENCH_DIR})
  add_subdirectory(${__BENCH_DIR})
endif ()

##
# Installation
##
install(TARGETS ${__TARGET} EXPORT ${PROJECT_NAME}Targets
  LIBRARY DESTINATION ${__LIB_INSDIR}
  ARCHIVE DESTINATION ${__LIB_INSDIR})
install(EXPORT ${PROJECT_NAME}Targets FILE ${PROJECT_NAME}Targets.cmake
  NAMESPACE ${PROJECT_NAME}::
  DESTINATION ${__CONF_INSDIR})
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
  VERSION ${PROJECT_VERSION} COMPATIBILITY AnyNewerVersion)
configure_package_config_file(
  ${CMAKE_CURRENT_LIST_DIR}/cmake/Config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION ${__CONF_INSDIR})
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
  DESTINATION ${__CONF_INSDIR})
export(EXPORT ${PROJECT_NAME}Targets
  FILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake
  NAMESPACE ${PROJECT_NAME}::)
export(PACKAGE ${PROJECT_NAME})
