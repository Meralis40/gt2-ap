// OVL1 8001d4b0-8001da08 (tabselector): Tab selector.
// The functions that don't match yet are the original assembly, with their C++
// under NON_MATCHING.
#include "common.h"

typedef void TabCallback(int event, TabEvent *ev, int tab);

extern "C" {
// Horizontal tab selector (Left/Right change tab). callback(event, ev, tab): 0 update,
// 1 draw, 3 confirm, 4 changed.
void TabSelector_Init(TabSelector *tabs, void *callback, void *userData)
{
    tabs->selected = '\0';
    tabs->anim = -1;
    tabs->hadInput = '\0';
    tabs->active = '\0';
    tabs->callback = callback;
    tabs->userData = userData;
    tabs->skipInput = '\0';
}

// Restarts the open animation.
void TabSelector_Open(TabSelector *tabs)
{
    tabs->anim = 0;
    tabs->slide = 0;
    tabs->previous = -1;
}

// Starts the close animation.
void TabSelector_Close(TabSelector *tabs)
{
    tabs->anim = -0x11;
}

// Runs the animations and the input: -3 the tab changed, -2 nothing, -1 back (Cross or
// Triangle on pad), or the tab confirmed. Without solo, confirming reads pad2 instead.
int TabSelector_Update(TabSelector *tabs, PadState *pad, PadState *pad2, int solo)
{
    TabEvent ev;
    int result = -2;
    unsigned int buttons;
    int sel;

    ev.tabs = tabs;
    ev.pad = pad;
    if (tabs->anim < 0) {
        if (tabs->anim < -1) {
            tabs->anim++;
        }
        return -2;
    }
    tabs->anim++;
    if (tabs->anim > 0x38) {
        tabs->anim = 0xc;
    }
    if (tabs->slide > 0) {
        tabs->slide--;
    }
    if (tabs->slide < 0) {
        tabs->slide++;
    }
    if (tabs->slide == 0) {
        tabs->previous = -1;
    }
    if (tabs->previous >= 0) {
        ((TabCallback *)tabs->callback)(0, &ev, tabs->previous);
    }
    ((TabCallback *)tabs->callback)(0, &ev, tabs->selected);
    if (tabs->skipInput != 0) {
        tabs->skipInput = 0;
        return -2;
    }
    tabs->hadInput = 0;
    if ((pad != 0) & (tabs->active == 0)) {
        buttons = pad->pressed;
        if ((buttons & 0x500) != 0) {
            return -1;
        }
        if (solo == 0) {
            buttons = pad2->pressed;
        }
        if ((buttons & 0xa00) != 0) {
            ((TabCallback *)tabs->callback)(3, &ev, tabs->selected);
            return tabs->selected;
        }
        buttons = pad->pressed | pad->repeat;
        if (tabs->count >= 2) {
            tabs->hadInput = 1;
            sel = tabs->selected;
            if ((buttons & 4) != 0) {
                tabs->previous = sel;
                sel--;
                result = -3;
                if (sel < 0) {
                    sel = tabs->count - 1;
                }
                tabs->slide = -0xc;
                tabs->selected = sel;
                ((TabCallback *)tabs->callback)(4, &ev, (signed char)sel);
            }
            if ((buttons & 8) != 0) {
                tabs->previous = sel;
                sel++;
                result = -3;
                if (sel >= tabs->count) {
                    sel = 0;
                }
                tabs->slide = 0xc;
                tabs->selected = sel;
                ((TabCallback *)tabs->callback)(4, &ev, (signed char)sel);
            }
        }
    }
    return result;
}

// The brightness of the open (0 to 0x80) and close animations.
int TabSelector_GetFade(TabSelector *tabs)
{
    int fade = 0;

    if (tabs->anim < -1) {
        fade = (~tabs->anim << 7) / 16;
    }
    if (tabs->anim > 0) {
        fade = 0x80;
        if (tabs->anim < 12) {
            fade = (tabs->anim << 7) / 12;
        }
    }
    return fade;
}

// Draws the side arrows, then the tab sliding out and the selected one.
void TabSelector_Draw(TabSelector *tabs, uint *ot)
{
    TabEvent ev;
    char arrow[8];
    int x = tabs->x;
    int y = tabs->y;
    int arrowAnim;
    int had;
    int fade;
    int off;
    int step;
    int dist;

    ev.tabs = tabs;
    ev.ot = ot;
    ev.y = y;
    {
        int anim = tabs->anim;

        arrowAnim = anim - 12;
        if (anim < 0) {
            if (anim >= -1) {
                return;
            }
            fade = (~anim << 7) / 16;
            ev.x = x;
            ev.brightness = fade;
            ((TabCallback *)tabs->callback)(1, &ev, tabs->selected);
            return;
        }
    }
    {
        if (arrowAnim >= 0 && ((had = tabs->hadInput) & 1) != 0) {
            off = (tabs->width >> 1) + 8;
            Menu_DrawSideArrow(arrow, ot, x + off + 6, y, 6, 10, arrowAnim);
            Menu_DrawSideArrow(arrow, ot, x - off - 6, y, -6, 10, arrowAnim);
            Prim_AddDrawMode(ot, 0x20);
        }
        fade = 0x80;
        if (tabs->anim < 12) {
            fade = (tabs->anim << 7) / 12;
        }
        off = (tabs->width * tabs->slide) / 12;
        dist = tabs->slide;
        step = -tabs->width;
        if (dist < 0) {
            dist = -dist;
            step = tabs->width;
        }
        dist = 12 - dist;
        fade = (fade * dist) / 12;
        if (tabs->previous >= 0) {
            ev.x = x + off + step;
            ev.brightness = 0x80 - fade;
            ((TabCallback *)tabs->callback)(1, &ev, tabs->previous);
        }
        ev.x = x + off / 2;
    }
    ev.brightness = fade;
    ((TabCallback *)tabs->callback)(1, &ev, tabs->selected);
}
}
