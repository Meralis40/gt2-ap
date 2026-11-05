#!/bin/sh
# Builds the spike for 32-bit Linux. Non-PIE: guest symbols are absolute.
set -e
mkdir -p build
g++ -m32 -std=c++20 -O2 -fno-pic -no-pie spike.cpp guest_syms.S -o build/spike-linux
