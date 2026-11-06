# SPDX-License-Identifier: GPL-2.0-or-later
# A 32-bit build for Linux, from a 64-bit host with the i386 C and C++ libraries (Ubuntu:
# gcc-multilib and g++-multilib). The port needs it: docs/port.md.
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR i686)

if(NOT DEFINED CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER clang)
endif()
if(NOT DEFINED CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER clang++)
endif()
set(CMAKE_C_FLAGS_INIT -m32)
set(CMAKE_CXX_FLAGS_INIT -m32)
set(CMAKE_ASM_FLAGS_INIT -m32)
set(CMAKE_EXE_LINKER_FLAGS_INIT -m32)
