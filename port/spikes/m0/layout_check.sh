#!/bin/sh
# Prints the probe's layout[] from GCC 2.8.1 (WSL Ubuntu-24.04), one value a line.
set -e
cd /mnt/e/sources/gt2-ap/decomp
cpp -nostdinc -undef -D__GNUC__=2 -D__GNUC_MINOR__=8 -Dmips -D__mips__ -DMIPSEL -Iinclude \
    ../port/spikes/m0/layout_probe.cpp \
    | tools/toolchain/gcc-2.8.1-psx/cc1plus -quiet -O2 -g0 -G0 -fvtable-thunks -fno-exceptions -fno-builtin -o /tmp/gt2_layout.s && cat /tmp/gt2_layout.s
