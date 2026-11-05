// Resident code 8007e8a0-8007f01c (card): the memory card driver's low level, the BIOS
// events and the vblank state machine that reads a card's directory and runs a port's
// reads and writes.
#include "common.h"

// A BIOS event the driver opens, and the handler that records it.
struct CardEvent {
    unsigned int cls;
    int spec;
    void (*func)();
};

// g_CardEventTable: the 9 events and a word of padding.
struct CardEvents {
    CardEvent events[9];
};

// CardSystem, with the fields McPort and CardSystem leave unnamed.
struct CardDriver {
    unsigned int unk00;
    int events[9];              // the handles of g_CardEventTable's events
    VSyncCallback vsync;
    unsigned short flags;       // the events seen, CARD_EV_ bits
    unsigned char port;         // the port the state machine is on
    unsigned char unk3b;
    int sector;                 // the running transfer's
    void *buffer;
    McPort ports[2];
    unsigned char frame[0x80];  // g_McFrame
};

// A port of the driver, at CardDriver.ports.
struct CardPort {
    int (*op)(int event, int *sector, void **buf);
    int frameIndex;             // the next frame to read of the directory or bad sectors
    unsigned char unk008[0x253];
    unsigned char addr;         // the port's BIOS number: 0 or 0x10
    unsigned char locked;       // stay on this port when a run ends
    unsigned char state;        // the result of the last run, as for result
    unsigned char formatted;
    unsigned char dirty;        // set when the directory has been read: 1 skips reading it
    unsigned char unk260;
    unsigned char step;         // the state machine's step, 0 idle
    unsigned char result;       // 1 running, otherwise as for state
    unsigned char retries;
};

// The events an operation gets. It returns the next step, 0 to end the run.
#define CARD_DONE 0
#define CARD_NEWCARD 2
#define CARD_TIMEOUT 4
#define CARD_ERROR 5

