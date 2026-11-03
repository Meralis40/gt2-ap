// OVL2 8001b37c-8001c118 (menuitem): Menu items and lists: another version of the
// resident MenuList, whose rows are text or icon items.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {

// Initialises the item widget from its template and data (the text or the sprite).
void MenuItem_Init_OVL2(ArcMenuItem *it)
{
    switch (it->kind & 1) {
    case 0: {
        ArcTextItem *item = (ArcTextItem *)it->item;
        TextItem_Init_OVL2(item, it->tmpl);
        item->text = (char *)it->data;
        break;
    }
    case 1: {
        ArcIconItem *icon = (ArcIconItem *)it->item;
        IconItem_Init(icon, it->tmpl);
        icon->sprite = it->data;
        break;
    }
    }
}

// Text: blink 0x3c / fade 0x78 then TextItem_Show; icon: IconItem_Show.
void MenuItem_Show_OVL2(ArcMenuItem *it)
{
    switch (it->kind & 1) {
    case 0: {
        ArcTextItem *item = (ArcTextItem *)it->item;
        item->blinkSteps = 0x3c;
        item->fadeSteps = 0x78;
        TextItem_Show_OVL2(item, -1);
        break;
    }
    case 1:
        IconItem_Show((ArcIconItem *)it->item);
        break;
    }
}

void MenuItem_Update_OVL2(ArcMenuItem *it)
{
    switch (it->kind & 1) {
    case 0:
        TextItem_Update_OVL2((ArcTextItem *)it->item);
        break;
    case 1:
        IconItem_Update((ArcIconItem *)it->item);
        break;
    }
}

void MenuItem_Hide_OVL2(ArcMenuItem *it)
{
    switch (it->kind & 1) {
    case 0:
        TextItem_Hide_OVL2((ArcTextItem *)it->item);
        break;
    case 1:
        IconItem_Hide((ArcIconItem *)it->item);
        break;
    }
}

void MenuItem_Draw_OVL2(ArcMenuItem *it, void *ot, TextCtx *ctx)
{
    switch (it->kind & 1) {
    case 0:
        TextItem_Draw_OVL2((ArcTextItem *)it->item, ot, ctx);
        break;
    case 1:
        IconItem_Draw((ArcIconItem *)it->item, ot);
        break;
    }
}

void MenuItem_SetPos_OVL2(ArcMenuItem *it, short x, short y)
{
    switch (it->kind & 1) {
    case 0: {
        ArcTextItem *item = (ArcTextItem *)it->item;
        item->x = x;
        item->y = y;
        break;
    }
    case 1: {
        ArcIconItem *icon = (ArcIconItem *)it->item;
        icon->x = x;
        icon->y = y;
        break;
    }
    }
}

void MenuItem_SetBrightness_OVL2(ArcMenuItem *it, unsigned char brightness)
{
    switch (it->kind & 1) {
    case 0:
        ((ArcTextItem *)it->item)->brightness = brightness;
        break;
    case 1:
        ((ArcIconItem *)it->item)->brightness = brightness;
        break;
    }
}

// Selected item: the text blinks (flags 8), the icon pulses (flags 1).
void MenuItem_Focus_OVL2(ArcMenuItem *it)
{
    switch ((unsigned char)it->kind) {
    case 0: {
        ArcTextItem *item = (ArcTextItem *)it->item;
        item->flags |= 8;
        TextItem_ShowNow_OVL2(item);
        break;
    }
    case 1: {
        ArcIconItem *icon = (ArcIconItem *)it->item;
        icon->flags |= 1;
        IconItem_ShowNow(icon);
        break;
    }
    }
}

// Clears the blink / pulse flag.
void MenuItem_Unfocus_OVL2(ArcMenuItem *it)
{
    switch (it->kind & 1) {
    case 0:
        ((ArcTextItem *)it->item)->flags &= 0xf7;
        break;
    case 1:
        ((ArcIconItem *)it->item)->flags &= 0xfe;
        break;
    }
}

// Resets the selection, scroll and reveal state and initialises every item.
void MenuList_Init_OVL2(ArcMenuList *list)
{
    int i;

    list->state = -1;
    list->selected = 0;
    list->blinkTimer = 0;
    list->scrollAnim = 0;
    list->revealIndex = 0;
    list->revealTimer = 6;
    for (i = 0; i < list->count; i++) {
        MenuItem_Init_OVL2(&list->items[i]);
    }
}

