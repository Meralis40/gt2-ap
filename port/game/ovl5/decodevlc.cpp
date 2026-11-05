// Huffman (VLC) decoding of a movie frame into MDEC run-level codes. The
// leading-zero counter of the GTE (LZCS/LZCR) finds the code length.
#include "common.h"

// Starts a leading-zero count of v, and reads the result.
#define gte_lzcs(v) __asm__ volatile("mtc2 %0, $30" : : "r"(v))
#define gte_lzcr(r) __asm__ volatile("mfc2 %0, $31" : "=r"(r))

// Tops up the bit buffer with the next 16 bits when it runs low.
#define VLC_REFILL                                \
    if (avail < 1) {                              \
        avail += 16;                              \
        bits |= *src++ << (sixteen - avail);      \
    }

extern "C" {
// frame: two header words, the width and height, the size in bytes of the
// LZ-compressed first codes of the blocks, then those and the bit stream. out
// gets the two header words and the MDEC codes, padded with end-of-block codes
// to out[0] * 2 + 2 halfwords.
void Movie_DecodeVlc(unsigned short *frame, unsigned short *out)
{
    int blocks;
    int count;
    unsigned char *q;
    unsigned char *buf;
    unsigned char *dc;
    unsigned char *lz;
    unsigned short *src;
    unsigned short *o;
    unsigned int bits;
    int avail;
    int zeros;
    int row;
    int code;
    unsigned int len;
    unsigned short *end;
    // Constants kept in registers through the loop.
    int done;
    int eob;
    int six;
    int five;
    int topBit;
    int sixteen;

    out[0] = frame[0];
    out[1] = frame[1];
    blocks = ((frame[2] + 15) >> 4) * ((frame[3] + 15) >> 4);
    src = frame + 5;
    lz = (unsigned char *)src;
    src = (unsigned short *)((unsigned char *)src + frame[4]);
    count = blocks * 6;
    buf = Lz_Decompress((unsigned char *)__builtin_alloca(blocks * 12), lz, blocks * 12);
    bits = (src[0] << 16) | src[1];
    src += 2;
    avail = 16;
    q = buf;
    dc = buf + count;
    o = out + 2;
    done = -1;
    eob = 0xfe00;
    sixteen = 16;
    six = 6;
    five = 5;
    topBit = 23;
    while (--count != done) {
        // First code of the block (quantizer and DC).
        *o++ = (*q++ << 8) | *dc++;
        for (;;) {
            gte_lzcs(bits);
            if ((int)bits < 0) {
                // Codes starting with 1: end of block (10), or the two
                // 3-bit codes 110 and 111.
                code = bits >> 29;
                len = 3;
                if (code < 6) {
                    *o++ = eob;
                    avail -= 2;
                    bits <<= 2;
                    VLC_REFILL
                    break;
                }
                *o = code == six ? 1 : 0x3ff;
                o++;
            } else {
                gte_lzcr(zeros);
                row = zeros << 8;
                if ((bits >> 23) == 0) {
                    // 9 or more leading zeros: skip 9 bits, the table row
                    // stays the one of the full count.
                    avail -= 9;
                    bits <<= 9;
                    VLC_REFILL
                    zeros -= 9;
                } else if (zeros == five) {
                    // Escape: the next 16 bits are the code.
                    avail -= 6;
                    bits <<= 6;
                    VLC_REFILL
                    *o++ = bits >> 16;
                    len = 16;
                    goto put;
                }
                {
                    unsigned int e;

                    // Table entry: code length in the top 6 bits, MDEC code
                    // in the low 16.
                    row += (bits >> (topBit - zeros)) & 0xfc;
                    e = *(unsigned int *)((unsigned char *)D_800114CC + row);
                    o++;
                    len = e >> 26;
                    o[-1] = e;
                }
            }
put:
            avail -= len;
            bits <<= len;
            VLC_REFILL
        }
    }
    end = out + (out[0] * 2 + 2);
    while (o < end) {
        *o++ = 0xfe00;
    }
}
}
