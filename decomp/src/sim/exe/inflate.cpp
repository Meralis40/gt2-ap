// Resident code 80083b0c-80085858 (inflate). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern ushort g_InflateLenBase[];
extern ushort g_InflateLenExtra[];
extern ushort g_InflateDistBase[];
extern ushort g_InflateDistExtra[];
extern byte g_InflateCodeLenOrder[];
extern jmpbuf g_GzipCallerCtx;
extern int g_GzipChunkSize;
extern jmpbuf g_GzipInflateCtx;
extern GzipStream *g_GzipStream;

void GzipStream_Main(void)
{
    Inflate_Run(&g_GzipStream->state);
    g_GzipStream = 0;
    longjmp(&g_GzipCallerCtx, 2);
}

InflateState *GzipStream_Yield(InflateState *state)
{
    int iVar1;

    iVar1 = setjmp(&g_GzipInflateCtx);
    if (iVar1 == 0) {
        longjmp(&g_GzipCallerCtx, 1);
    }
    return state;
}

InflateState *GzipStream_Flush(InflateState *state)
{
    int iVar1;

    iVar1 = setjmp(&g_GzipInflateCtx);
    if (iVar1 == 0) {
        longjmp(&g_GzipCallerCtx, 1);
    }
    state->outEnd = state->out + g_GzipChunkSize;
    return state;
}

