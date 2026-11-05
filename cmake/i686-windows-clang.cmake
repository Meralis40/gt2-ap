# SPDX-License-Identifier: GPL-2.0-or-later
# 32-bit Windows with LLVM's Clang and the Visual Studio x86 libraries.
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86)

set(LLVM_ROOT "C:/Program Files/LLVM" CACHE PATH "LLVM install")
set(CMAKE_C_COMPILER "${LLVM_ROOT}/bin/clang.exe")
set(CMAKE_CXX_COMPILER "${LLVM_ROOT}/bin/clang++.exe")
set(CMAKE_ASM_COMPILER "${LLVM_ROOT}/bin/clang.exe")
set(CMAKE_LINKER "${LLVM_ROOT}/bin/lld-link.exe")
set(CMAKE_RC_COMPILER "${LLVM_ROOT}/bin/llvm-rc.exe")
set(CMAKE_C_COMPILER_TARGET i686-pc-windows-msvc)
set(CMAKE_CXX_COMPILER_TARGET i686-pc-windows-msvc)
set(CMAKE_ASM_COMPILER_TARGET i686-pc-windows-msvc)
set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld")
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(VS_VC "C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/MSVC/14.51.36231" CACHE PATH "MSVC tools")
set(WIN_SDK "C:/Program Files (x86)/Windows Kits/10" CACHE PATH "Windows SDK")
set(WIN_SDK_VERSION 10.0.26100.0 CACHE STRING "Windows SDK version")

# -idirafter: Clang's own stdarg and vadefs must win over MSVC's.
foreach(dir "${VS_VC}/include" "${WIN_SDK}/Include/${WIN_SDK_VERSION}/ucrt"
            "${WIN_SDK}/Include/${WIN_SDK_VERSION}/um" "${WIN_SDK}/Include/${WIN_SDK_VERSION}/shared")
    string(APPEND CMAKE_C_FLAGS_INIT " -idirafter \"${dir}\"")
    string(APPEND CMAKE_CXX_FLAGS_INIT " -idirafter \"${dir}\"")
endforeach()
foreach(dir "${VS_VC}/lib/x86" "${WIN_SDK}/Lib/${WIN_SDK_VERSION}/ucrt/x86" "${WIN_SDK}/Lib/${WIN_SDK_VERSION}/um/x86")
    string(APPEND CMAKE_EXE_LINKER_FLAGS_INIT " -Xlinker /LIBPATH:\"${dir}\"")
endforeach()
