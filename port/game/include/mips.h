// What the game's MIPS inline assembly did, in C.
//
// Mips_Lwl2 and Mips_Swl2 are `lwl rt, 2(p)` and `swl rt, 2(p)` on a word-aligned
// little-endian address: they move the low three bytes of *p to and from the top
// three bytes of a register. The game links GPU packets with them, because a tag's
// low three bytes are the 24-bit link and its top byte is the packet length.
#ifndef MIPS_H
#define MIPS_H

static inline unsigned int Mips_Lwl2(const void *p)
{
    return *(const unsigned int *)p << 8;
}

static inline void Mips_Swl2(unsigned int value, void *p)
{
    unsigned int *word = (unsigned int *)p;

    *word = (*word & 0xff000000) | (value >> 8);
}

// A word at any address (lwr and lwl together).
static inline unsigned int Mips_ReadWord(const void *p)
{
    const unsigned char *b = (const unsigned char *)p;

    return b[0] | b[1] << 8 | b[2] << 16 | (unsigned int)b[3] << 24;
}

// A word to any address (swr and swl together).
static inline void Mips_WriteWord(void *p, unsigned int value)
{
    unsigned char *b = (unsigned char *)p;

    b[0] = value;
    b[1] = value >> 8;
    b[2] = value >> 16;
    b[3] = value >> 24;
}

#endif
