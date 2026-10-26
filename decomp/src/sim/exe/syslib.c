// The game's own C library: string and memory functions, sprintf and a small
// heap. The functions still in assembly are included from splat's output.
// Jump table labels are global (macro.inc), as the tables are in another file.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "syslib.h"

void Sys_Init(void)
{
    Heap_InitDefault();
    Boot_InitHardware();
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/syslib", memset);

char *strcpy(char *dst, const char *src)
{
    char *d = dst;
    while ((*d++ = *src++) != 0) {
    }
    return dst;
}

// Returns -1, 0 or 1. The sign is computed before the test, which puts it in
// the branch's delay slot.
int strcmp(const char *s1, const char *s2)
{
    char c1, c2;
    int d, r;
    for (;;) {
        c1 = *s1++;
        c2 = *s2++;
        d = c1 - c2;
        r = d >> 31;
        if (d != 0) {
            return r | 1;
        }
        if (c1 == 0) {
            return 0;
        }
    }
}

int sprintf(char *buf, const char *fmt, ...)
{
    return vsprintf(buf, fmt, (char *)&fmt + 4);
}

char *strncpy(char *dst, const char *src, size_t n)
{
    size_t len = strlen(src) + 1;
    if (n < len) {
        len = n;
    }
    bcopy(src, dst, len);
    return dst;
}

size_t strlen(const char *s)
{
    size_t n = -1;
    int c;
    do {
        c = *s++;
        n++;
    } while (c != 0);
    return n;
}

void *memcpy(void *dst, const void *src, size_t n)
{
    unsigned char *d;
    const unsigned char *s = (const unsigned char *)src;
    if (src == dst) {
        return dst;
    }
    d = (unsigned char *)dst;
    while (n--) {
        *d++ = *s++;
    }
    return dst;
}

char *strcat(char *dst, const char *src)
{
    strcpy(dst + strlen(dst), src);
    return dst;
}

// Compares up to n characters, ignoring ASCII case.
int strncasecmp(const char *a, const char *b, int n)
{
    int ca, cb;
    while (n != 0) {
        ca = *a++;
        cb = *b++;
        if ((unsigned int)(ca - 'a') < 26) {
            ca = (char)(ca - 0x20);
        }
        if ((unsigned int)(cb - 'a') < 26) {
            cb = (char)(cb - 0x20);
        }
        if (ca != cb) {
            return ca - cb;
        }
        n--;
        if (ca == 0) {
            break;
        }
    }
    return 0;
}

// Best fit: the smallest free block that holds size rounded up to 16 bytes.
void *malloc(size_t size)
{
    HeapBlock *b;
    HeapBlock *best = 0;
    size = (size + 15) & ~15;
    for (b = g_HeapBase; b != 0; b = b->next) {
        if (b->used == 0 && size <= b->size && (best == 0 || b->size < best->size)) {
            best = b;
        }
    }
    if (best == 0) {
        return 0;
    }
    Heap_SplitBlock(best, size);
    best->used = 1;
    return best + 1;
}

int abs(int x)
{
    return (x >= 0) ? x : -x;
}

int memcmp(const void *a, const void *b, int n)
{
    const unsigned char *p = (const unsigned char *)a;
    const unsigned char *q = (const unsigned char *)b;
    unsigned int ca, cb;
    while (n != 0) {
        ca = *p++;
        cb = *q++;
        if (ca < cb) {
            return -1;
        }
        n--;
        if (cb < ca) {
            return 1;
        }
    }
    return 0;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/syslib", Printf_EmitString);

INCLUDE_ASM("asm/sim/exe/nonmatchings/syslib", Printf_EmitNumber);

INCLUDE_ASM("asm/sim/exe/nonmatchings/syslib", Printf_FormatDec64);

// 64-bit octal. buf[0] keeps the sign, and the sign variable then becomes the
// flag that leading zeros have been skipped.
void Printf_FormatOct64(PutcFunc putc, unsigned int flags, int width, int prec, unsigned long long val, int sign)
{
    char buf[64];
    int len, shift, d;
    buf[0] = sign;
    len = 1;
    sign = 0;
    for (shift = 63; shift >= 0; shift -= 3) {
        d = (val >> shift) & 7;
        if (sign || d != 0) {
            sign = 1;
            buf[len++] = d + '0';
        }
    }
    Printf_EmitNumber(putc, flags, width, prec, buf[0], buf + 1, len - 1, 8);
}

// 64-bit hex; conv is the conversion character, 'x' or 'X'.
void Printf_FormatHex64(PutcFunc putc, unsigned int flags, int width, int prec, unsigned long long val, int conv)
{
    char buf[64];
    char *digits;
    int len = 0, started, shift;
    if (conv == 'x') {
        digits = D_80090F74;
    } else {
        digits = D_80090F88;
    }
    started = 0;
    for (shift = 60; shift >= 0; shift -= 4) {
        int d = (val >> shift) & 15;
        if (started || d != 0) {
            started = 1;
            buf[len++] = digits[d];
        }
    }
    // base 17 makes Printf_EmitNumber write an upper-case 0X
    Printf_EmitNumber(putc, flags, width, prec, 0, buf, len, conv == 'X' ? 17 : 16);
}

// Reads a decimal number at *p and moves *p past it.
int Printf_ParseInt(char **p)
{
    char *s = *p;
    int n = 0;
    char c = *s;
    while ((unsigned int)(c - '0') < 10) {
        n = n * 10 + (c - '0');
        c = *++s;
    }
    *p = s;
    return n;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/syslib", Printf_Format);

// The whole region becomes one free block.
void InitHeap(unsigned long *base, unsigned long size)
{
    g_HeapBase = (HeapBlock *)base;
    g_HeapSize = size;
    if (size < sizeof(HeapBlock)) {
        size = 0;
    } else {
        size -= sizeof(HeapBlock);
    }
    g_HeapBase->prev = 0;
    g_HeapBase->next = 0;
    g_HeapBase->size = size;
    g_HeapBase->used = 0;
}

// The heap runs from the end of the BSS to g_StackSize below the stack that
// start was called with, both aligned to 16 bytes, or is empty.
void Heap_InitDefault(void)
{
    int base, top, size;
    base = (int)(D_801F0D60 + 15) & ~15;
    base |= 0x80000000;
    top = g_StartupSP & ~15;
    top = (top - g_StackSize + 15) & ~15;
    top |= 0x80000000;
    size = 0;
    if (base < top) {
        size = top - base;
    }
    InitHeap((unsigned long *)base, size);
}

// Frees the block, then merges it with the free blocks around it: back to the
// first of the free blocks before it, then forward.
void free(void *ptr)
{
    HeapBlock *b = (HeapBlock *)ptr - 1;
    HeapBlock *p;
    b->used = 0;
    for (p = b->prev; p != 0 && p->used == 0; p = p->prev) {
        b = p;
    }
    for (p = b->next; p != 0 && p->used == 0; p = b->next) {
        Heap_MergeNext(b);
    }
}

// vsprintf's sink: appends c to the output buffer.
int Printf_PutcToBuffer(int c)
{
    *D_801C9898++ = c;
    return 0;
}

int vsprintf(char *buf, const char *fmt, void *args)
{
    int n;
    D_801C9898 = buf;
    n = Printf_Format(Printf_PutcToBuffer, fmt, args);
    buf[n] = 0;
    return n;
}

// Cuts block to size bytes when the rest holds a header and 16 bytes or more;
// the rest becomes a free block after it.
void Heap_SplitBlock(HeapBlock *block, unsigned int size)
{
    HeapBlock *nb, *next;
    if (block->size >= size + 2 * sizeof(HeapBlock)) {
        nb = (HeapBlock *)((char *)block + size) + 1;
        *nb = *block;
        next = block->next;
        block->size = size;
        block->next = nb;
        nb->prev = block;
        nb->used = 0;
        nb->size -= sizeof(HeapBlock);
        nb->size -= size;
        if (next != 0) {
            next->prev = nb;
        }
    }
}

// Absorbs the next block, header and payload, into block.
void Heap_MergeNext(HeapBlock *block)
{
    HeapBlock *next = block->next;
    HeapBlock *nn;
    if (next != 0) {
        nn = next->next;
        if (nn != 0) {
            nn->prev = block;
        }
        block->next = nn;
        block->size += next->size + sizeof(HeapBlock);
    }
}
