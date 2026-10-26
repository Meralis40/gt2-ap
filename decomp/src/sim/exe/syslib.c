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
    signed char c;
    do {
        c = *src++;
        *d++ = c;
    } while (c != 0);
    return dst;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/syslib", strcmp);

int sprintf(char *buf, const char *fmt, ...)
{
    return vsprintf(buf, fmt, (char *)&fmt + 4);
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/syslib", strncpy);

size_t strlen(const char *s)
{
    size_t n = -1;
    int c;
    do {
        c = *(signed char *)s++;
        n++;
    } while (c != 0);
    return n;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/syslib", memcpy);

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
        ca = *(signed char *)a++;
        cb = *(signed char *)b++;
        if ((unsigned int)(ca - 'a') < 26) {
            ca = (signed char)(ca - 0x20);
        }
        if ((unsigned int)(cb - 'a') < 26) {
            cb = (signed char)(cb - 0x20);
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

INCLUDE_ASM("asm/sim/exe/nonmatchings/syslib", Printf_FormatOct64);

INCLUDE_ASM("asm/sim/exe/nonmatchings/syslib", Printf_FormatHex64);

INCLUDE_ASM("asm/sim/exe/nonmatchings/syslib", Printf_ParseInt);

INCLUDE_ASM("asm/sim/exe/nonmatchings/syslib", Printf_Format);

INCLUDE_ASM("asm/sim/exe/nonmatchings/syslib", InitHeap);

INCLUDE_ASM("asm/sim/exe/nonmatchings/syslib", Heap_InitDefault);

INCLUDE_ASM("asm/sim/exe/nonmatchings/syslib", free);

// vsprintf's sink: appends c to the output buffer.
int Printf_PutcToBuffer(int c)
{
    *D_801C9898++ = c;
    return 0;
}

INCLUDE_ASM("asm/sim/exe/nonmatchings/syslib", vsprintf);

INCLUDE_ASM("asm/sim/exe/nonmatchings/syslib", Heap_SplitBlock);

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
