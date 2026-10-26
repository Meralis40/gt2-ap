// The game's own C library (syslib, 0x8008ce08-0x8008dfc4): string and memory
// functions, sprintf and a small heap. See docs/runtime-libs.md.
#ifndef SYSLIB_H
#define SYSLIB_H

typedef unsigned int size_t;

// Header of a heap block; the payload follows it.
typedef struct HeapBlock {
    struct HeapBlock *prev;
    struct HeapBlock *next;
    unsigned int size; // payload bytes
    unsigned int used;
} HeapBlock;

typedef int (*PutcFunc)(int c);

void Boot_InitHardware(void);
void Heap_InitDefault(void);
void Heap_SplitBlock(HeapBlock *block, unsigned int size);
void Heap_MergeNext(HeapBlock *block);
void bcopy(const void *src, void *dst, size_t n);
size_t strlen(const char *s);
char *strcpy(char *dst, const char *src);
int vsprintf(char *buf, const char *fmt, void *args);
int Printf_Format(PutcFunc putc, const char *fmt, void *args);

extern HeapBlock *g_HeapBase;
extern char *D_801C9898; // vsprintf's output pointer

#endif
