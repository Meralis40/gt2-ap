// The game's own C library: string and memory functions, sprintf and a small
// heap. The functions still in assembly are included from splat's output.
#include "syslib.h"

void Sys_Init(void)
{
    Heap_InitDefault();
    Boot_InitHardware();
}

// Fills by words, with swr and swl for an unaligned head and tail. When dst is
// unaligned and n >= 4, it returns dst rounded up to a word, not dst.
// The loops count down to the -1 kept in end (at the top), not to 0.
void *memset(void *dst, int c, size_t n)
{
    unsigned char *p;
    unsigned int *w;
    unsigned int a, x, rem;
    unsigned int end = -1;
    if (n == 0) {
        return dst;
    }
    c &= 0xff;
    if (n < 4) {
        p = dst;
        if (--n != end) {
            do {
                *p++ = c;
            } while (--n != -1);
        }
    } else {
        a = (unsigned int)dst & 3;
        x = c | (c << 8);
        c = x | (x << 16);
        if (a != 0) {
            __asm__ volatile("swr %0, 0(%1)" : : "r"(c), "r"(dst));
            a = 4 - a;
            dst = (char *)dst + a;
            n -= a;
        }
        w = dst;
        a = n >> 2;
        rem = n & 3;
        if (--a != end) {
            do {
                *w++ = c;
            } while (--a != -1);
        }
        if (rem != 0) {
            p = (unsigned char *)w + rem;
            __asm__ volatile("swl %0, -1(%1)" : : "r"(c), "r"(p));
        }
    }
    return dst;
}

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

// %s: at most prec characters of s (all of them when prec < 0), padded to
// width. Flag 1 pads on the right ('-'), flag 4 pads a right-aligned string
// with '0'. Returns the characters written, or -1 when putc fails.
int Printf_EmitString(PutcFunc putc, unsigned int flags, int width, int prec, char *s)
{
    int len, right = 0, left = 0, count = 0, i, fill;
    len = strlen(s);
    if (prec >= 0 && prec < len) {
        len = prec;
    }
    width -= len;
    if (width < 0) {
        width = 0;
    }
    if ((flags & 1) == 0) {
        left = width;
    } else {
        right = width;
    }
    fill = ' ';
    if (flags & 4) {
        fill = '0';
    }
    for (i = 0; i < left; i++) {
        if (putc(fill) < 0) {
            return -1;
        }
        count++;
    }
    for (i = 0; i < len; i++, count++) {
        if (putc(s[i]) < 0) {
            return -1;
        }
    }
    for (i = 0; i < right; i++) {
        if (putc(' ') < 0) {
            return -1;
        }
        count++;
    }
    return count;
}

// Writes a converted number: padding, the prefix (sign, then 0 or 0x/0X with
// flag 0x10), zeros up to prec digits, the len digits, then padding. Flags:
// 1 pads on the right, 2 adds '+', 4 pads with zeros. base 17 is hex with 0X.
// Returns the characters written, or -1 when putc fails.
int Printf_EmitNumber(PutcFunc putc, unsigned int flags, int width, int prec, char sign, char *digits, int len, int base)
{
    char pfx[64];
    int right = 0, npfx = 0, left = 0, count, zeros, i;
    if (width < 0) {
        width = 0;
    }
    if (prec < 0) {
        prec = 1;
    }
    if (base < 11) {
        if (sign != '-' && (flags & 2)) {
            sign = '+';
        }
        if (sign) {
            pfx[0] = sign;
            npfx = 1;
        }
    }
    if (flags & 0x10) {
        if (base >= 8) {
            pfx[npfx++] = '0';
        }
        if (base >= 16) {
            pfx[npfx++] = (base & 1) ? 'X' : 'x';
        }
    }
    zeros = prec - len;
    if (zeros < 0) {
        zeros = 0;
    }
    if ((len | zeros) == 0) {
        left = width;
        npfx = 0;
    } else {
        i = width - (npfx + len) - zeros;
        if (i < 0) {
            zeros += i;
            i = 0;
            if (zeros <= 0) {
                zeros = len == 0;
            }
        } else if ((flags & 1) == 0) {
            if (flags & 4) {
                zeros += i;
            } else {
                left = i;
            }
        } else {
            right = i;
        }
    }
    count = 0;
    for (i = 0; i < left; i++) {
        if (putc(' ') < 0) {
            return -1;
        }
        count++;
    }
    for (i = 0; i < npfx; i++, count++) {
        if (putc(pfx[i]) < 0) {
            return -1;
        }
    }
    for (i = 0; i < zeros; i++) {
        if (putc('0') < 0) {
            return -1;
        }
        count++;
    }
    for (i = 0; i < len; i++, count++) {
        if (putc(digits[i]) < 0) {
            return -1;
        }
    }
    for (i = 0; i < right; i++) {
        if (putc(' ') < 0) {
            return -1;
        }
        count++;
    }
    return count;
}

// 64-bit decimal, by repeated subtraction of the powers of ten. With point
// set and prec >= 0, a '.' goes before the last prec digits; point then holds
// the index of the digit before it (19 for none).
int Printf_FormatDec64(PutcFunc putc, unsigned int flags, int width, int prec, unsigned long long val, int sign,
                        int point)
{
    char buf[64];
    int started, len, i, d, c;
    unsigned long long p;
    if (point == 0 || prec < 0 || (point = 18 - prec) < 0) {
        point = 19;
    }
    started = 0;
    len = 0;
    for (i = 0; i < 19; i++) {
        p = D_800A8CB8[i];
        d = 0;
        while (val >= p) {
            d++;
            val -= p;
        }
        c = d + '0';
        if (started || d != 0 || i == point) {
            if (i == point) {
                buf[len++] = c;
                c = '.';
            }
            buf[len++] = c;
            started = 1;
        }
    }
    return Printf_EmitNumber(putc, flags, width, prec, sign, buf, len, 10);
}

