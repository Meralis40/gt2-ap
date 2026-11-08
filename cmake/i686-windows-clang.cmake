# SPDX-License-Identifier: GPL-2.0-or-later
# 32-bit Windows with LLVM's Clang and the Visual Studio x86 libraries.
set(GT2AP_WIN_PROCESSOR x86)
set(GT2AP_WIN_TARGET i686-pc-windows-msvc)
set(GT2AP_WIN_LIBARCH x86)
include("${CMAKE_CURRENT_LIST_DIR}/windows-clang.inc.cmake")
