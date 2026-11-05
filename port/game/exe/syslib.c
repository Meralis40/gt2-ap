// The game's own C library: string and memory functions, sprintf and a small
// heap. The functions still in assembly are included from splat's output.
#include "syslib.h"

void Sys_Init(void)
{
    Boot_InitHardware();
}




int sprintf(char *buf, const char *fmt, ...)
{
    return vsprintf(buf, fmt, (char *)&fmt + 4);
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