// Shows the items above the selection at once and reveals the rest one every 6 frames.
void MenuList_Open_OVL2(ArcMenuList *list)
{
    int i;
    int first = list->selected - list->visibleRows;

    list->state = 0;
    if (first < 0) {
        first = 0;
    }
    for (i = 0; i < first; i++) {
        MenuItem_Show_OVL2(&list->items[i]);
    }
    list->revealTimer = 6;
    list->revealIndex = first;
    list->blinkTimer = 0x1e;
}

// Starts the close animation (state -0x41) and hides every item but the selected one
// (hidden later, at -0x3a).
void MenuList_Close_OVL2(ArcMenuList *list)
{
    int i;

    list->state = -0x41;
    for (i = 0; i < list->count; i++) {
        if (i != list->selected) {
            MenuItem_Hide_OVL2(&list->items[i]);
        }
    }
}

// Updates the items and animations; Up/Down move the selection (wrapping with flags & 4)
// and return -3, Cross/Circle the item's value (-4 when disabled), Triangle/Square -1,
// else -2.
int MenuList_Update_OVL2(ArcMenuList *list, PadState *pad)
{
    int i;
    int sel;
    int result = -2;
    unsigned int buttons;

    for (i = 0; i < list->count; i++) {
        MenuItem_Update_OVL2(&list->items[i]);
    }
    if (list->state < 0) {
        if (list->state != -1) {
            list->state++;
            if (list->state == -0x3a) {
                MenuItem_Hide_OVL2(&list->items[list->selected]);
            }
        }
        return result;
    }
    if (list->scrollAnim < 0) {
        list->scrollAnim++;
    }
    if (list->scrollAnim > 0) {
        list->scrollAnim--;
    }
    if (++list->blinkTimer > 0x3c) {
        list->blinkTimer = 0;
    }
    if (++list->state > 0x2d) {
        list->state = 0;
    }
    if (pad != 0) {
        buttons = pad->pressed;
        if (buttons & (PAD_SQUARE | PAD_TRIANGLE)) {
            result = -1;
        } else {
            sel = list->selected;
            if (buttons & (PAD_CIRCLE | PAD_CROSS)) {
                ArcMenuItem *it = list->items + sel;
                result = -4;
                // The game tests the sign bit as (kind >> 7) ^ 1.
                if (((unsigned char)it->kind >> 7 ^ 1) != 0) {
                    result = it->value;
                }
            } else {
                buttons |= pad->repeat;
                if (buttons & PAD_UP) {
                    MenuItem_Unfocus_OVL2(&list->items[sel]);
                    sel--;
                    list->scrollAnim = -8;
                    if (sel < 0) {
                        sel = 0;
                        if (list->flags & 4) {
                            sel = list->count - 1;
                        }
                    }
                    result = -3;
                    MenuItem_Focus_OVL2(&list->items[sel]);
                    list->blinkTimer = 0;
                }
                if (buttons & PAD_DOWN) {
                    MenuItem_Unfocus_OVL2(&list->items[sel]);
                    sel++;
                    list->scrollAnim = 8;
                    if (sel >= list->count) {
                        sel = list->count - 1;
                        if (list->flags & 4) {
                            sel = 0;
                        }
                    }
                    result = -3;
                    MenuItem_Focus_OVL2(&list->items[sel]);
                    list->blinkTimer = 0;
                }
                list->selected = sel;
            }
        }
    }
    if (list->revealIndex < list->count && list->revealTimer-- == 1) {
        MenuItem_Show_OVL2(&list->items[list->revealIndex]);
        list->revealTimer = 6;
        list->revealIndex++;
    }
    return result;
}

