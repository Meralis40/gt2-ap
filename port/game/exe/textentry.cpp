// Resident code 8007306c-80073ce4 (textentry): the on-screen keyboard: a grid of
// characters drawn by a MenuList, with the entered text and its cursor above it.
#include "gptr.h"
#include "common.h"

// The part of a TextEntry its template sets.
struct TextEntryDesc {
    game_pointer<Font> font;
    char cellWidth;
    char cellHeight;
    char cellGap;
    char rowGap;
    short textPri;
    short glyphY;
    short spacing;
    short x;
    short y;
    short pad12;
};

// The grid's list, as the template has it.
struct TextEntryGrid {
    MenuList list;
    unsigned char pad[4];
};

// The cursor's row and column, read as signed bytes.
struct TextEntryCursor {
    unsigned char pad[0x1a];
    signed char row;
    signed char col;
};

// The blend mode in the texture page of a TextCtx.
struct TextCtxBlend {
    unsigned char pad[0xc];
    unsigned int : 21;
    unsigned int abr : 2;
};

#define TEXT_BLEND(ctx) (((TextCtxBlend *)(ctx))->abr)

#define ENTRY_ROW(entry) (((TextEntryCursor *)(entry))->row)
#define ENTRY_COL(entry) (((TextEntryCursor *)(entry))->col)