// 64-bit octal. buf[0] keeps the sign, and the sign variable then becomes the
// flag that leading zeros have been skipped.
int Printf_FormatOct64(PutcFunc putc, unsigned int flags, int width, int prec, unsigned long long val, int sign)
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
    return Printf_EmitNumber(putc, flags, width, prec, buf[0], buf + 1, len - 1, 8);
}

// 64-bit hex; conv is the conversion character, 'x' or 'X'.
int Printf_FormatHex64(PutcFunc putc, unsigned int flags, int width, int prec, unsigned long long val, int conv)
{
    char buf[64];
    char *digits;
    int len = 0, started, shift;
    if (conv == 'x') {
        digits = "0123456789abcdef";
    } else {
        digits = "0123456789ABCDEF";
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
    return Printf_EmitNumber(putc, flags, width, prec, 0, buf, len, conv == 'X' ? 17 : 16);
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

// The printf engine: sends the output of fmt to putc and returns how many
// characters it wrote, or -1 when putc fails. Flags are - + 0 space #, width
// and precision can be * (but .* sets the width), sizes are h, l (int) and L
// (64 bits). %i prints in octal and %t in decimal with a point before the last
// prec digits; e, f, g, n and p only skip their argument.
int Printf_Format(PutcFunc putc, const char *fmt, char *args)
{
    int count = 0;
    char c;
    unsigned int flags;
    int width, prec, size, esc, r, sign;
    const char *start;
    long long sval;
    unsigned long long uval;

    while ((c = *fmt++) != 0) {
        start = fmt;
        esc = c == '%';
        if (*fmt != '%') {
            esc = 0;
        }
        if (c != '%' || esc) {
            fmt = start + esc;
            if (putc(c) != 0) {
                return -1;
            }
            count++;
            continue;
        }
        flags = 0;
        for (;; fmt++) {
            c = *fmt;
            switch (*fmt) {
            case '-':
                flags |= 1;
                continue;
            case '+':
                flags |= 2;
                continue;
            case '0':
                flags |= 4;
                continue;
            case ' ':
                flags |= 8;
                continue;
            case '#':
                flags |= 0x10;
                continue;
            }
            break;
        }
        width = -1;
        if ((unsigned int)(c - '0') < 10) {
            width = Printf_ParseInt((char **)&fmt);
            c = *fmt;
        } else if (c == '*') {
            width = *(int *)args;
            args += 4;
            c = *++fmt;
        }
        prec = -1;
        if (c == '.') {
            c = *++fmt;
            if ((unsigned int)(c - '0') < 10) {
                prec = Printf_ParseInt((char **)&fmt);
            } else if (c == '*') {
                width = *(int *)args;
                args += 4;
                fmt++;
            }
        }
        switch (*fmt++) {
        case 'h':
            size = 1;
            break;
        case 'l':
            size = 2;
            break;
        case 'L':
            size = 3;
            break;
        default:
            size = 0;
            fmt--;
            break;
        }
        c = *fmt;
        switch (*fmt++) {
        case 'X':
        case 'd':
        case 'i':
        case 'o':
        case 't':
        case 'u':
        case 'x':
            switch (size) {
            case 1:
                sval = (short)*(unsigned short *)args;
                args += 4;
                uval = sval & 0xffff;
                break;
            case 3:
                sval = *(long long *)args;
                args += 8;
                uval = sval;
                break;
            default:
                sval = *(int *)args;
                args += 4;
                uval = sval & 0xffffffff;
                break;
            }
            sign = 0;
            if (sval < 0 && (c == 'd' || c == 'i' || c == 't')) {
                uval = -sval;
                sign = '-';
            }
            switch (c) {
            case 'd':
            case 'u':
                r = Printf_FormatDec64(putc, flags, width, prec, uval, sign, 0);
                break;
            case 't':
                r = Printf_FormatDec64(putc, flags, width, prec, uval, sign, 1);
                break;
            case 'i':
            case 'o':
                r = Printf_FormatOct64(putc, flags, width, prec, uval, sign);
                break;
            case 'X':
            case 'x':
                r = Printf_FormatHex64(putc, flags, width, prec, uval, c);
                break;
            default:
                r = 0;
                break;
            }
            if (r < 0) {
                return -1;
            }
            count += r;
            break;
        case 'E':
        case 'G':
        case 'e':
        case 'f':
        case 'g':
            if (size < 2) {
                args += sizeof(float) ;
            } else if (size < 3) {
                args += sizeof(double);
            } else {
                args += sizeof(long double);
            }
            break;
        case 'n':
        case 'p':
            args += 4;
            break;
        case 'c':
            if (putc(*args) < 0) {
                return -1;
            }
            args += 4;
            count++;
            break;
        case 's':
            r = Printf_EmitString(putc, flags, width, prec, *(char **)args);
            args += 4;
            if (r < 0) {
                return -1;
            }
            count += r;
            break;
        default:
            // unknown conversion: the % prints nothing, and the rest is copied
            fmt = start;
            break;
        }
    }
    return count;
}

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
