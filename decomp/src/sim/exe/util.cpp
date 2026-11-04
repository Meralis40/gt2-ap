// Resident code 80082fac-800831bc (util): one-shot gzip inflate, the string hash, the
// lists of vsync callbacks and CRC-32.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {

extern unsigned int D_800A6ACC[256]; // the CRC-32 table

// Inflates a whole gzip file from src to dst in one go. The state and a 12 KB Huft heap
// live on the stack; no input or output limits, no callbacks.
void Gzip_Inflate(void *src, void *dst)
{
    unsigned char heap[0x3000];
    InflateState state;

    state.inBase = (byte *)src;
    state.in = (ushort *)src;
    state.outBase = (byte *)dst;
    state.out = (byte *)dst;
    state.inEnd = (ushort *)((char *)src - 1);
    state.yield = 0;
    state.refill = 0;
    state.flush = 0;
    state.outEnd = (byte *)dst - 1;
    state.heapBase = heap;
    state.heapEnd = &state;
    Inflate_Run(&state);
}

// h = rotl(h, 6) + c over the string.
#ifdef NON_MATCHING
// The game leaves the increment before the branch, with the shift in the delay slot; GCC
// puts the increment in the slot, and one word longer with the nop after the load.
unsigned int Str_Hash(char *s)
{
    unsigned int h = 0;

    while (1) {
        int c = *s++;
        if (c == 0) {
            break;
        }
        h = h << 6 | h >> 26;
        h += c;
    }
    return h;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/util", Str_Hash);
#endif

// Inserts node by ascending priority, after the nodes of the same priority.
void VSyncList_Add(VSyncCallback **list, VSyncCallback *node)
{
    int leave = Sys_CriticalSection(0);
    VSyncCallback *prev = 0;
    unsigned int priority = node->priority;
    VSyncCallback *cur = *list;

    while (cur != 0) {
        VSyncCallback *next = cur->next;
        if (priority < cur->priority) {
            break;
        }
        prev = cur;
        cur = next;
    }
    node->prev = prev;
    node->next = cur;
    if (cur != 0) {
        cur->prev = node;
    }
    if (prev != 0) {
        prev->next = node;
    } else {
        *list = node;
    }
    Sys_CriticalSection(leave);
}

#ifdef NON_MATCHING
// The game copies prev and next once more ($a1, $a3) and doesn't reuse node's register;
// GCC drops the copies, whatever the form tried (locals, inline helpers).
void VSyncList_Remove(VSyncCallback **list, VSyncCallback *node)
{
    int leave = Sys_CriticalSection(0);
    VSyncCallback *prev = node->prev;
    VSyncCallback *next = node->next;

    if (next != 0) {
        next->prev = prev;
    }
    if (prev != 0) {
        prev->next = next;
    } else {
        *list = next;
    }
    Sys_CriticalSection(leave);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/util", VSyncList_Remove);
#endif

// Calls each callback; a callback can remove itself.
void VSyncList_Run(VSyncCallback **list)
{
    VSyncCallback *next = *list;

    while (next != 0) {
        VSyncCallback *cb = next;
        next = cb->next;
        cb->func();
    }
}

// Standard reflected CRC-32.
unsigned int Crc32(unsigned char *data, int size)
{
    unsigned int crc = -1;
    unsigned char *end = data + size;
    unsigned int *table = D_800A6ACC;

    if (size != 0) {
        do {
            crc = table[(crc ^ *data++) & 0xff] ^ crc >> 8;
        } while (data != end);
    }
    return ~crc;
}

}
