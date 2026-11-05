// OVL0 800163b8-8001681c (inputlog): Input logs.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "common.h"

// A frame of a car's input, as InputLog stores it.
struct RaceInput {
    unsigned char flags;
    unsigned char buttons;
    unsigned char steer;
    unsigned char throttle;
    unsigned char brake;
};

extern "C" {
void InputLog_Flush(InputLog *log);
// Recording: clears the header. Playback: rewinds (framesLeft = framesWritten).
// runLength = -1.
void InputLog_Reset(InputLog *log, int forPlayback, unsigned short capacity)
{
    if (log != (InputLog *)0x0) {
        if (forPlayback == 0) {
            memset(log, 0, 0x1c);
        }
        else {
            memset(&log->header, 0, 5);
            log->ended = 0;
            log->pos = 0;
            log->framesLeft = log->framesWritten;
        }
        log->capacity = capacity;
        log->runLength = -1;
    }
}

// Plays back a frame: a packet holds the values that changed and how many frames repeat them.
void InputLog_Read(InputLog *log, RaceInput *out)
{
    if (log != 0) {
        int left = log->framesLeft;
        unsigned int flags;
        unsigned int buttons;
        unsigned int steer;
        unsigned int pedals;
        int run;

        if (log->ended != 0) {
            return;
        }
        left--;
        log->framesLeft = left;
        if (left == 0) {
            log->ended = 1;
            log->runLength = 0;
            return;
        }
        flags = log->flags;
        buttons = log->buttons;
        steer = log->steer;
        run = log->runLength;
        pedals = log->pedals;
        if (run-- <= 0) {
            unsigned char *data = log->data;
            unsigned char *p = data + log->pos;
            unsigned int header = *p++;

            log->header = header;
            if (header & 8) {
                flags = *p++;
                log->flags = flags;
            }
            if (header & 0x10) {
                buttons = *p++;
                log->buttons = buttons;
            }
            if (header & 0x20) {
                steer = *p++;
                log->steer = steer;
            }
            if (header & 0x40) {
                pedals = *p++;
                log->pedals = pedals;
            }
            run = 0;
            if (header & 0x80) {
                run = *p++;
                if (run & 0x80) {
                    run = (run & 0x7f) << 8 | *p++;
                    if (run & 0x4000) {
                        run = (run & 0x3fff) << 8 | *p++;
                        if (run & 0x200000) {
                            run = (run & 0x1fffff) << 8 | *p++;
                        }
                    }
                }
                run <<= 3;
            }
            run |= header & 7;
            log->pos = p - data;
        }
        log->runLength = run;
        out->flags = flags;
        out->buttons = buttons;
        out->steer = steer;
        out->throttle = pedals & 0xf;
        out->brake = pedals >> 4;
    }
}

// Writes the pending packet, and ends the log when it is nearly full.
void InputLog_Flush(InputLog *log)
{
    unsigned char *data = log->data;
    int pos = log->pos;
    int run = log->runLength;
    int header = log->header;
    int flags = log->flags;
    int buttons = log->buttons;
    int steer = log->steer;
    int pedals = log->pedals;
    unsigned char *p = data + pos;
    int length;

    header = (header & 0xf8) | (run & 7);

    if (run >= 8) {
        header |= 0x80;
    }
    *p++ = header;
    if (header & 8) {
        *p++ = flags;
    }
    if (header & 0x10) {
        *p++ = buttons;
    }
    if (header & 0x20) {
        *p++ = steer;
    }
    if (header & 0x40) {
        *p++ = pedals;
    }
    if (header & 0x80) {
        int high = 0;
        int mid = high;

        run >>= 3;
        if (run <= 0x1fffff) {
            if (run >= 0x4000) {
                high = 0xc0;
                goto three;
            }
            if (run < 0x80) {
                goto one;
            }
            mid = 0x80;
            goto two;
        }
        *p++ = (run >> 24) | 0xe0;
    three:
        *p++ = (run >> 16) | high;
    two:
        *p++ = (run >> 8) | mid;
    one:
        *p++ = run;
    }
    length = p - data;
    log->pos = length;
    log->length = length;
    if (length >= log->capacity - 0x11) {
        log->ended = 1;
    }
}

// Records a frame, adding to the run while nothing changes.
void InputLog_Write(InputLog *log, RaceInput *in)
{
    if (log != 0) {
        int count = log->framesWritten;
        int changed;
        int flags;
        int buttons;
        int steer;
        int pedals;
        int run;

        if (log->ended != 0) {
            return;
        }
        changed = 0x78;
        log->framesWritten = count + 1;
        pedals = in->throttle | in->brake << 4;
        flags = in->flags;
        buttons = in->buttons;
        run = log->runLength;
        steer = in->steer;
        if (run >= 0) {
            changed = (flags != log->flags) << 3;
            if (buttons != log->buttons) {
                changed |= 0x10;
            }
            if (steer != log->steer) {
                changed |= 0x20;
            }
            if ((pedals & 0xff) != log->pedals) {
                changed |= 0x40;
            }
            if ((changed & 0x78) == 0) {
                log->runLength = run + 1;
                return;
            }
            InputLog_Flush(log);
        }
        log->header = changed;
        log->flags = flags;
        log->buttons = buttons;
        log->steer = steer;
        log->pedals = pedals;
        log->runLength = 0;
    }
}

// Ends the log, writing the last packet unless discard is set.
void InputLog_Close(InputLog *log, int discard)
{
    if (log != 0) {
        int ended = log->ended;
        int run;

        log->ended = 1;
        run = log->runLength;
        if (ended == 0 && discard == 0 && run >= 0) {
            InputLog_Flush(log);
        }
    }
}
}