// Starts inflating src into dst as a coroutine (setjmp and longjmp, on its own stack, which starts
// at stream + 0x3ffc, past the GzipStream fields). It stops after each chunkSize bytes of output (0: never) and at each block
// header and table build. Returns 0 when it stopped, 1 when it's done; GzipStream_Continue
// resumes it. One stream at a time (g_GzipStream).
#ifdef NON_MATCHING
// Matches but for one instruction's place: the copy of stream into $t0 (stream itself lives in
// its argument slot, because of setjmp) comes three instructions earlier here than in the game,
// which has it after GzipStream_Yield's address. A local state pointer or an inline init
// function don't move it, nor did the permuter.
int GzipStream_Begin(void *src, void *dst, int chunkSize, GzipStream *stream)
{
    InflateState *(*flush)(InflateState *);
    byte *heap;

    g_GzipStream = stream;
    g_GzipChunkSize = chunkSize;
    stream->state.inBase = (byte *)src;
    stream->state.in = (ushort *)src;
    stream->state.yield = GzipStream_Yield;
    stream->state.refill = 0;
    stream->state.inEnd = (ushort *)((byte *)src - 1);
    flush = GzipStream_Flush;
    heap = stream->heap;
    if (flush == 0 || chunkSize == 0) {
        chunkSize = -1;
    }
    stream->state.flush = flush;
    stream->state.outEnd = (byte *)dst + chunkSize;
    stream->state.outBase = (byte *)dst;
    stream->state.out = (byte *)dst;
    stream->state.heapBase = heap;
    stream->state.heapEnd = heap + sizeof(stream->heap);
    switch (setjmp(&g_GzipCallerCtx)) {
    case 0:
        // Runs the inflate on its own stack at the end of the stream buffer.
        __asm__ volatile(".set\tnoreorder\n\tmove\t$3,$sp\n\tjal\tGzipStream_Main\n\tmove\t$sp,%0\n\t.set\treorder"
                         : : "r"((byte *)stream + 0x3ffc) : "$3", "memory");
    case 1:
        return 0;
    }
    return 1;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/inflate", GzipStream_Begin);
#endif

// Resumes the stream's inflate. Returns 0 when it stopped again, 1 when it's done (or there's
// no stream).
int GzipStream_Continue(void)
{
    int r;

    if (g_GzipStream != 0) {
        switch (setjmp(&g_GzipCallerCtx)) {
        case 0:
            longjmp(&g_GzipInflateCtx, 1);
            break;
        case 1:
            return 0;
        }
    }
    return 1;
}

// LZSS: one flag byte for each 8 items, bit 0 first. A 0 bit is a literal byte; a 1 bit is a copy
// of (a byte + 3) bytes from (a byte, or two bytes when its bit 7 is set, + 1) bytes back. Stops
// after size bytes and returns dst. Used by the movie decoders.
unsigned char *Lz_Decompress(unsigned char *dst, unsigned char *src, int size)
{
    unsigned char *out = dst;
    int flags = 0;
    int n = 0;
    unsigned int v;
    int dist;
    int hi;
    int len;
    unsigned char *p;

    while (size > 0) {
        flags >>= 1;
        if (n == 0) {
            flags = *src++;
        }
        // The item's bytes come from one unaligned load that the game splits in two: lwr here,
        // and lwl (the next two bytes) only for a copy. GCC emits lwl and lwr together, so both
        // are asm. Giving v a new value here (=r) gets it the game's register.
        __asm__("lwr\t%0,0(%1)" : "=r"(v) : "r"(src));
        src++;
        if (!(flags & 1)) {
            *out++ = v;
            size--;
        } else {
            __asm__("lwl\t%0,2(%1)" : "+r"(v) : "r"(src));
            src++;
            len = v & 0xff;
            dist = v >> 8;
            hi = dist & 0x80;
            dist &= 0x7f;
            if (hi) {
                dist <<= 8;
                dist |= v >> 16 & 0xff;
                src++;
            }
            len += 3;
            size -= len;
            dist++;
            if (size < 0) {
                len += size;
            }
            p = out - dist;
            if (dist >= 4 && len >= 4) {
                // A word at a time while the copy doesn't overlap the bytes it reads. The game
                // has lwr then lwl (and swr then swl); GCC's unaligned word copy (a packed
                // struct) is lwl then lwr.
                do {
                    __asm__("lwr\t%0,0(%1)" : "=r"(v) : "r"(p));
                    __asm__("lwl\t%0,3(%1)" : "+r"(v) : "r"(p));
                    len -= 4;
                    __asm__("swr\t%0,0(%1)" : : "r"(v), "r"(out) : "memory");
                    __asm__("swl\t%0,3(%1)" : : "r"(v), "r"(out) : "memory");
                    p += 4;
                    out += 4;
                } while (len >= 4);
            }
            while (len != 0) {
                len--;
                *out++ = *p++;
            }
        }
        n = (n + 1) & 7;
    }
    return dst;
}

// Builds the decoding tables for one Huffman code (gzip's huft_build): b holds the n code lengths,
// symbols below s are literals and the others take their base and extra bits from d and e. The
// tables come from state's heap, linked through t; *m is the first table's bits (lookupBits at
// most). Returns non-zero for an incomplete code.
#ifdef NON_MATCHING
// The game's code but for the registers and spills (3 instructions fewer): the game keeps state
// in its argument slot and has fp for &c[k]; this keeps state in fp. 20 minutes of permuter
// didn't get close. The do-while entered at its test is the game's loop shape; a plain while
// loop tests at the top.
int Inflate_BuildHuft(InflateState *state, uint *b, uint n, uint s, ushort *d, ushort *e, Huft **t,
                      uint *m, uint lookupBits)
{
    uint a;
    uint c[17];
    uint f;
    int g;
    int h;
    uint i;
    uint j;
    uint k;
    int l;
    uint *p;
    Huft *q;
    Huft r;
    Huft *u[16];
    uint v[288];
    int w;
    uint x[17];
    uint *xp;
    int ii;
    int jj;
    int y;
    uint z;

    *m = lookupBits;
    p = c - 1;
    do {
        *++p = 0;
    } while (p != c + 16);
    p = b;
    do {
        c[*p]++;
        p++;
    } while (p != b + n);
    if (c[0] == n) {
        *t = 0;
        *m = 0;
        return 0;
    }

    l = *m;
    for (jj = 1; jj <= 16; jj++) {
        if (c[jj]) {
            break;
        }
    }
    k = jj;
    if (l < jj) {
        l = jj;
    }
    for (ii = 16; ii; ii--) {
        if (c[ii]) {
            break;
        }
    }
    g = ii;
    if (l > ii) {
        l = ii;
    }
    *m = l;

    y = 1 << jj;
    for (p = c + jj; p != c + ii; p++) {
        y -= *p;
        y <<= 1;
    }
    y -= *p;
    *p += y;

    x[0] = 0;
    x[1] = j = 0;
    p = c + 1;
    xp = x + 2;
    while (p != c + g) {
        *xp++ = (j += *p++);
    }

    p = b;
    i = 0;
    while (p != b + n) {
        if ((j = *p++) != 0) {
            v[x[j]++] = i;
        }
        i++;
    }

    x[0] = i = 0;
    p = v;
    h = -1;
    w = -l;
    u[0] = 0;
    q = 0;
    z = 0;

    for (; k <= g; k++) {
        a = c[k];
        while (a--) {
            while (k > w + l) {
                h++;
                w += l;

                z = (z = g - w) > (uint)l ? l : z;
                if ((f = 1 << (j = k - w)) > a + 1) {
                    f -= a + 1;
                    xp = c + k;
                    goto next;
                    do {
                        if ((f <<= 1) > *++xp) {
                            f -= *xp;
                        } else {
                            break;
                        }
                    next:;
                    } while (++j < z);
                }
                z = 1 << j;

                q = (Huft *)state->heapPtr;
                state->heapPtr = q + z + 1;
                *t = q + 1;
                *(t = &q->next) = 0;
                u[h] = ++q;

                if (h) {
                    x[h] = i;
                    r.bits = l;
                    r.op = 16 + j;
                    r.base = 0;
                    r.next = q;
                    j = i >> (w - l);
                    u[h - 1][j] = r;
                }
            }

            r.bits = k - w;
            if (p >= v + n) {
                r.op = 99;
            } else if (*p < s) {
                r.op = *p < 256 ? 16 : 15;
                r.base = *p;
                p++;
            } else {
                r.op = e[*p - s];
                r.base = d[*p++ - s];
            }

            f = 1 << (k - w);
            for (j = i >> w; j < z; j += f) {
                q[j] = r;
            }

            for (j = 1 << (k - 1); i & j; j >>= 1) {
                i ^= j;
            }
            i ^= j;

            while ((i & ((1 << w) - 1)) != x[h]) {
                h--;
                w -= l;
            }
        }
    }

    return y != 0 && g != 1;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/inflate", Inflate_BuildHuft);
#endif

// Inflate_Run's and Inflate_Codes's locals go to the state around each callback, and come back
// from the state it returns.
#define INFLATE_SAVE()                                                                             \
    state->in = in;                                                                                \
    state->inEnd = inEnd;                                                                          \
    state->bitBuf = bits;                                                                          \
    state->bitCount = count;                                                                       \
    state->out = out;                                                                              \
    state->outEnd = outEnd;                                                                        \
    state->saved = saved
#define INFLATE_LOAD()                                                                             \
    in = state->in;                                                                                \
    inEnd = state->inEnd;                                                                          \
    bits = state->bitBuf;                                                                          \
    count = state->bitCount;                                                                       \
    out = state->out;                                                                              \
    outEnd = state->outEnd;                                                                        \
    saved = state->saved
#define INFLATE_CALL(fn)                                                                           \
    {                                                                                              \
        INFLATE_SAVE();                                                                            \
        state = state->fn(state);                                                                  \
        INFLATE_LOAD();                                                                            \
    }
// Drops n bits, then tops the bit buffer up with the next 16 bits when it runs dry.
#define INFLATE_DUMP(n)                                                                            \
    count -= n;                                                                                    \
    bits >>= n;                                                                                    \
    if (count < 0) {                                                                               \
        count += 16;                                                                               \
        if (in == inEnd) INFLATE_CALL(refill)                                                      \
        bits |= *in++ << count;                                                                    \
    }
// Takes the next n bits into saved.
#define INFLATE_GET(n)                                                                             \
    saved = bits & ((1 << (n)) - 1);                                                               \
    INFLATE_DUMP(n)
#define INFLATE_YIELD()                                                                            \
    if (state->yield != 0) INFLATE_CALL(yield)

#ifdef NON_MATCHING
// Handwritten: the original keeps its locals in $t registers across the refill and flush calls,
// and loads its constants (1, 15, 16, 17) back into $t8, $s3, $s2 and $t9 after each one.
// Like Inflate_Run's, its callbacks get the locals through the state, and it takes everything
// back (the tables and masks too) from the state they return.
#define CODES_CALL(fn)                                                                             \
    {                                                                                              \
        INFLATE_SAVE();                                                                            \
        state = state->fn(state);                                                                  \
        INFLATE_LOAD();                                                                            \
        tl = state->litTable;                                                                      \
        td = state->distTable;                                                                     \
        ml = state->litBits;                                                                       \
        md = state->distBits;                                                                      \
    }
#define CODES_DUMP(n)                                                                              \
    count -= n;                                                                                    \
    bits >>= n;                                                                                    \
    if (count < 0) {                                                                               \
        count += 16;                                                                               \
        if (in == inEnd) CODES_CALL(refill)                                                        \
        bits |= *in++ << count;                                                                    \
    }

// Decodes one block's literals and length/distance pairs with the state's tables, up to the
// end-of-block code. Turns litBits and distBits into masks in place.
void Inflate_Codes(InflateState *state)
{
    byte *out = state->out;
    byte *outEnd = state->outEnd;
    ushort *in = state->in;
    ushort *inEnd = state->inEnd;
    uint bits = state->bitBuf;
    int count = state->bitCount;
    Huft *tl = state->litTable;
    Huft *td = state->distTable;
    uint ml;
    uint md;
    uint saved;
    Huft *t;
    uint e;
    uint n;
    byte *p;
    byte *end;
    byte c;

    ml = state->litBits = (1 << state->litBits) - 1;
    md = state->distBits = (1 << state->distBits) - 1;
    for (;;) {
        t = tl + (bits & ml);
        for (;;) {
            e = t->op;
            CODES_DUMP(t->bits);
            if (e <= 16) {
                break;
            }
            t = t->next + (bits & ((1 << (e - 16)) - 1));
        }
        if (e == 16) {
            if (out == outEnd) CODES_CALL(flush)
            *out++ = t->base;
            continue;
        }
        if (e == 15) {
            state->out = out;
            state->outEnd = outEnd;
            state->in = in;
            state->inEnd = inEnd;
            state->bitBuf = bits;
            state->bitCount = count;
            return;
        }

        // A length (e extra bits), then its distance.
        n = t->base + (bits & ((1 << e) - 1));
        CODES_DUMP(e);
        t = td + (bits & md);
        for (;;) {
            e = t->op;
            CODES_DUMP(t->bits);
            if (e <= 16) {
                break;
            }
            t = t->next + (bits & ((1 << (e - 16)) - 1));
        }
        saved = bits & ((1 << e) - 1);
        p = out - (t->base + saved);
        CODES_DUMP(e);

        end = p + n;
        do {
            c = *p++;
            if (out == outEnd) CODES_CALL(flush)
            *out++ = c;
        } while (p != end);
    }
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/inflate", Inflate_Codes);
#endif

// Decodes a gzip stream: skips the header (and the file name when FLG has FNAME; the other
// optional fields aren't handled), then inflates blocks until the last one. The trailer isn't read.
#ifdef NON_MATCHING
// Same length and instructions as the game's but for the registers. The game keeps outEnd in fp
// and spills flags and the code length pointer (ll + i); here the pointer gets fp, and outEnd a
// caller-saved register, saved around the Inflate_BuildHuft calls. The declaration order
// changes nothing. The do-while loops behind an if are the game's loop shape; for and while
// loops test at the top.
void Inflate_Run(InflateState *state)
{
    uint ll[316];
    uint fixedLit[288];
    uint fixedDist[30];
    byte *out;
    byte *outEnd;
    ushort *in;
    ushort *inEnd;
    uint bits;
    int count;
    uint saved;
    uint flags;
    int i;
    int last;
    uint type;
    int nl;
    int nd;
    int nb;
    int n;
    uint mask;
    Huft *tl;
    Huft *td;
    int j;
    uint l;
    uint *p;

    out = state->out;
    outEnd = state->outEnd;
    in = state->in;
    inEnd = state->inEnd;
    bits = 0;
    count = -16;
    if ((uint)in & 1) {
        bits = *(byte *)in;
        count = -8;
        in = (ushort *)((byte *)in + 1);
    }
    INFLATE_DUMP(0);

    INFLATE_GET(8);
    INFLATE_GET(8);
    INFLATE_GET(8);
    INFLATE_GET(8);
    flags = saved;
    i = 6;
    while (i--) {
        INFLATE_GET(8);
    }
    if (flags & 8) {
        do {
            INFLATE_GET(8);
        } while (saved != 0);
    }

    do {
        INFLATE_GET(1);
        last = saved;
        INFLATE_GET(2);
        type = saved;
        INFLATE_YIELD();
        if (type == 2) {
            INFLATE_GET(5);
            nl = saved + 257;
            INFLATE_GET(5);
            nd = saved + 1;
            INFLATE_GET(4);
            nb = saved + 4;
            state->heapPtr = state->heapBase;
            i = 0;
            if (i < nb) {
                do {
                    p = &ll[g_InflateCodeLenOrder[i]];
                    INFLATE_GET(3);
                    *p = saved;
                } while (++i < nb);
            }
            for (; i < 19; i++) {
                ll[g_InflateCodeLenOrder[i]] = 0;
            }
            Inflate_BuildHuft(state, ll, 19, 19, 0, 0, &state->litTable, &state->litBits, 7);
            INFLATE_YIELD();

            n = nl + nd;
            mask = (1 << state->litBits) - 1;
            tl = state->litTable;
            i = l = 0;
            if (i < n) do {
                td = tl + (bits & mask);
                INFLATE_DUMP(td->bits);
                j = td->base;
                if (j < 16) {
                    ll[i++] = l = j;
                } else if (j == 16) {
                    INFLATE_GET(2);
                    j = saved + 3;
                    while (j--) {
                        ll[i++] = l;
                    }
                } else if (j == 17) {
                    INFLATE_GET(3);
                    j = saved + 3;
                    while (j--) {
                        ll[i++] = 0;
                    }
                    l = 0;
                } else {
                    INFLATE_GET(7);
                    j = saved + 11;
                    while (j--) {
                        ll[i++] = 0;
                    }
                    l = 0;
                }
            } while (i < n);
            INFLATE_YIELD();

            state->heapPtr = state->heapBase;
            Inflate_BuildHuft(state, ll, nl, 257, g_InflateLenBase, g_InflateLenExtra, &state->litTable,
                              &state->litBits, 9);
            INFLATE_YIELD();
            Inflate_BuildHuft(state, ll + nl, nd, 0, g_InflateDistBase, g_InflateDistExtra,
                              &state->distTable, &state->distBits, 6);
            INFLATE_YIELD();
        } else if (type == 0) {
            n = count & 7;
            INFLATE_DUMP(n);
            INFLATE_GET(16);
            n = saved;
            INFLATE_GET(16);
            while (n--) {
                if (out == outEnd) INFLATE_CALL(flush)
                INFLATE_GET(8);
                *out++ = saved;
            }
            continue;
        } else if (type == 1) {
            state->heapPtr = state->heapBase;
            for (i = 0; i < 144; i++) {
                fixedLit[i] = 8;
            }
            for (; i < 256; i++) {
                fixedLit[i] = 9;
            }
            for (; i < 280; i++) {
                fixedLit[i] = 7;
            }
            for (; i < 288; i++) {
                fixedLit[i] = 8;
            }
            for (i = 0; i < 30; i++) {
                fixedDist[i] = 5;
            }
            Inflate_BuildHuft(state, fixedLit, 288, 257, g_InflateLenBase, g_InflateLenExtra,
                              &state->litTable, &state->litBits, 7);
            Inflate_BuildHuft(state, fixedDist, 30, 0, g_InflateDistBase, g_InflateDistExtra,
                              &state->distTable, &state->distBits, 5);
            INFLATE_YIELD();
        } else {
            continue;
        }
        state->out = out;
        state->outEnd = outEnd;
        state->in = in;
        state->inEnd = inEnd;
        state->bitBuf = bits;
        state->bitCount = count;
        Inflate_Codes(state);
        out = state->out;
        outEnd = state->outEnd;
        in = state->in;
        inEnd = state->inEnd;
        bits = state->bitBuf;
        count = state->bitCount;
    } while (!last);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/inflate", Inflate_Run);
#endif
}
