#!/bin/sh
# Regenerates port/gen/guest_syms.S after the game's external names change. Run it from
# the repository root, in Git Bash, once the game's objects are built (a failed link is
# fine); it needs LLVM's nm and, in WSL Ubuntu-24.04, Python.
set -e
NM="/c/Program Files/LLVM/bin/llvm-nm.exe"
OBJECTS=build/port-win/port/CMakeFiles/game.dir/game
WORK=port/.scan

find "$OBJECTS" -name '*.obj' > "$WORK/objects.txt"
xargs -d '\n' "$NM" --defined-only -f just-symbols < "$WORK/objects.txt" 2>/dev/null | sort -u > "$WORK/defs.txt"
xargs -d '\n' "$NM" -u -f just-symbols < "$WORK/objects.txt" 2>/dev/null | sort -u > "$WORK/u.txt"
comm -23 "$WORK/u.txt" "$WORK/defs.txt" | grep -vE '^$|^\?' > "$WORK/undefs.txt"
wsl -d Ubuntu-24.04 -e bash -c 'cd /mnt/e/sources/gt2-ap/port && python3 tools/gen_syms.py .scan/undefs.txt --out gen/guest_syms.S --report .scan/report.txt'
