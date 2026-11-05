// Resident code 8006e1cc-8006ebf0 (carddialog): the memory card dialogs, a
// message over two option boxes picked with left and right. They open by
// sliding the boxes in and close as a wipe of the chosen box. Also the card
// slot status and a scaled sprite helper. CardSlots_Update's jump table is
// this file's .rodata (0x8008fab0).
#include "common.h"

// Pad bits of PadState.pressed.
#define PAD_LEFT 0x4
#define PAD_RIGHT 0x8
#define PAD_CONFIRM 0xa00
#define PAD_CANCEL 0x500

extern "C" {
extern TextStyle D_80091EC8;    // the dialog's text style
extern uint D_80091EE4;         // the colour boxes fade from and to
extern CardSystem g_Card;

void CardDialog_Init(CardDialog *dlg, CardDialogDesc *desc, int choice)
{
    short width;
    short height;
    short textHeight = desc->textHeight;
    Font *font = desc->font;
    TextItem *option0;
    TextItem *option1;

    dlg->x = desc->x;
    dlg->y = desc->y;
    switch (desc->flags & 6) {
    case 0:
        width = 0x50;
        height = 0xc;
        break;
    case 4:
        width = 0x60;
        height = 0x18;
        break;
    case 2:
        width = 0x60;
        height = 0xc;
        break;
    case 6:
        width = 0x80;
        height = 0x18;
        break;
    default:
        width = 0x50;
        height = 0xc;
        break;
    }
    dlg->boxHeight = height;
    dlg->boxWidth = width;
    D_80091EC8.font = font;
    D_80091EC8.height = textHeight;
    D_80091EC8.rgbFrom = desc->messageRgb;
    D_80091EC8.flags = 200;
    if (desc->messageRgb & 0x2000000) {
        D_80091EC8.flags = 0xe9;
    }
    TextItem_Init(&dlg->message, &D_80091EC8, desc->message);
    D_80091EC8.rgbFrom = desc->optionRgb;
    D_80091EC8.flags = 0xc0;
    if (desc->optionRgb & 0x2000000) {
        D_80091EC8.flags = 0xe1;
    }
    option0 = &dlg->option0;
    TextItem_Init(option0, &D_80091EC8, desc->option0);
    int spacing0 = desc->spacing;    // ints: loaded with lb, as in the original
    option0->spacing = spacing0;
    option1 = &dlg->option1;
    TextItem_Init(option1, &D_80091EC8, desc->option1);
    int spacing1 = desc->spacing;
    option1->spacing = spacing1;
    dlg->sfx = desc->sfx;
    dlg->choice = choice != 0;
    dlg->flags = desc->flags;
    dlg->color = desc->color;
    dlg->color2 = desc->color2;
    dlg->anim = -1;
    dlg->flash = 0;
}

void CardDialog_Open(CardDialog *dlg)
{
    TextItem_Show(&dlg->message, 0x3c);
    TextItem_Show(&dlg->option0, -1);
    TextItem_Show(&dlg->option1, -1);
    dlg->anim = 0;
    dlg->flash = 0;
    dlg->flags = dlg->flags | 0x80;
}

void FUN_8006e3e0(int param_1)
{
    *(short *)(param_1 + 0x82) = 0xffff;
    *(short *)(param_1 + 0x84) = 0;
    *(short *)(param_1 + 0x26) = 0xffff;
    *(short *)(param_1 + 0x4e) = 0xffff;
    *(short *)(param_1 + 0x76) = 0xffff;
}

void CardDialog_Close(CardDialog *dlg)
{
    dlg->anim = -0x11;
    TextItem_Hide(&dlg->message);
    TextItem_Hide(&dlg->option0);
    TextItem_Hide(&dlg->option1);
}

// Returns the choice when confirmed, -1 when cancelled, -3 when the choice
// moved and -2 otherwise (and while closed or closing).
int CardDialog_Update(CardDialog *dlg, PadState *pad)
{
    int result = -2;

    if (dlg->anim < 0) {
        if (dlg->anim < -1) {
            dlg->anim++;
        }
        TextItem_Update(&dlg->message);
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
        uint pressed = pad->pressed;
        int choice = dlg->choice;

        if (pressed & PAD_CANCEL) {
            result = -1;
            if (dlg->flags & 8) {
                CardDialog_Close(dlg);
            }
        } else if (pressed & PAD_CONFIRM) {
            result = choice;
            if (dlg->flags & 8) {
                CardDialog_Close(dlg);
            }
        } else {
            int prev = choice;

            if (pressed & PAD_LEFT) {
                choice = 0;
            }
            if (pressed & PAD_RIGHT) {
                choice = 1;
            }
            if (choice != prev) {
                result = -3;
                if (dlg->anim >= 12) {
                    dlg->anim = 12;
                }
                dlg->flash = 6;
                dlg->flags |= 0x80;
                if (dlg->sfx >= 0) {
                    Sfx_PlayMenu(dlg->sfx);
                }
            }
            dlg->choice = choice != 0;
        }
    }
    TextItem_Update(&dlg->message);
    TextItem_Update(&dlg->option0);
    TextItem_Update(&dlg->option1);
    return result;
}

void CardDialog_Draw(CardDialog *dlg, uint *ot, TextCtx *ctx)
{
    uint level = 0x7f;
    int offset = 0;             // also the box width while closing, then the flash width
    int messageDy = -2;
    int optionDy = -2;
    ushort flags = dlg->flags;
    uint *back = ot + 2;
    int anim;
    int t;                      // animation steps; one variable for the whole function
    int left;                   // also the box centre while closing
    int right;
    int y;
    uint color;
    uint white;
    RECT outline;
    GouraudQuad quad;

    if (flags & 4) {
        messageDy = -4;
        optionDy = -4;
    }
    anim = dlg->anim;
    if (anim < 0) {
        t = anim + 17;
        if (anim < -1) {
            left = dlg->x + (dlg->boxWidth >> 1);
            if (dlg->choice == 0) {
                left -= dlg->boxWidth;
            }
            offset = dlg->boxWidth - dlg->boxWidth * (anim + 17) / 16;
            color = Rgb_Lerp((uchar *)&dlg->color, (uchar *)&D_80091EE4, t, 16);
            quad.rect.x = left - (offset >> 1);
            quad.rect.w = offset;
            quad.rect.y = dlg->y;
            quad.rect.h = dlg->boxHeight;
            quad.top = color;
            quad.bottom = color;
            Menu_AddGouraudQuad(ot, &quad);
            Prim_AddDrawMode(ot, 0x20);
            TextItem_Draw(&dlg->message, ot, dlg->x, dlg->y + messageDy, ctx);
        }
    } else {
        int textY;

        if (anim < 12) {
            t = 12 - anim;
            offset = (dlg->boxWidth << 1) * t / 12;
            level = anim * 0x7f / 12;
        }
        if (flags & 1) {
            offset = -offset;
        }
        color = level | level << 8 | level << 16;
        right = dlg->x + offset;
        left = right - dlg->boxWidth;
        y = dlg->y;
        textY = y + dlg->boxHeight;
        outline.y = quad.rect.y = y;
        outline.w = quad.rect.w = dlg->boxWidth;
        outline.h = quad.rect.h = dlg->boxHeight;
        outline.x = left;
        Prim_AddRectOutline(ot, color, &outline);
        outline.x = right;
        Prim_AddRectOutline(ot, color, &outline);
        quad.top = dlg->color2;
        quad.bottom = D_80091EE4;
        quad.rect.x = left;
        Menu_AddGouraudQuad(back, &quad);
        quad.rect.x = right;
        Menu_AddGouraudQuad(back, &quad);
        Prim_AddDrawMode(ot, 0x200);
        TextItem_Draw(&dlg->message, ot, dlg->x, y + messageDy, ctx);
        TextItem *option = &dlg->option0;
        option->brightness = level;
        TextItem_Draw(option, ot, left + (dlg->boxWidth >> 1), textY + optionDy, ctx);
        option = &dlg->option1;
        option->brightness = level;
        TextItem_Draw(option, ot, right + (dlg->boxWidth >> 1), textY + optionDy, ctx);
        ot++;
        t = 0;
        if (dlg->anim >= 12) {
            t = 60 - (dlg->anim - 12) * 4;
            if (t < 0) {
                t = 0;
            }
        }
        level = 0x80;
        if (flags & 0x80) {
            level = 0xff;
        }
        white = level | level << 8 | level << 16;
        color = Rgb_Lerp((uchar *)&dlg->color, (uchar *)&white, t, 60);
        t = 12 - dlg->anim;
        if (t < 0) {
            t = 0;
        }
        color = Rgb_Lerp((uchar *)&color, (uchar *)&D_80091EE4, t, 12);
        RECT *tile = Prim_AddTile(ot, color);
        tile->x = left;
        if (dlg->choice != 0) {
            tile->x = right;
        }
        tile->y = y;
        tile->w = dlg->boxWidth;
        tile->h = dlg->boxHeight;
        offset = dlg->boxWidth * dlg->flash / 6;
        if (dlg->flash > 0) {
            quad.rect.y = y;
            quad.rect.w = offset;
            quad.rect.h = dlg->boxHeight;
            quad.rect.x = right;
            quad.top = color;
            quad.bottom = D_80091EE4;
            if (dlg->choice != 0) {
                quad.rect.x = right - offset;
                quad.top = D_80091EE4;
                quad.bottom = color;
            }
            Menu_AddGouraudQuad(ot, &quad);
        }
        Prim_AddDrawMode(ot, 0x220);
    }
}

// Adds a sprite scaled by scale / 256 around (x, y) as a textured quad.
void FUN_8006ea38(uint *ot, Sprite *sprite, int x, int y, uint rgb, int scale)
{
    int w = sprite->w * scale;
    int h = sprite->h * scale;
    PrimFT4 *p = Prim_AddPolyFT4(ot, rgb);
    int left = x - (w >> 8);
    int right = x + (w >> 8);
    int top = y - (h >> 8);
    int bottom = y + (h >> 8);

    p->x0 = p->x2 = left;
    p->x1 = p->x3 = right;
    p->y0 = p->y1 = top;
    p->y2 = p->y3 = bottom;
    p->u0 = p->u2 = sprite->u;
    p->u1 = p->u3 = sprite->u + sprite->w - 1;
    p->v0 = p->v1 = sprite->v;
    p->v2 = p->v3 = sprite->v + sprite->h - 1;
    p->clut = sprite->clut;
    p->tpage = sprite->tpage;
}

void CardSlots_Init(short *status)
{
    *status = 2;
    status[1] = 2;
}

// Each frame: the menu status of each card slot from its driver state. Other
// states keep the previous status.
void CardSlots_Update(short *status)
{
    int i;

    for (i = 0; i < 2; i++) {
        int slot = *status;
        McPort *port = &Card_Get()->ports[i];

        switch (port->state) {
        case 0:
            slot = 0;
            break;
        case 2:
            slot = 1;
            break;
        case 3:
            slot = 3;
            break;
        case 4:
            slot = 2;
            break;
        case 5:
            slot = 4;
            break;
        }
        *status++ = slot;
    }
}
}
