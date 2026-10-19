# SPDX-License-Identifier: GPL-2.0-or-later
cmake_minimum_required(VERSION 3.24)

project(gt2ap
    VERSION 0.0.1
    DESCRIPTION "Gran Turismo 2 client for Archipelago"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(GT2AP_BUILD_TESTS "Build unit tests" ${PROJECT_IS_TOP_LEVEL})
option(GT2AP_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

include(cmake/CompilerWarnings.cmake)

add_subdirectory(src/discio)
add_subdirectory(src/client)
add_subdirectory(tools/discinfo)

if(GT2AP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
