// Resident code 8006dcb8-8006e1cc (buttonprompt): a box with one message,
// confirmed or cancelled with a button (used instead of the save dialog when
// saving is not offered). It opens as a fade and closes as a horizontal wipe.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

// Pad bits of PadState.pressed.
#define PAD_CONFIRM 0xa00
#define PAD_CANCEL 0x500

extern "C" {
extern TextStyle D_80091EA8;    // the message's style
extern uint D_80091EC4;         // the colour boxes fade from and to

void TextItem_Init(TextItem *item, TextStyle *style, char *text);
void TextItem_Hide(TextItem *item);
void TextItem_Show(TextItem *item, int endFrame);
void TextItem_Update(TextItem *item);
void TextItem_Draw(TextItem *item, uint *ot, int x, int y, TextCtx *ctx);
uint Rgb_Lerp(uchar *from, uchar *to, int t, int range);
void Menu_AddGouraudQuad(uint *ot, GouraudQuad *quad);
void Prim_AddRectOutline(uint *ot, uint rgb, RECT *rect);
void *Prim_AddDrawMode(uint *ot, uint tpage);
RECT *Prim_AddTile(uint *ot, uint rgb);

void ButtonPrompt_Init(ButtonPrompt *p, ButtonPromptDesc *desc)
{
    short width;
    short height;
    TextItem *text;

    p->x = desc->x;
    p->y = desc->y;
    switch (desc->style & 6) {
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
    p->width = width;
    p->height = height;
    D_80091EA8.font = desc->font;
    D_80091EA8.height = desc->textHeight;
    D_80091EA8.rgbFrom = desc->textRgb;
    text = &p->text;
    TextItem_Init(text, &D_80091EA8, desc->text);
    int spacing = desc->spacing;    // an int: loaded with lb, as in the original
    text->spacing = spacing;
    p->flags = desc->style;
    p->color = desc->color;
    p->color2 = desc->color2;
    p->anim = -1;
}

void ButtonPrompt_Open(void *p)
{
    TextItem_Show((TextItem *)((int)p + 4), -1);
    *(unsigned short *)((int)p + 0x32) = 0;
    *(unsigned short *)((int)p + 0x2c) = *(unsigned short *)((int)p + 0x2c) | 0x80;
}

void FUN_8006de0c(int param_1)
{
    *(short *)(param_1 + 0x32) = 0xffff;
    *(short *)(param_1 + 0x26) = 0xffff;
}

void ButtonPrompt_Close(void *p)
{
    *(short *)((int)p + 0x32) = 0xfff3;
    TextItem_Hide((TextItem *)((int)p + 4));
}

// Returns 0 when confirmed, -1 when cancelled, -2 otherwise (and while
// closed or closing).
int ButtonPrompt_Update(ButtonPrompt *p, PadState *pad)
{
    int result = -2;

    if (p->anim < 0) {
        if (p->anim < -1) {
            p->anim++;
        }
        return -2;
    }
    if (++p->anim >= 0x48) {
        p->anim = 12;
        p->flags &= ~0x80;
    }
    if (pad != 0) {
        uint pressed = pad->pressed;

        if (pressed & PAD_CONFIRM) {
            result = 0;
        }
        if (pressed & PAD_CANCEL) {
            result = -1;
        }
    }
    TextItem_Update(&p->text);
    return result;
}

void ButtonPrompt_Draw(ButtonPrompt *p, uint *ot, TextCtx *ctx)
{
    uint level = 0x7f;
    uint *back = ot + 2;
    int textDy = -2;
    ushort flags = p->flags;
    int left = p->x - (p->width >> 1);
    int anim;
    uint color;
    uint white;
    RECT outline;
    GouraudQuad quad;

    if (flags & 4) {
        textDy = -4;
    }
    anim = p->anim;
    if (anim < 0) {
        int width;

        if (anim >= -1) {
            return;
        }
        anim += 13;
        width = p->width;
        width -= width * anim / 12;
        color = Rgb_Lerp((uchar *)&p->color, (uchar *)&D_80091EC4, anim, 12);
        quad.rect.x = p->x - (width >> 1);
        quad.rect.w = width;
        quad.rect.y = p->y;
        quad.rect.h = p->height;
        quad.top = color;
        quad.bottom = color;
        Menu_AddGouraudQuad(ot, &quad);
        Prim_AddDrawMode(ot, 0x20);
    } else {
        int y;
        int t;

        if (anim < 12) {
            level = anim * 0x7f / 12;
        }
        y = p->y + p->height;
        outline.x = quad.rect.x = left;
        outline.y = quad.rect.y = p->y;
        outline.w = quad.rect.w = p->width;
        outline.h = quad.rect.h = p->height;
        color = level | level << 8 | level << 16;
        Prim_AddRectOutline(ot, color, &outline);
        quad.top = p->color2;
        quad.bottom = D_80091EC4;
        Menu_AddGouraudQuad(back, &quad);
        Prim_AddDrawMode(ot, 0x200);
        TextItem *text = &p->text;
        text->brightness = level;
        TextItem_Draw(text, ot, left + (p->width >> 1), y + textDy, ctx);
        ot++;
        t = 0;
        if (p->anim >= 12) {
            t = 60 - (p->anim - 12) * 4;
            if (t < 0) {
                t = 0;
            }
        }
        level = 0x80;
        if (flags & 0x80) {
            level = 0xff;
        }
        white = level | level << 8 | level << 16;
        color = Rgb_Lerp((uchar *)&p->color, (uchar *)&white, t, 60);
        t = 12 - p->anim;
        if (t < 0) {
            t = 0;
        }
        color = Rgb_Lerp((uchar *)&color, (uchar *)&D_80091EC4, t, 12);
        RECT *tile = Prim_AddTile(ot, color);
        tile->x = left;
        tile->y = p->y;
        tile->w = p->width;
        tile->h = p->height;
        Prim_AddDrawMode(ot, 0x220);
    }
}
}
