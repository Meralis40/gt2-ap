// OVL2 8001aab4-8001b37c (dialog): Two-option dialog.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "copies.h"
#include "common.h"

// The flat quad Prim_AddPolyF4 returns.
struct PrimF4 {
    unsigned int rgb;
    short x0, y0;
    short x1, y1;
    short x2, y2;
    short x3, y3;
};

extern "C" {
extern ArcTextItemTmpl D_8004F6D0; // the message's template


// A message over two option boxes, as CardDialog_Init.
void Dialog_Init_OVL2(ArcDialog *dlg, ArcDialogDesc *desc)
{
    int width;
    int height;
    Font *font = desc->font;
    ArcTextItem *item;
    ArcTextItem *option;
    int spacing;

    switch (desc->flags & 0xc) {
    case 0:
        width = 0x50;
        height = 0xc;
        break;
    case 8:
        width = 0x60;
        height = 0x18;
        break;
    case 0xc:
        width = 0x80;
        height = 0x18;
        break;
    default:
        width = 0x50;
        height = 0xc;
        break;
    }
    item = &dlg->message;
    dlg->boxWidth = width;
    dlg->boxHeight = height;
    D_8004F6D0.font = font;
    TextItem_Init_OVL2(item, &D_8004F6D0);
    option = &dlg->option0;
    item->text = desc->message;
    TextItem_Init_OVL2(option, 0);
    item = &dlg->option1;
    option->font = font;
    spacing = desc->spacing;
    option->flags = 0x40;
    option->spacing = spacing;
    option->text = desc->option0;
    TextItem_Init_OVL2(item, 0);
    item->font = font;
    spacing = desc->spacing;
    item->flags = 0x40;
    item->spacing = spacing;
    item->text = desc->option1;
    dlg->choice = desc->choice;
    dlg->anim = -1;
    dlg->flash = 0;
    dlg->pad89 = 0;
    dlg->flags = desc->flags;
}

// Starts the open animation and the text reveals.
void Dialog_Open_OVL2(ArcDialog *dlg)
{
    TextItem_Show_OVL2(&dlg->message, 0x3c);
    TextItem_Show_OVL2(&dlg->option0, -1);
    TextItem_Show_OVL2(&dlg->option1, -1);
    dlg->anim = 0;
    dlg->flash = 0;
}

// Hidden at once.
void FUN_OVL2__8001ac40(ArcDialog *dlg)
{
    dlg->anim = -1;
    dlg->flash = 0;
    dlg->message.anim = -1;
    dlg->option0.anim = -1;
    dlg->option1.anim = -1;
}

// Starts the close animation (anim -17).
void Dialog_Close_OVL2(ArcDialog *dlg)
{
    dlg->anim = -0x11;
    TextItem_Hide_OVL2(&dlg->message);
}

// Returns the choice when confirmed, -1 when cancelled, -3 when the choice moved and -2
// otherwise (and while closed or closing).
int Dialog_Update_OVL2(ArcDialog *dlg, PadState *pad)
{
    int result = -2;

    if (dlg->anim < 0) {
        if (dlg->anim < -1) {
            dlg->anim++;
        }
        TextItem_Update_OVL2(&dlg->message);
        return -2;
    }
    if (++dlg->anim >= 0x48) {
        dlg->anim = 12;
        dlg->flags &= ~0x80;
    }
    if (dlg->flash > 0) {
        dlg->flash--;
    }
    if (pad != 0) {
        int choice = dlg->choice;
        uint pressed = pad->pressed;
        int prev = choice;

        if (pressed & 4) {
            choice = 0;
        }
        if (pressed & 8) {
            choice = 1;
        }
        if (choice != prev) {
            result = -3;
            if (dlg->anim >= 12) {
                dlg->anim = 12;
            }
            dlg->flash = 6;
            dlg->flags |= 0x80;
        }
        dlg->choice = choice;
        if (pressed & 0xa00) {
            result = choice;
        }
        if (pressed & 0x500) {
            result = -1;
        }
    }
    TextItem_Update_OVL2(&dlg->message);
    TextItem_Update_OVL2(&dlg->option0);
    TextItem_Update_OVL2(&dlg->option1);
    return result;
}

// The boxes and their outlines, the texts, then the highlight on the chosen box.
void Dialog_Draw_OVL2(ArcDialog *dlg, void *arg, TextCtx *ctx)
{
    uint *ot = (uint *)arg;
    int level = 0x7f;
    int offset = 0;
    int messageDy = -2;
    int optionDy = -2;
    unsigned int flags = dlg->flags;
    int anim;
    GouraudQuad quad;
    ArcTextItem *item;
    int w;

    if (flags & 8) {
        messageDy = -4;
        optionDy = -4;
    }
    anim = dlg->anim;
    if (anim < 0) {
        if (anim < -1) {
            int t = anim + 0x11;
            int b0 = 0xff - t * 0xff / 16;
            int r0 = 0;
            int b1 = 0;
            int r1 = 0;

            if (flags & 1) {
                r0 = b0;
                b0 = 0;
            }
            if (dlg->choice != 0) {
                b1 = b0;
                r1 = r0;
                r0 = 0;
                b0 = 0;
            }
            offset = dlg->boxWidth - dlg->boxWidth * t / 16;
            quad.rect.w = offset * 2;
            quad.rect.x = dlg->x - offset;
            quad.rect.y = dlg->y;
            quad.rect.h = dlg->boxHeight;
            quad.top = r0 | b0 << 16;
            quad.bottom = r1 | b1 << 16;
            Menu_AddGouraudQuad(ot, &quad);
            Prim_AddDrawMode(ot, 0x20);
            item = &dlg->message;
            w = TextItem_MeasureWidth_OVL2(item, ctx);
            item->x = dlg->x - (w >> 1);
            item->y = dlg->y + messageDy;
            TextItem_Draw_OVL2(item, ot, ctx);
        }
    } else {
        int y;
        int bottom;
        int mid;
        int left;
        int right;
        unsigned int color;
        LineF2 *l;
        int x0;
        int x1;
        int t;
        int glow;
        int bright;
        unsigned int rgb3;
        int r;
        int g;
        int b;

        if (anim < 12) {
            offset = (dlg->boxWidth * 2) * (12 - anim) / 12;
            level = anim * 0x7f / 12;
        }
        if (flags & 2) {
            offset = -offset;
        }
        color = level | level << 8 | level << 16;
        mid = dlg->x + offset;
        left = mid - dlg->boxWidth;
        y = dlg->y;
        right = mid + dlg->boxWidth;
        bottom = y + dlg->boxHeight;
        l = Prim_AddLineF2(ot, color);
        l->x0 = left;
        l->x1 = right;
        l->y0 = y;
        l->y1 = y;
        l = Prim_AddLineF2(ot, color);
        l->x0 = left;
        l->x1 = right;
        l->y0 = bottom;
        l->y1 = bottom;
        l = Prim_AddLineF2(ot, color);
        l->x0 = left;
        l->x1 = left;
        l->y0 = y;
        l->y1 = bottom;
        l = Prim_AddLineF2(ot, color);
        l->x0 = mid;
        l->x1 = mid;
        l->y0 = y;
        l->y1 = bottom;
        l = Prim_AddLineF2(ot, color);
        l->x0 = right;
        l->x1 = right;
        l->y0 = y;
        l->y1 = bottom;
        item = &dlg->message;
        w = TextItem_MeasureWidth_OVL2(item, ctx);
        item->y = y + messageDy;
        item->x = dlg->x - (w >> 1);
        TextItem_Draw_OVL2(item, ot, ctx);
        if (dlg->anim >= 0) {
            item = &dlg->option0;
            w = TextItem_MeasureWidth_OVL2(item, ctx);
            item->brightness = level;
            optionDy += bottom;
            item->y = optionDy;
            item->x = left + ((dlg->boxWidth - w) >> 1);
            TextItem_Draw_OVL2(item, ot, ctx);
            item = &dlg->option1;
            w = TextItem_MeasureWidth_OVL2(item, ctx);
            item->y = optionDy;
            item->brightness = level;
            item->x = mid + ((dlg->boxWidth - w) >> 1);
            TextItem_Draw_OVL2(item, ot, ctx);
        }
        w = dlg->boxWidth * dlg->flash / 12;
        if (dlg->choice == 0) {
            right = mid;
            x0 = right;
            x1 = right + w;
        } else {
            left = mid;
            x0 = left;
            x1 = left - w;
        }
        t = 0;
        if (dlg->anim >= 12) {
            t = 60 - (dlg->anim - 12) * 4;
            if (t < 0) {
                t = 0;
            }
        }
        b = (level * 0xff) >> 7;
        bright = 0x80;
        if (flags & 0x80) {
            bright = 0xff;
        }
        glow = bright * t / 60;
        rgb3 = b << 16;
        r = glow;
        g = glow;
        if (flags & 1) {
            r = b;
            b = g;
            rgb3 = r;
        }
        ot++;
        if (dlg->flash > 0) {
            PrimG4 *q = Prim_AddPolyG4(ot);
            unsigned int c = r | g << 8 | b << 16 | 0x3a000000;

            q->c2 = c;
            q->c0 = c;
            q->c3 = rgb3;
            q->c1 = rgb3;
            q->x2 = x0;
            q->x0 = x0;
            q->x3 = x1;
            q->x1 = x1;
            q->y1 = y;
            q->y0 = y;
            q->y3 = bottom;
            q->y2 = bottom;
        }
        {
            PrimF4 *f = (PrimF4 *)Prim_AddPolyF4(ot, r | g << 8 | b << 16 | 0x2000000);

            f->x2 = left;
            f->x0 = left;
            f->x3 = right;
            f->x1 = right;
            f->y1 = y;
            f->y0 = y;
            f->y3 = bottom;
            f->y2 = bottom;
            Prim_AddDrawMode(ot, 0x20);
        }
    }
}
}