// Draws the visible rows with the scroll animation (disabled items at quarter
// brightness), the cursor box behind the selected icon item and the up/down arrows.
// The game keeps fadeOut on the stack and the scroll offset in s3; this version
// allocates them differently.
#ifdef NON_MATCHING
void MenuList_Draw_OVL2(ArcMenuList *list, void *ot, TextCtx *ctx)
{
    int rowH = list->itemGap + list->itemHeight;
    int y = list->y + list->scrollAnim * rowH / 8;
    int fadeOut = -1;
    int fadeIn = -1;
    int rows = list->visibleRows;
    int end = rows;
    int sel = list->selected;
    int half = rows >> 1;
    int after = rows - half;
    int fadeB = list->scrollAnim * 0x80 / 8;
    int fadeA;
    int first;
    int i;
    int off;

    if (fadeB < 0) {
        fadeB = -fadeB;
    }
    fadeA = 0x80 - fadeB;
    first = sel - half;
    if (first + rows >= list->count) {
        first = list->count - rows;
    }
    if (first < 0) {
        first = 0;
    }
    if (list->scrollAnim < 0) {
        fadeOut = sel - half;
        fadeIn = sel + after;
        if (fadeOut >= list->count - rows) {
            fadeOut = -1;
        }
        if (fadeIn < rows) {
            fadeIn = -1;
        }
        end = rows + 1;
        if (sel == list->count - 1) {
            fadeOut = list->count - rows;
            if (rows >= list->count) {
                y = list->y;
                end = rows;
                fadeOut = -1;
            }
        } else if (sel < half || sel >= list->count - after) {
            end = rows;
            y = list->y;
        }
    }
    if (list->scrollAnim > 0) {
        fadeOut = sel + after - 1;
        fadeIn = sel - half - 1;
        if (fadeOut < rows) {
            fadeOut = -1;
        }
        if (fadeIn >= list->count - rows) {
            fadeIn = -1;
        }
        end++;
        first--;
        y -= rowH;
        if (sel == 0) {
            first = -1;
            end = rows + 1;
            fadeOut = rows + first;
            if (rows >= list->count) {
                fadeOut = first;
                y = list->y;
                first = 0;
                end = rows;
            }
        } else if (sel <= half || sel > list->count - after) {
            first++;
            end = rows;
            y = list->y;
        }
    }
    off = first * sizeof(ArcMenuItem);
    end += first;
    for (i = first; i < end; i++, off += sizeof(ArcMenuItem)) {
        if ((i >= 0) & (i < list->count)) {
            ArcMenuItem *it = (ArcMenuItem *)((char *)list->items + off);
            int h = it->height;
            int b;

            if (h <= 0) {
                h = list->itemHeight;
            }
            if (((i == sel) & (list->state >= 0)) && (it->kind & 1) == 1) {
                GouraudQuad box;

                box.rect.x = list->x;
                box.rect.y = y;
                box.rect.w = list->cursorWidth;
                box.rect.h = h;
                box.top = 0x7071e;
                box.bottom = 0x17175e;
                if (((unsigned char)it->kind >> 7 ^ 1) != 0) {
                    box.top = 0x1e1e1e;
                    box.bottom = 0x5e5e5e;
                }
                Menu_DrawCursorBox((uint *)ot, &box, list->blinkTimer * 0x80 / 60);
            }
            b = 0x20;
            if (((unsigned char)it->kind >> 7 ^ 1) != 0) {
                b = 0x80;
            }
            if (i == fadeOut) {
                b = b * fadeA >> 7;
            }
            if (i == fadeIn) {
                b = b * fadeB >> 7;
            }
            MenuItem_SetBrightness_OVL2(it, b);
            MenuItem_SetPos_OVL2(it, list->x, y + (h >> 1));
            MenuItem_Draw_OVL2(it, (char *)ot + 4, ctx);
            y += list->itemGap + h;
        }
    }
    if (list->state >= 0) {
        int t = 0x28 - list->state;
        short x = list->x;
        int c;

        if (t > 10) {
            t = 10;
        }
        if (t < 0) {
            t = 0;
        }
        c = t * 0xff / 10;
        if (first > 0) {
            PrimF3 *p = Prim_AddPolyF3((uint *)ot, c | (c >> 1) << 8 | 0x2000000);
            short ay;

            p->x1 = x + 6;
            p->x0 = x;
            p->x2 = x - 6;
            ay = list->y - list->itemGap;
            p->y0 = ay - 10;
            p->y1 = ay;
            p->y2 = ay;
        }
        if (end < list->count) {
            PrimF3 *p = Prim_AddPolyF3((uint *)ot, c | (c >> 1) << 8 | 0x2000000);
            short ay;

            p->x1 = x - 6;
            p->x0 = x;
            p->x2 = x + 6;
            ay = list->y + rowH * rows;
            p->y0 = ay + 10;
            p->y1 = ay;
            p->y2 = ay;
        }
        Prim_AddDrawMode(ot, 0x20);
    }
}
#else
INCLUDE_ASM("asm/sim/ovl2/nonmatchings/menuitem", MenuList_Draw_OVL2);
#endif
}
