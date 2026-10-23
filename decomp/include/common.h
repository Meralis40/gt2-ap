// Declarations shared by the decompiled sources.
#ifndef COMMON_H
#define COMMON_H

// Functions are extern "C" while their callers are still assembly, so the
// symbols keep the names the disassembly uses.
extern "C" {
void *memset(void *dst, int c, unsigned int n);
}

#endif
