#!/bin/sh
# Builds the spike for 32-bit Windows with LLVM and the VS x86 libraries.
set -e
CLANG="/c/Program Files/LLVM/bin/clang++.exe"
VC='C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/MSVC/14.51.36231'
SDK='C:/Program Files (x86)/Windows Kits/10'
SDKV=10.0.26100.0
mkdir -p build
MSYS2_ARG_CONV_EXCL='*' "$CLANG" --target=i686-pc-windows-msvc -std=c++20 -O2 -fuse-ld=lld -fno-pic \
    -nostdlibinc \
    -idirafter "$VC/include" -idirafter "$SDK/Include/$SDKV/ucrt" \
    -idirafter "$SDK/Include/$SDKV/um" -idirafter "$SDK/Include/$SDKV/shared" \
    spike.cpp guest_syms.S -o build/spike.exe \
    "-Wl,/LIBPATH:$VC/lib/x86" "-Wl,/LIBPATH:$SDK/Lib/$SDKV/ucrt/x86" "-Wl,/LIBPATH:$SDK/Lib/$SDKV/um/x86" \
    -Wl,/BASE:0x400000,/FIXED,/LARGEADDRESSAWARE,/DYNAMICBASE:NO,/NXCOMPAT,/SAFESEH:NO