extern "C" {
extern CardEvents g_CardEventTable;

// Opens and enables the card events and adds the driver to the vblank list.
// The address in $a0 first (matching-quirks.md, not solved).
void Card_Init()
{
    CardEvents table = g_CardEventTable;
    CardDriver *card = (CardDriver *)&g_Card;
    int i;

    memset(card, 0, 0x58c);
    card->vsync.func = Card_VSync;
    card->vsync.priority = 0xfffe0000;
    ((CardPort *)&card->ports[0])->addr = 0;
    ((CardPort *)&card->ports[0])->state = 1;
    ((CardPort *)&card->ports[1])->addr = 0x10;
    ((CardPort *)&card->ports[1])->state = 1;
    EnterCriticalSection();
    for (i = 0; i < 9; i++) {
        CardEvent *e = &table.events[i];

        card->events[i] = OpenEvent(e->cls, e->spec, 0x1000, e->func);
        EnableEvent(card->events[i]);
    }
    ExitCriticalSection();
    VSyncList_Add(&g_GfxIrq.vsyncCallbacks, &card->vsync);
}

// Removes the driver from the vblank list and closes the events.
// The address in $a0 first.
void func_8007E9DC(void)
{
    CardDriver *card = (CardDriver *)&g_Card;
    int *e;

    VSyncList_Remove(&g_GfxIrq.vsyncCallbacks, &card->vsync);
    EnterCriticalSection();
    for (e = card->events; e != (int *)&card->vsync; e++) {
        CloseEvent(*e);
    }
    ExitCriticalSection();
}

// Sets the port's operation and returns the old one.
void *func_8007EA44(CardSystem *card, int port, void *op)
{
    void *old = (void *)card->ports[port].op;

    card->ports[port].op = (int (*)(int, int *, void **))op;
    return old;
}

// The BIOS event handlers. Each is a `j Card_SetEventFlag` with its flag in the delay
// slot and the last one falls into it: GCC makes neither.
int Card_EvHwIOE(int unused)
{
    return Card_SetEventFlag(unused, 1);
}

int Card_EvHwError(int unused)
{
    return Card_SetEventFlag(unused, 2);
}

int Card_EvHwTimeout(int unused)
{
    return Card_SetEventFlag(unused, 4);
}

int Card_EvHwNewCard(int unused)
{
    return Card_SetEventFlag(unused, 8);
}

int Card_EvSwIOE(int unused)
{
    return Card_SetEventFlag(unused, 0x10);
}

int Card_EvSwError(int unused)
{
    return Card_SetEventFlag(unused, 0x20);
}

int Card_EvSwTimeout(int unused)
{
    return Card_SetEventFlag(unused, 0x40);
}

int Card_EvSwNewCard(int unused)
{
    return Card_SetEventFlag(unused, 0x80);
}

int Card_EvHwUnknown(int unused)
{
    return Card_SetEventFlag(unused, 0x100);
}

// Records an event. The handlers run in the interrupt, Card_VSync picks it up.
int Card_SetEventFlag(int unused, int bit)
{
    register CardDriver *card __asm__("a0") = (CardDriver *)&g_Card;

    card->flags |= bit;
    return 0;
}

// The vblank state machine of the port it is on: picks up the event of the last command,
// goes to the next step, and issues that step's command. The first steps read the card's
// header, its bad sector list and directory after a new card; an operation (McPort.op)
// then gets each event at its transfer steps and answers with the next step.
// Needs its 3 jump tables in .rodata; the address in $a0 first, spilled to the stack.
void Card_VSync(void)
{
    CardDriver *card = (CardDriver *)&g_Card;
    int slot = card->port;
    int sent = 0;
    McPort *mc = &card->ports[slot];
    CardPort *port = (CardPort *)mc;
    int flags = card->flags;
    int step = port->step;
    int frame = port->frameIndex;
    int result = 1;
    int event = 0;
    int (*op)(int, int *, void **);
    int ret;

    // The first step (1, _card_info) and those past the commands run on the BIOS's
    // software events, the others on the hardware's.
    if (step != 0) {
        if (step < 8 && step >= 2) {
            if (flags & 0x102) event = 1;
            else if (flags & 4) event = 2;
            else if (flags & 8) event = 3;
            else if (flags & 1) event = 4;
        } else {
            if (flags & 0x20) event = 1;
            else if (flags & 0x40) event = 2;
            else if (flags & 0x80) event = 3;
            else if (flags & 0x10) event = 4;
        }
    }

    op = port->op;
    switch (event) {
    case 0: // no event: starts from idle, or waits for the command
        if (step != 0) return;
        step = 1;
        break;

    case 1: // error: retried twice, then the operation is told
        if (port->retries++ < 2) {
            flags = 0;
            break;
        }
        result = 5;
        if (op != 0) {
            step = op(CARD_ERROR, &card->sector, &card->buffer);
            if (step != 0) {
                result = 1;
                break;
            }
        }
        goto finished;

    case 2: // timeout
        if (port->retries++ < 2) {
            flags = 0;
            break;
        }
        flags = 0;
        result = 4;
        if (op != 0) op(CARD_TIMEOUT, &card->sector, &card->buffer);
        goto finished;

    case 3: // new card: during a transfer, clears it once or twice and goes on
        result = 2;
        if (step < 8 && step >= 6) {
            int retries = port->retries;

            if (retries < 2) {
                sent = 1;
                if (_card_clear(port->addr) != 0) {
                    port->retries = retries + 1;
                    flags = 0;
                }
                result = 1;
                break;
            }
        }
        step = 2;
        if (op != 0) op(CARD_NEWCARD, &card->sector, &card->buffer);
        break;

    case 4: // the command is done
        port->retries = 0;
        switch (step) {
        case 1: // _card_info
            step = 3;
            if (port->dirty == 0) break;
            goto opDone;
        case 2: // _card_clear
            step++;
            break;
        case 3: // the header frame
            if (McFrame_Checksum((unsigned int *)card->frame) == 0
                && *(unsigned short *)card->frame == 0x434d) {
                step++;
                frame = 0;
                port->formatted = 1;
                port->dirty = 0;
                port->unk260 = 0;
                break;
            }
            result = 3;
            port->formatted = 0;
            port->dirty = 1;
            port->unk260 = 0;
            goto finished;
        case 4: { // a frame of the bad sector list
            int bad = 0;

            if (McFrame_Checksum((unsigned int *)card->frame) == 0) {
                bad = *(int *)card->frame;
            }
            mc->badSectors[frame] = bad;
            frame++;
            if (frame == 20) {
                frame = 0;
                step++;
            }
            break;
        }
        case 5: { // a frame of the directory
            McDirEntry *entry = &mc->dir[frame];

            frame++;
            if (McFrame_Checksum((unsigned int *)card->frame) == 0) {
                McDirEntry_FromFrame(entry, frame, (McDirFrame *)card->frame);
            } else {
                McDirEntry_Clear(entry);
            }
            if (frame != 15) break;
            port->dirty = 1;
            McPort_RebuildBlockUse(mc);
            goto opDone;
        }
        case 6:
        case 7: // a transfer
        opDone:
            if (op != 0) {
                step = op(CARD_DONE, &card->sector, &card->buffer);
                if (step != 0) break;
            }
            result = port->formatted != 0 ? 0 : 3;
            goto finished;
        default:
            step++;
            break;
        }
        break;

    finished:
        flags = 0;
        if (port->locked == 0) slot ^= 1;
        step = 0;
        break;
    }

    if (!sent) {
        int addr = port->addr;

        switch (step) {
        case 1:
            ret = _card_info(addr);
            break;
        case 2:
            ret = _card_clear(addr);
            break;
        case 3:
            ret = _card_read(addr, 0, card->frame);
            break;
        case 4:
            ret = _card_read(addr, frame + 0x10, card->frame);
            break;
        case 5:
            ret = _card_read(addr, McPort_MapSector(mc, frame + 1), card->frame);
            break;
        case 6:
            ret = _card_read(addr, card->sector, card->buffer);
            break;
        case 7:
            ret = _card_write(addr, card->sector, card->buffer);
            break;
        default:
            ret = 0;
            break;
        }
        if (ret != 0) flags = 0;
    }

    card->port = slot;
    card->flags = flags;
    port->step = step;
    port->result = result;
    port->frameIndex = frame;
    if (result != 1) {
        port->state = result;
        port->retries = 0;
    }
}

}
