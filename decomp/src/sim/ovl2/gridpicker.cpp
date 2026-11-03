// OVL2 8001c7ac-8001d13c (gridpicker): Grid picker.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "copies.h"
#include "common.h"

extern "C" {

// Copies the layout, the draw callback and its argument from the template; row and
// item 0, hidden.
void GridPicker_Init(ArcGridPicker *gp, ArcGridPickerTmpl *tmpl, signed char *enabled)
{
    gp->x = tmpl->x;
    gp->y = tmpl->y;
    gp->w = tmpl->w;
    gp->h = tmpl->h;
    gp->draw = tmpl->draw;
    gp->drawArg = tmpl->drawArg;
    gp->enabled = enabled;
    gp->cur.row = 0;
    gp->cur.item = 0;
    gp->anim = -1;
    gp->moving = 0;
}

// Sets the number of rows and the item count of each.
void GridPicker_SetColumns(ArcGridPicker *gp, int count, int *items)
{
    int i;

    for (i = 0; i < count; i++) {
        gp->rowItems[i] = items[i];
    }
    gp->rowCount = count;
}

// Starts the open animation.
void GridPicker_Open(ArcGridPicker *gp)
{
    gp->prev.row = -1;
    gp->prev.item = -1;
    gp->anim = 0;
    gp->slide = 0;
}

// Shown at once: the open animation finished.
void GridPicker_ShowNow(ArcGridPicker *gp)
{
    if (gp->anim < 12) {
        gp->anim = 12;
    }
    gp->prev.row = -1;
    gp->prev.item = -1;
    gp->slide = 0;
}

// Starts the close animation.
void GridPicker_Close(ArcGridPicker *gp)
{
    gp->anim = -17;
}

// Left/Right change the item (skipping the disabled ones), Up/Down the row. Returns -1
// on Triangle/Square, the item on Cross/Circle, -3 when the cursor moved, else -2.
// The game recomputes rowItems[row] in the Left/Right loops and allocates the
// registers in another order.
#ifdef NON_MATCHING
int GridPicker_Update(ArcGridPicker *gp, PadState *pad)
{
    int result = -2;
    int n;
    int i;
    int j;
    int row;
    unsigned int buttons;

    if (gp->anim < 0) {
        if (gp->anim < -1) {
            gp->anim++;
        }
        return -2;
    }
    if (++gp->anim > 0x38) {
        gp->anim = 12;
    }
    n = 1;
    if (gp->slideVertical == 1) {
        n = 2;
    }
    for (i = 0; i < n; i++) {
        ArcGridPos *prev = &gp->prev;

        if (gp->slide > 0) {
            gp->slide--;
        }
        if (gp->slide < 0) {
            gp->slide++;
        }
        if (gp->slide == 0) {
            gp->prev.row = -1;
            prev->item = -1;
        }
    }
    if (gp->enabled != 0 && gp->enabled[gp->cur.item] == 0) {
        int count = gp->rowItems[gp->cur.row];
        for (j = 0; j < count; j++) {
            signed char *en = gp->enabled;
            if (en[j] != 0) {
                gp->cur.item = j;
                break;
            }
        }
    }
    gp->moving = 0;
    if (pad != 0) {
        buttons = pad->pressed;
        row = gp->cur.row;
        if (buttons & (PAD_SQUARE | PAD_TRIANGLE)) {
            return -1;
        }
        if (buttons & (PAD_CIRCLE | PAD_CROSS)) {
            return gp->cur.item;
        }
        buttons |= pad->repeat;
        if (gp->rowItems[row] > 1) {
            i = gp->cur.item;
            j = i;
            gp->moving = 1;
            if (buttons & PAD_LEFT) {
                do {
                    if (--i < 0) {
                        i = gp->rowItems[row] - 1;
                    }
                } while (gp->enabled != 0 && gp->enabled[i] == 0);
                if (i != j) {
                    result = -3;
                    gp->prev.row = row;
                    gp->prev.item = j;
                    gp->slide = -12;
                    gp->slideVertical = 0;
                }
            }
            if (buttons & PAD_RIGHT) {
                do {
                    if (++i >= gp->rowItems[row]) {
                        i = 0;
                    }
                } while (gp->enabled != 0 && gp->enabled[i] == 0);
                if (i != j) {
                    result = -3;
                    gp->prev.row = row;
                    gp->prev.item = j;
                    gp->slide = 12;
                    gp->slideVertical = 0;
                }
            }
            gp->cur.item = i;
        }
        if (gp->rowCount > 1) {
            row = gp->cur.row;
            i = row;
            j = row;
            if (buttons & PAD_UP) {
                if (--i < 0) {
                    i = 0;
                }
                if (i != j) {
                    result = -3;
                    gp->prev.row = j;
                    gp->slide = -12;
                    gp->slideVertical = 1;
                    gp->prev.item = gp->cur.item;
                }
            }
            if (buttons & PAD_DOWN) {
                if (++i >= gp->rowCount) {
                    i = gp->rowCount - 1;
                }
                if (i != gp->cur.row) {
                    result = -3;
                    gp->slide = 12;
                    gp->prev.row = row;
                    gp->slideVertical = 1;
                    gp->prev.item = gp->cur.item;
                }
            }
            gp->cur.row = i;
        }
    }
    return result;
}
#else
INCLUDE_ASM("asm/sim/ovl2/nonmatchings/gridpicker", GridPicker_Update);
#endif

// The brightness of the open (0 to 0x80) and close animations.
int GridPicker_GetFade(ArcGridPicker *gp)
{
    int fade = 0;

    if (gp->anim < -1) {
        fade = (~gp->anim << 7) / 16;
    }
    if (gp->anim > 0) {
        fade = 0x80;
        if (gp->anim < 12) {
            fade = (gp->anim << 7) / 12;
        }
    }
    return fade;
}

// Draws the open and close bands, the Left/Right and Up/Down arrows, then the item
// callback for the item sliding out and the current one.
// The game keeps one more value in a saved register (s7) and reads the fields
// in another order.
#ifdef NON_MATCHING
void GridPicker_Draw(ArcGridPicker *gp, void *ot)
{
    int x = gp->x;
    int y = gp->y;
    short w = gp->w;
    int h = gp->h;
    GouraudQuad band;
    ArcGridPickerDraw ctx;
    void (*draw)(ArcGridPickerDraw *, int);
    int item;
    int anim;

    ctx.picker = gp;
    ctx.ot = ot;
    ctx.y = y;
    ctx.arg = gp->drawArg;
    anim = gp->anim;
    if (anim < 0) {
        int t;
        int a;
        int b;
        int c;
        int f;
        int left;

        if (anim >= -1) {
            return;
        }
        t = anim + 17;
        a = w * t / 16;
        b = (w >> 1) * t / 16;
        c = h * ~anim / 16;
        f = (~anim << 7) / 16;
        left = x - (w >> 1);
        band.rect.x = left + a;
        band.rect.w = w + b - a;
        band.rect.y = y - (c >> 1);
        band.rect.h = c;
        band.top = 0;
        band.bottom = f;
        Menu_AddGreyBand((uint *)ot, &band);
        band.rect.x = left - b;
        band.top = f;
        band.bottom = 0;
        Menu_AddGreyBand((uint *)ot, &band);
        Prim_AddDrawMode(ot, 0x20);
        ctx.x = x;
        ctx.brightness = f;
        ctx.pos.row = -1;
        ctx.pos.item = -1;
        draw = gp->draw;
        item = -1;
    } else {
        int fade;
        int slide;
        int dx;
        int dy;
        int ox;
        int oy;

        if (anim >= 12 && (gp->moving & 1)) {
            int t = 0x34 - anim;
            int half;
            int c;
            unsigned int rgb;
            PrimF3 *p;
            short px;

            if (t > 10) {
                t = 10;
            }
            if (t < 0) {
                t = 0;
            }
            half = (gp->w >> 1) + 8;
            c = t * 0xff / 10;
            rgb = c | (c >> 1) << 8 | 0x2000000;
            p = Prim_AddPolyF3((uint *)ot, rgb);
            px = x + half;
            p->x0 = px + 6;
            p->x1 = px;
            p->x2 = px;
            p->y0 = y;
            p->y1 = y + 10;
            p->y2 = y - 10;
            p = Prim_AddPolyF3((uint *)ot, rgb);
            px = x - half;
            p->x0 = px - 6;
            p->x1 = px;
            p->x2 = px;
            p->y0 = y;
            p->y1 = y + 10;
            p->y2 = y - 10;
            Prim_AddDrawMode(ot, 0x20);
        }
        anim = gp->anim;
        if ((anim >= 12) & (gp->rowCount >= 2)) {
            int t = 0x34 - anim;
            int half;
            int c;
            PrimF3 *p;
            short py;

            if (t > 10) {
                t = 10;
            }
            if (t < 0) {
                t = 0;
            }
            half = (gp->h >> 1) + 6;
            c = t * 0xff / 10;
            if (gp->cur.row > 0) {
                p = Prim_AddPolyF3((uint *)ot, c | (c >> 1) << 8 | 0x2000000);
                p->x1 = x + 6;
                p->x2 = x - 6;
                py = y - half;
                p->x0 = x;
                p->y0 = py - 8;
                p->y1 = py;
                p->y2 = py;
            }
            if (gp->cur.row < gp->rowCount - 1) {
                p = Prim_AddPolyF3((uint *)ot, c | (c >> 1) << 8 | 0x2000000);
                p->x1 = x + 6;
                p->x2 = x - 6;
                py = y + half;
                p->x0 = x;
                p->y0 = py + 8;
                p->y1 = py;
                p->y2 = py;
            }
            Prim_AddDrawMode(ot, 0x20);
            anim = gp->anim;
        }
        fade = 0x80;
        if (anim < 12) {
            fade = (anim << 7) / 12;
        }
        slide = gp->slide;
        dx = gp->w * slide / 12;
        dy = gp->h * slide / 12;
        ox = -gp->w;
        oy = -gp->h;
        if (slide < 0) {
            slide = -slide;
            ox = gp->w;
            oy = gp->h;
        }
        fade = fade * (12 - slide) / 12;
        if (gp->slideVertical == 1) {
            dx = 0;
            ox = 0;
        } else {
            dy = 0;
            oy = 0;
        }
        if (gp->prev.item >= 0) {
            ctx.x = x + dx + ox;
            ctx.y = y + dy + oy;
            ctx.brightness = 0x80 - fade;
            ctx.pos = gp->prev;
            gp->draw(&ctx, gp->cur.item);
        }
        ctx.x = x + dx;
        ctx.y = y + dy;
        ctx.brightness = fade;
        ctx.pos = gp->cur;
        item = gp->cur.item;
        draw = gp->draw;
    }
    draw(&ctx, item);
}
#else
INCLUDE_ASM("asm/sim/ovl2/nonmatchings/gridpicker", GridPicker_Draw);
#endif
}