extern "C" {

extern char (&g_TextEntry_Charset)[]; // 16 per row, 12 rows
extern unsigned char (&D_8009232C)[4];
extern unsigned char (&D_80092330)[4];
extern unsigned char (&D_80092334)[4];
extern WipeBar (&D_80092338);
extern TextEntryGrid (&D_80092354);
extern signed char (&sB_Cancel)[]; // the text for OK is 3 bytes before it


// Callback of the grid: draws a row of 16 characters (row 12: Cancel and OK) with the
// cursor box.
// Registers: the game keeps cellWidth, brightness, the cell's x and the column in $s2-$s5,
// spills the text level and rebuilds &ctx in each branch.
int TextEntry_GridCallback(MenuListEventId event, MenuListEvent *ev, int row)
{
    TextCtx ctx;
    TextCtx *text;
    GouraudQuad box;
    unsigned int rgb;
    int textLevel;
    int gap;
    int cellH;
    MenuList *list = ev->list;
    TextEntry *entry = (TextEntry *)list->userData;
    int brightness = ev->brightness;
    uint *ot = (uint *)ev->ot;
    int cellW = (signed char)entry->cellWidth;

    gap = (signed char)entry->cellGap;
    cellH = (signed char)entry->cellHeight;
    Text_Init(&ctx, entry->textPri);
    Text_SetFont(&ctx, entry->font);
    text = &ctx;
    text->ot = ot;
    TEXT_BLEND(text) = 1;
    if (event == MLE_DRAW) {
        int open = list->state >= 0;

        if (open | (list->state < -1)) {
            int fade = (list->fade << 7) / list->fadeMax;
            int level = fade * brightness >> 7;

            if (level > 0) {
                int pitch;
                int total;
                int x;
                int y;
                int col;

                textLevel = level * 9 >> 4;
                if (open) {
                    rgb = Rgb_Lerp(D_80092330, D_8009232C, fade, 0x80);
                } else {
                    rgb = Rgb_Lerp(D_80092334, D_8009232C, fade, 0x80);
                }
                rgb = Rgb_Lerp(D_80092334, (unsigned char *)&rgb, brightness, 0x80);
                box.top = (level >> 1) | 0x2000000;
                box.bottom = level | level << 8 | level << 16 | 0x2000000;
                pitch = cellW + gap;
                total = pitch << 3;
                x = ev->x - total + (pitch >> 1);
                y = ev->y - (cellH >> 1);
                col = 0;
                if (row == 12) {
                    int half;
                    int w;
                    unsigned int color;
                    signed char *cancel;
                    RECT *tile;

                    x -= cellW >> 1;
                    w = total - gap;
                    if (ENTRY_ROW(entry) == row) {
                        box.rect.h = cellH;
                        box.rect.y = y;
                        box.rect.w = w;
                        box.rect.x = x + w + gap + (w >> 1);
                        if (ENTRY_COL(entry) < 8) {
                            box.rect.x = x + (w >> 1);
                        }
                        Menu_DrawCursorBox(ot, &box, entry->blinkTimer * 0x80 / 60);
                        Prim_AddDrawMode(ot, 0x20);
                    }
                    text = &ctx;
                    cancel = sB_Cancel;
                    half = w >> 1;
                    color = textLevel | textLevel << 8 | textLevel << 16 | 0x2000000;
                    text->rgb = color;
                    Text_DrawCentered(text, cancel, x + half, y + entry->glyphY, entry->spacing);
                    tile = Prim_AddTile(ot, rgb);
                    tile->x = x;
                    tile->y = y;
                    tile->w = w;
                    tile->h = cellH;
                    Prim_AddDrawMode(ot, 0x220);
                    x = ev->x;
                    text->rgb = color;
                    Text_DrawCentered(text, cancel - 3, x + half, y + entry->glyphY, entry->spacing);
                    tile = Prim_AddTile(ot, rgb);
                    tile->x = x;
                    tile->y = y;
                    tile->w = w;
                    tile->h = cellH;
                    Prim_AddDrawMode(ot, 0x220);
                } else {
                    unsigned int color = textLevel | textLevel << 8 | textLevel << 16 | 0x2000000;
                    char *charset = g_TextEntry_Charset;

                    text = &ctx;
                    while (col < 16) {
                        RECT *tile;

                        if ((row == ENTRY_ROW(entry)) & (col == ENTRY_COL(entry))) {
                            box.rect.x = x;
                            box.rect.y = y;
                            box.rect.w = cellW;
                            box.rect.h = cellH;
                            Menu_DrawCursorBox(ot, &box, entry->blinkTimer * 0x80 / 60);
                            Prim_AddDrawMode(ot, 0x20);
                        }
                        text->rgb = color;
                        Text_DrawGlyph(text, (unsigned char)charset[col + row * 16] | 0x200, x,
                                       y + entry->glyphY);
                        col++;
                        tile = Prim_AddTile(ot, rgb);
                        tile->x = x - (cellW >> 1);
                        tile->y = y;
                        tile->w = cellW;
                        x += cellW + gap;
                        tile->h = cellH;
                        Prim_AddDrawMode(ot, 0x220);
                    }
                }
            }
        }
    }
    return 1;
}

// Removes s[index], moving the rest of the string down.
void Str_DeleteChar(char *s, int index)
{
    signed char c;

    s += index;
    do {
        c = s[1];
        *s++ = c;
    } while (c != 0);
}

// Copies a template, resets the cursor and the wipe bar and builds the character grid.
void TextEntry_Init(TextEntry *entry, TextEntry *desc)
{
    *(TextEntryDesc *)entry = *(TextEntryDesc *)desc;
    entry->anim = -1;
    entry->col = 0;
    entry->row = 0;
    entry->cursorPos = 0;
    entry->cursorTimer = 0;
    entry->blinkTimer = 0;
    entry->bar = D_80092338;
    entry->bar.t = -1;
    *(TextEntryGrid *)&entry->list = D_80092354;
    MenuList_Init(&entry->list, TextEntry_GridCallback, entry);
}

// Shows the entry with the cursor at the start and opens the character grid.
void TextEntry_Open(TextEntry *entry)
{
    MenuList *list = &entry->list;
    int row;

    entry->row = 0;
    entry->bar.width = entry->maxWidth;
    row = ENTRY_ROW(entry);
    entry->anim = 0;
    entry->col = 0;
    entry->cursorPos = 0;
    entry->cursorTimer = 0;
    entry->blinkTimer = 0;
    entry->bar.t = 0;
    list->selected = row;
    entry->list.itemHeight = (signed char)entry->cellHeight;
    entry->list.itemGap = (signed char)entry->rowGap;
    entry->list.x = entry->x;
    entry->list.y = entry->y + (signed char)entry->cellHeight - 8;
    MenuList_Open(list);
}

// Hides the entry and closes the grid.
void TextEntry_Close(TextEntry *entry)
{
    WipeBar *bar = &entry->bar;

    bar->t = ~bar->duration;
    MenuList_Close(&entry->list);
    entry->anim = -1;
}

// The grid's input: the D-pad moves (row 12 is Cancel and OK), L1 and R1 move the text
// cursor, Cross and Circle insert the character if it fits maxLen and maxWidth, Square and
// Triangle delete, Start jumps to OK. Returns 0 for OK, -1 to cancel, -2 for nothing, -3
// when the cursor moved.
int TextEntry_Update(TextEntry *entry, PadState *pad)
{
    TextCtx ctx;
    int result = -2;
    MenuList *list = &entry->list;

    MenuList_Update(list, pad);
    WipeBar_Update(&entry->bar);
    if (entry->anim < 0) {
        if (entry->anim < -1) {
            entry->anim++;
        }
        return -2;
    }
    Text_SetFont(&ctx, entry->font);
    if (++entry->anim > 12) {
        entry->anim = 12;
    }
    if (++entry->blinkTimer > 60) {
        entry->blinkTimer = 0;
    }
    if (++entry->cursorTimer > 40) {
        entry->cursorTimer = 0;
    }
    if (pad != 0) {
        PadButton buttons = pad->pressed;
        PadButton repeat = pad->repeat;
        PadButton pressed = buttons;

        buttons |= repeat;
        if (buttons & (PAD_SQUARE | PAD_TRIANGLE)) {
            buttons = pressed;
            if (entry->cursorPos > 0) {
                Sfx_PlayMenu(2);
                entry->cursorPos--;
                Str_DeleteChar(entry->text, entry->cursorPos);
            } else {
                Sfx_PlayMenu(0);
            }
        } else {
            buttons = pressed;
            if (buttons & (PAD_CIRCLE | PAD_CROSS)) {
                if (ENTRY_ROW(entry) == 12) {
                    result = 0;
                    if (ENTRY_COL(entry) < 8) {
                        result = -1;
                    }
                } else {
                    int len = strlen(entry->text);

                    if (len >= entry->maxLen) {
                        Sfx_PlayMenu(2);
                    } else {
                        char *p = entry->text + len;

                        if (len >= entry->cursorPos) {
                            do {
                                p[1] = *p;
                                len--;
                                p--;
                            } while (len >= entry->cursorPos);
                        }
                        entry->text[entry->cursorPos] = g_TextEntry_Charset[ENTRY_COL(entry) + ENTRY_ROW(entry) * 16];
                        if (Text_MeasureString(&ctx, (signed char *)entry->text, entry->spacing) >= entry->maxWidth) {
                            Str_DeleteChar(entry->text, entry->cursorPos);
                            Sfx_PlayMenu(0);
                        } else {
                            Sfx_PlayMenu(1);
                            entry->cursorPos++;
                        }
                    }
                }
            } else if (buttons & PAD_START) {
                int row = 12;

                entry->row = row;
                MenuList_SetSelection(list, row);
                entry->col = 8;
                Sfx_PlayMenu(5);
                result = -3;
            } else {
                buttons |= repeat;
                if ((buttons & PAD_L1) && --entry->cursorPos < 0) {
                    entry->cursorPos = 0;
                }
                if (buttons & PAD_R1) {
                    int len;

                    entry->cursorPos++;
                    len = strlen(entry->text);
                    if (entry->cursorPos > len) {
                        entry->cursorPos = len;
                    }
                }
                {
                    int row = entry->list.selected;

                    entry->row = row;
                }
                if (buttons & PAD_LEFT) {
                    if (--ENTRY_COL(entry) < 0) {
                        entry->col = 15;
                    }
                    if (ENTRY_ROW(entry) == 12) {
                        entry->col = 7;
                    }
                }
                if (buttons & PAD_RIGHT) {
                    if (++ENTRY_COL(entry) > 15) {
                        entry->col = 0;
                    }
                    if (ENTRY_ROW(entry) == 12) {
                        entry->col = 8;
                    }
                }
                if (buttons & (PAD_R1 | PAD_L1 | PAD_RIGHT | PAD_LEFT | PAD_DOWN | PAD_UP)) {
                    Sfx_PlayMenu(5);
                    result = -3;
                }
            }
        }
    }
    return result;
}

// Draws the grid, the text (twice, the second additive), its wipe bar and the cursor arrow.
void TextEntry_Draw(TextEntry *entry, uint *ot)
{
    TextCtx ctx;
    int arrow[2];
    int anim;

    MenuList_Draw(&entry->list, ot);
    anim = entry->anim;
    if (anim != -1) {
        int level = anim;
        int x;
        TextCtx *text;
        unsigned int color;
        char *p;
        signed char c;
        int width;

        if (level < 0) {
            level = ~level;
        }
        level = level * 80 / 12;
        x = entry->x - (entry->maxWidth >> 1);
        Text_Init(&ctx, entry->textPri);
        Text_SetFont(&ctx, entry->font);
        text = &ctx;
        color = level >> 1 | level << 8 | level << 16 | 0x2000000;
        text->ot = ot;
        text->rgb = color;
        TEXT_BLEND(text) = 1;
        Text_DrawString(text, (signed char *)entry->text, x, entry->y, entry->spacing);
        text->rgb = color;
        TEXT_BLEND(text) = 2;
        Text_DrawString(text, (signed char *)entry->text, x, entry->y, entry->spacing);
        WipeBar_Draw(&entry->bar, ot, x, entry->y - 6);
        Prim_AddDrawMode(ot, 0x220);
        p = entry->text + entry->cursorPos;
        c = *p;
        *p = 0;
        width = Text_MeasureString(text, (signed char *)entry->text, entry->spacing);
        *p = c;
        Menu_DrawScrollArrow(arrow, ot, x + width, entry->y + 10, 4, -7, entry->cursorTimer);
        Prim_AddDrawMode(ot, 0x20);
    }
}

}
