# SPDX-License-Identifier: GPL-2.0-or-later
# 64-bit Windows with LLVM's Clang and the Visual Studio x64 libraries.
set(GT2AP_WIN_PROCESSOR AMD64)
set(GT2AP_WIN_TARGET x86_64-pc-windows-msvc)
set(GT2AP_WIN_LIBARCH x64)
include("${CMAKE_CURRENT_LIST_DIR}/windows-clang.inc.cmake")
