// Resident code 8007d32c-8007da44 (mccard). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#include "common.h"

extern "C" {
extern char g_McCreateName[];

void McHeader_Init(McFileHeader *h)
{
    memset(h, 0, 0x80); // the first frame
    h->magic = 0x4353;
    h->blockCount = 1;
    h->iconType = 0x11;
}

// Sets the title from two-byte codes (ASCII in their low 7 bits), as Shift-JIS.
// The game loads the first byte twice, signed for the test and unsigned for the code,
// and the second byte first; GCC tests the unsigned load.
void McHeader_SetTitle(McFileHeader *h, char *title)
{
    unsigned char *p = h->title;

    memset(p, 0, sizeof(h->title));
    while (*title != 0) {
        int lo = title[1] & 0x7f;
        int c = Ascii_ToSjis((title[0] & 0x7f) << 8 | lo);

        title += 2;
        p[0] = c >> 8;
        p[1] = c;
        p += 2;
    }
}

McDirEntry *McCard_FindFile(int port, char *name)
{
    return McDir_FindFile(&Card_Get()->ports[port], name);
}

// Starts creating a file of size bytes. Returns 0, or why not: 5 the card isn't ready,
// 1 busy, 6 the port is flagged (+0x260), 7 the file exists, 9 not enough free blocks.
int McCard_StartCreate(int port, char *name, unsigned int size)
{
    int leave = Sys_CriticalSection(0);
    int result = 5;
    McPort *p = &Card_Get()->ports[port];
    int blocks;

    if (p->state == 0) {
        result = 1;
        if (p->op == 0) {
            result = 6;
            if (p->unk260 == 0) {
                result = 7;
                if (McDir_FindFile(p, name) == 0) {
                    blocks = (size + 0x1fff) >> 13;
                    result = 9;
                    if (p->freeBlocks >= blocks) {
                        strcpy(g_McCreateName, name);
                        result = 0;
                        g_McOp_Port.port = p;
                        g_McOp_Port.index = 0;
                        g_McOp_Port.size = size;
                        g_McOp_Port.remaining = blocks;
                        g_McOp_Port.result = 5;
                        p->op = McOp_Create;
                    }
                }
            }
        }
    }
    Sys_CriticalSection(leave);
    return result;
}

// Allocates the blocks, then writes the directory frames from the first.
// The game loads &g_McOp_Port into $a0 first, moving the arguments up to $a1-$a3, then
// copies it to a saved register (as in Overlay_Load); the rest is the game's code.
int McOp_Create(int event, int *sector, void **buf)
{
    McOpState *op = McOp_Get();
    McPort *port = op->port;
    signed char *used;
    int i;

    if (event == 0) {
        i = op->index;
        *buf = g_McFrame;
        if (i == 0) {
            McDirEntry *e;

            port->dirty = 0;
            e = McPort_AllocBlocks(port, op->remaining);
            e->size = op->size;
            strcpy(e->name, g_McCreateName);
        }
        i = op->index;
        used = &port->dirState[i];
        if (i != 15) {
            do {
                McDirEntry *e = &port->dir[i];
                int u = *used;

                i++;
                *sector = i;
                op->index = i;
                if (u == 0) {
                    McFrame_FromDirEntry(e, (McDirFrame *)g_McFrame);
                    return 7;
                }
                used++;
            } while (i != 15);
        }
        McPort_RebuildBlockUse(port);
        port->dirty = 1;
        op->result = 0;
    } else {
        port->dirty = 0;
    }
    port->op = 0;
    return 0;
}

// Starts reading (cmd 6) or writing (7) size bytes of a file from offset (size 0 for
// the rest of it). Returns 0, or why not: 5 the card isn't ready, 1 busy, 8 no file.
int McCard_StartTransfer(int cmd, int port, char *name, unsigned char *buf, unsigned int offset, int size,
                         unsigned short *sectorMap)
{
    int leave = Sys_CriticalSection(0);
    int result = 5;
    McPort *p = &Card_Get()->ports[port];
    McDirEntry *e;

    if (p->state == 0) {
        result = 1;
        if (p->op == 0) {
            g_McOp_Port.port = p;
            g_McOp_Port.cmd = cmd;
            g_McOp_Port.sectorMap = sectorMap;
            g_McOp_Port.buffer = buf - 0x80;
            g_McOp_Port.offset = offset;
            e = McDir_GetBlockChain(p, name, g_McOp_Port.blocks);
            result = 8;
            if (e != 0) {
                int fileSize = e->size;

                g_McOp_Port.index = -1;
                g_McOp_Port.phase = 0;
                g_McOp_Port.result = 5;
                if (size == 0) {
                    size = fileSize - offset;
                }
                result = 0;
                g_McOp_Port.remaining = size + 0x80;
                g_McOp_Port.size = size;
                p->op = McOp_Transfer;
            }
        }
    }
    Sys_CriticalSection(leave);
    return result;
}

void FUN_8007d7a0(void)
{
    g_McOp_Port.port->op = 0;
}

unsigned int McOp_FileToCardSector(McOpState *op, unsigned int fileSector)
{
    return op->blocks[(int)fileSector >> 6] << 6 | fileSector & 0x3f;
}

// 1 while an operation runs, then its result (0 done).
// The game keeps &g_McOp_Port in $a0 and loads through it; GCC folds the address into
// the loads.
int McCard_GetResult(void)
{
    McOpState *op = McOp_Get();

    if (op->port->op != 0) {
        return 1;
    }
    return op->result;
}

// Moves the file a sector a step (event 0 when the last one went through, 5 when it
// failed, which is retried). Writes read the sector back and compare it; a bad one is
// remapped.
// The game loads &g_McOp_Port into $a0 first, moving the arguments up to $a1-$a3, then
// copies it to a saved register (as in Overlay_Load); the rest differs in registers.
int McOp_Transfer(int event, int *sector, void **buf)
{
    McOpState *op = &g_McOp_Port;
    McPort *port = op->port;
    int cmd = op->cmd;
    int phase = op->phase;
    int s;
    unsigned int n;
    unsigned int rem;
    unsigned int off;
    unsigned char *p;
    unsigned short *map;

    switch (event) {
    case 0:
        switch (phase) {
        case 0:
            if (cmd == 7) {
                op->phase = 2;
            }
            break;
        case 1:
            op->phase = 2;
            if (memcmp(g_McFrame, op->buffer, 0x80) == 0) {
                break;
            }
        remap:
            s = McPort_RemapBadSector(port, op->cardSector);
            if (s == -1) {
                goto stop;
            }
            *sector = s + 0x10;
            *buf = g_McFrame;
            McFrame_InitBadSector((int *)g_McFrame, op->cardSector);
            op->phase = 3;
            return 7;
        case 2:
            op->phase = 1;
            *sector = op->cardSector;
            *buf = g_McFrame;
            return 6;
        case 3:
            op->phase = 2;
            *sector = McPort_MapSector(port, op->cardSector);
            *buf = op->buffer;
            return 7;
        }
        p = op->buffer + 0x80;
        rem = op->remaining;
        n = rem - 0x80;
        op->buffer = p;
        *buf = p;
        if (rem < n) {
            n = 0;
        }
        op->remaining = n;
        if (n == 0) {
            op->result = 0;
            goto stop;
        }
        s = op->index + 1;
        map = op->sectorMap;
        off = op->offset;
        op->index = s;
        if (map != 0) {
            s = map[s];
        }
        s = McOp_FileToCardSector(op, s + (off >> 7));
        op->cardSector = s;
        *sector = McPort_MapSector(port, s);
        op->retries = 10;
        break;
    case 5:
        p = op->buffer;
        n = op->retries;
        s = op->cardSector;
        *buf = p;
        *sector = s;
        op->retries = n - 1;
        if (n != 0) {
            break;
        }
        op->retries = 10;
        if (cmd == 7 && phase != 3) {
            goto remap;
        }
        goto stop;
    default:
        goto stop;
    }
    return op->cmd;
stop:
    port->op = 0;
    return 0;
}
}
