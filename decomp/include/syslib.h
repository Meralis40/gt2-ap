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
int Printf_Format(PutcFunc putc, const char *fmt, char *args);
int Printf_ParseInt(char **p);
int Printf_FormatDec64(PutcFunc putc, unsigned int flags, int width, int prec, unsigned long long val, int sign,
                       int point);
int Printf_FormatOct64(PutcFunc putc, unsigned int flags, int width, int prec, unsigned long long val, int sign);
int Printf_FormatHex64(PutcFunc putc, unsigned int flags, int width, int prec, unsigned long long val, int conv);
int Printf_EmitString(PutcFunc putc, unsigned int flags, int width, int prec, char *s);
int Printf_EmitNumber(PutcFunc putc, unsigned int flags, int width, int prec, char sign, char *digits, int len,
                      int base);
int Printf_PutcToBuffer(int c);
void InitHeap(unsigned long *base, unsigned long size);

extern HeapBlock *g_HeapBase;
extern unsigned long g_HeapSize;
extern unsigned int g_StackSize;
extern unsigned int g_StartupSP; // sp when start was called
extern char D_801F0D60[];        // end of the BSS
extern char *D_801C9898; // vsprintf's output pointer
extern unsigned long long D_800A8CB8[19]; // 10^18 down to 10^0

#endif
