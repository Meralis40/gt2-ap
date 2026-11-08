// Resident code 8008264c-80082ba8 (mcport). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#include <stdarg.h>
#include "common.h"

extern "C" {
extern TextCtx (&D_801F0D00);
extern unsigned int (&D_801F0D14);

// The frame a sector is read from: its replacement frame (36-55) if it is bad.
// The game computes the loop's end (port + 0x23c) inside the loop, in the load delay
// slot; a do-while hoists it, and a goto loop puts it in the branch delay slot.
int McPort_MapSector(McPort *port, unsigned int sector)
{
    int *p = port->badSectors;
    int frame = 36;

loop:
    if (*p == sector) {
        return frame;
    }
    p++;
    frame++;
    if (p != port->badSectors + 20) {
        goto loop;
    }
    return sector;
}

// Gives a bad sector a replacement frame (and frees the one it had, if any).
// Returns the frame's index, or -1 when none is free.
// The game computes the loop's end inside the loop and keeps 0 in a register for the
// store; GCC hoists the end and stores $zero.
int McPort_RemapBadSector(McPort *port, unsigned int sector)
{
    int *p = port->badSectors;
    int i = 0;
    int found = -1;

    do {
        int s = *p;
        int t = s + 1;

        if (s == sector) {
            *p = 0;
        }
        if ((t | (found + 1)) == 0) { // s and found both -1
            *p = sector;
            found = i;
        }
        p++;
        i++;
    } while (p != port->badSectors + 20);
    return found;
}

// Marks the frames of each file's block chain used (1), or free when the chain is
// broken, then rebuilds the free list and clears the free frames that aren't.
// The game keeps the first loops' pointers in temporaries and computes the marking
// loop's end inside it; GCC uses more saved registers and a different order.
void McPort_RebuildBlockUse(McPort *port)
{
    McDirEntry *e;
    signed char *s;
    signed char *q;
    int i;

    memset(port->dirState, 0, 15);
    e = port->dir;
    i = 0;
    do {
        if (e->state == 0x51) {
            int j = i;
            int ok = 1;

            do {
                McDirEntry *f = &port->dir[j];
                int next = f->next;

                if ((f->state & 0xf0) != 0x50) {
                    ok = 0;
                    break;
                }
                port->dirState[j] = 2;
                j = next;
            } while (j != -1);
            s = port->dirState;
            do {
                if (*s == 2) {
                    *s = ok;
                }
                s++;
            } while (s != port->dirState + 15);
        }
        e++;
        i++;
    } while (e != port->dir + 15);

    s = port->dirState;
    q = port->freeList;
    do {
        if (*s == 0) {
            *q++ = s - port->dirState;
        }
        s++;
    } while (s != port->dirState + 15);
    port->freeBlocks = q - port->freeList;

    e = port->dir;
    s = port->dirState;
    do {
        if (*s == 0 && e->state != 0xa0) {
            *s = -1;
            McDirEntry_Clear(e);
        }
        e++;
        s++;
    } while (e != port->dir + 15);
}

// The game computes the loop's end before the increment; GCC computes it after.
McDirEntry *McDir_FindFile(McPort *port, char *name)
{
    McDirEntry *e = port->dir;

loop:
    {
        int state = e->state;
        char *n = e->name;

        if (state == 0x51 && strcmp(name, n) == 0) {
            return e;
        }
    }
    e++;
    if (e != port->dir + 15) {
        goto loop;
    }
    return 0;
}

// Finds a file and copies its chain of directory frames.
McDirEntry *McDir_GetBlockChain(McPort *port, char *name, signed char *chain)
{
    McDirEntry *e = McDir_FindFile(port, name);

    if (e != 0) {
        McDirEntry *f = e;
        int next;

        do {
            next = f->next;
            *chain++ = f->frame;
            f = &port->dir[next];
        } while (next >= 0);
    }
    return e;
}

// The game computes the loop's end before the increment and the memset address again
// after the loop, where GCC reuses e. The permuter matches it with an if whose two arms
// are both "end = port->dir + 15; e++;", a block boundary that isn't real code.
void McPort_ResetDir(McPort *port)
{
    McDirEntry *e = port->dir;

loop:
    McDirEntry_Clear(e);
    e++;
    if (e != port->dir + 15) {
        goto loop;
    }
    memset(port->badSectors, -1, sizeof(port->badSectors));
}

// Chains the first blocks frames of the free list into a file (0x51 its first
// frame, 0x52 the middle ones, 0x53 the last). Returns the first frame's entry.
McDirEntry *McPort_AllocBlocks(McPort *port, int blocks)
{
    McDirEntry *first = 0;
    McDirEntry *prev = 0;
    signed char *p = port->freeList;
    signed char *end = p + blocks;

    do {
        int b = *p++;
        McDirEntry *e = &port->dir[b];
        int state;

        port->dirState[b] = 0;
        e->frame = b + 1;
        if (prev != 0) {
            prev->next = b;
        }
        e->next = -1;
        state = p < end; // 0x52 or 0x53, as two steps (one expression is a branch)
        state ^= 0x53;
        if (first == 0) {
            first = e;
            state = 0x51;
        }
        e->state = state;
        prev = e;
    } while (p != end);
    return first;
}

void McDirEntry_Clear(McDirEntry *e)
{
    memset(e, 0, 0x20);
    e->state = 0xa0;
    e->next = -1;
}

void McDirEntry_FromFrame(McDirEntry *e, int frame, McDirFrame *f)
{
    int size = f->size;
    int next = f->next;

    e->size = size;
    e->state = f->state;
    e->frame = frame;
    e->unk05 = f->unk7e;
    e->next = next;
    strcpy(e->name, f->name);
}

void McFrame_FromDirEntry(McDirEntry *e, McDirFrame *f)
{
    int size;
    int next;

    memset(f, 0, sizeof(McDirFrame));
    size = e->size;
    next = e->next;
    f->size = size;
    f->state = e->state;
    f->unk7e = e->unk05;
    f->next = next;
    strcpy(f->name, e->name);
    f->checksum = McFrame_Checksum((unsigned int *)f);
}

void Text_SetDefaultTexturePage(int tpage)
{
    Text_SetTexturePage(&D_801F0D00, tpage);
}

void FUN_80082abc(Font *param_1)
{
    Text_SetFont(&D_801F0D00, param_1);
}

void FUN_80082ae4(void)
{
}

void FUN_80082aec(unsigned int param_1)
{
    D_801F0D14 = param_1;
}

// Prints at (x, y) into ot with the default text context.
void FUN_80082af8(unsigned int *ot, int x, int y, char *fmt, ...)
{
    TextCtx *ctx = &D_801F0D00;

    ctx->ot = ot;
    ((short *)&ctx->unk18)[0] = x;
    y += ctx->font.unk0a;
    ((short *)&ctx->unk18)[1] = y;
    va_list args;

    va_start(args, fmt);
    FUN_8007db30(ctx, fmt, args);
    va_end(args);
}

// Draws s at (x, y) into ot with the default text context.
void FUN_80082b40(unsigned int *ot, int x, int y, char *s)
{
    TextCtx *ctx = &D_801F0D00;

    ctx->ot = ot;
    ((short *)&ctx->unk18)[0] = x;
    ((short *)&ctx->unk18)[1] = y;
    FUN_8007db9c(ctx, (signed char *)s);
}

unsigned char McFrame_Checksum(unsigned int *frame)
{
    unsigned int sum = 0;
    unsigned int *end = frame + 32;

    do {
        sum ^= *frame++;
    } while (frame != end);
    sum ^= sum >> 16;
    sum ^= sum >> 8;
    return sum & 0xff;
}
}
