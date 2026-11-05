// Resident code 8006cdcc-8006d9c8 (menulist): the vertical list menu. The rows are
// drawn and told about the list's changes by the list's callback (MLE_ events).
#include "common.h"

extern "C" {

// Resets the list and sends MLE_INIT to every item.
void MenuList_Init(MenuList *list, MenuListFn *callback, void *userData)
{
    MenuListEvent ev;
    int i;

    ev.list = list;
    list->state = -1;
    list->selected = 0;
    list->blinkTimer = 0;
    list->scrollAnim = 0;
    list->revealIndex = 0;
    list->revealTimer = list->revealDelay = 6;
    list->fade = 0;
    list->callback = callback;
    list->userData = userData;
    if (!(list->skipEvents & 1)) {
        for (i = 0; i < list->count; i++) {
            list->callback(MLE_INIT, &ev, i);
        }
    }
}

// Restarts the items' reveal and sends MLE_OPEN for the selected item.
void MenuList_Open(MenuList *list)
{
    MenuListEvent ev;

    ev.list = list;
    list->revealTimer = list->revealDelay = 6;
    list->blinkTimer = 0;
    list->scrollAnim = 0;
    list->revealIndex = 0;
    list->fade = 0;
    list->state = 0;
    list->blinkTimer = 0x1e;
    if (!(list->skipEvents & 0x80)) {
        list->callback(MLE_OPEN, &ev, list->selected);
    }
}

// Starts the close animation (state -0x41) and sends MLE_CLOSE to every item but the
// selected one (closed later, at -0x3a).
void MenuList_Close(MenuList *list)
{
    MenuListEvent ev;
    int i;

    ev.list = list;
    list->state = -0x41;
    list->fade = list->fadeMax;
    if (!(list->skipEvents & 4)) {
        for (i = 0; i < list->count; i++) {
            if (i != list->selected) {
                list->callback(MLE_CLOSE, &ev, i);
            }
        }
    }
}

// The list's brightness, 0 to 128, from fade / fadeMax, following the close animation.
int MenuList_GetFade(MenuList *list)
{
    int fade = list->state;

    if (fade < -1) {
        int max = list->fadeMax;

        int t = max - 0x3b;

        fade = t - fade;
        if (fade < 0) {
            fade = 0;
        }
        if (fade > max) {
            fade = max;
        }
    } else {
        fade = list->fade;
    }
    return (fade << 7) / list->fadeMax;
}

// Runs one frame of the list (pad 0 = no input). Up and Down move (wrapping with flags & 4),
// Left and Right jump a page (flags & 0x10), Cross and Circle confirm, Triangle and Square
// cancel. Returns the confirmed index, -1 to cancel, -2 for nothing, -3 when the cursor
// moved, -4 to confirm a disabled item (MLE_IS_ENABLED returned 0).
// Three scheduling differences: the first loop's test against $s2 (the 0 of prev), which
// the for gets with blez, and the load before the cursor's increment or copy.
int MenuList_Update(MenuList *list, PadState *pad)
{
    MenuListEvent ev;
    int result = -2;
    int prev = 0;
    int event = -1;
    int sel;
    PadButton buttons;

    ev.list = list;
    ev.pad = pad;
    list->hadInput = 0;
    if (!(list->skipEvents & 8)) {
        for (sel = 0; sel < list->count; sel++) {
            list->callback(MLE_UPDATE, &ev, sel);
        }
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
    if (list->state < 0) {
        if (list->state != -1) {
            list->state++;
            if (!(list->skipEvents & 4) && list->state == -0x3a) {
                list->callback(MLE_CLOSE, &ev, list->selected);
            }
            if (--list->fade < 0) {
                list->fade = 0;
            }
        }
        return result;
    }
    if (++list->fade > list->fadeMax) {
        list->fade = list->fadeMax;
    }
    if (++list->state > 0x2d) {
        list->state = 0;
    }
    if (pad != 0) {
        list->hadInput = 1;
        buttons = pad->pressed;
        if (buttons & (PAD_SQUARE | PAD_TRIANGLE)) {
            result = -1;
        } else {
            sel = list->selected;
            if (buttons & (PAD_CIRCLE | PAD_CROSS)) {
                result = sel;
                if (list->callback(MLE_IS_ENABLED, &ev, result) == 0) {
                    result = -4;
                }
            } else {
                buttons |= pad->repeat;
                if (buttons & PAD_UP) {
                    prev = sel;
                    sel--;
                    event = MLE_LEAVE;
                    if (sel < 0) {
                        sel = 0;
                        if (list->flags & 4) {
                            sel = list->count - 1;
                        }
                    }
                    if (sel != list->selected) {
                        result = -3;
                        list->scrollAnim = -8;
                        list->blinkTimer = 0;
                    }
                }
                if (buttons & PAD_DOWN) {
                    int count;

                    prev = sel;
                    count = list->count;
                    sel++;
                    event = MLE_LEAVE;
                    if (sel >= count) {
                        sel = count - 1;
                        if (list->flags & 4) {
                            sel = 0;
                        }
                    }
                    if (sel != list->selected) {
                        result = -3;
                        list->scrollAnim = 8;
                        list->blinkTimer = 0;
                    }
                }
                if (list->flags & 0x10) {
                    if (buttons & PAD_LEFT) {
                        int rows = list->visibleRows;

                        prev = sel;
                        sel = sel - rows - 1;
                        event = MLE_LEAVE;
                        if (sel < 0) {
                            sel = 0;
                        }
                        result = -3;
                        list->scrollAnim = -8;
                        list->blinkTimer = 0;
                    }
                    if (buttons & PAD_RIGHT) {
                        prev = sel;
                        sel = prev + list->visibleRows - 1;
                        event = MLE_LEAVE;
                        if (sel >= list->count) {
                            sel = list->count - 1;
                        }
                        result = -3;
                        list->scrollAnim = 8;
                        list->blinkTimer = 0;
                    }
                }
                if (event >= 0) {
                    list->callback(event, &ev, prev);
                    list->callback(MLE_ENTER, &ev, sel);
                }
                list->selected = sel;
            }
        }
    }
    if (list->revealIndex < list->count) {
        do {
            if (--list->revealTimer <= 0) {
                if (!(list->skipEvents & 2)) {
                    list->callback(MLE_REVEAL, &ev, list->revealIndex);
                }
                list->revealTimer = list->revealDelay;
                list->revealIndex++;
            }
        } while ((list->revealIndex < list->count) & (list->revealTimer < 0));
    }
    return result;
}

// Moves the cursor to index (clamped), with the scroll animation and MLE_LEAVE and
// MLE_ENTER.
void MenuList_SetSelection(MenuList *list, int index)
{
    MenuListEvent ev;
    int sel = list->selected;
    int old = sel;

    ev.list = list;
    if (index >= list->count) {
        index = list->count - 1;
    }
    if (index < 0) {
        index = 0;
    }
    if (index < sel) {
        list->scrollAnim = -8;
    }
    if (index > sel) {
        list->scrollAnim = 8;
    }
    list->blinkTimer = 0;
    if (index != old) {
        list->callback(MLE_LEAVE, &ev, old);
        list->callback(MLE_ENTER, &ev, index);
    }
    list->selected = index;
}

// Clamps the selection into [min, max] and into the item count.
void MenuList_ClampSelection(MenuList *list, int min, int max)
{
    int sel = list->selected;

    if (sel < min) {
        sel = min;
        list->scrollAnim = 0;
    }
    if (sel > max) {
        sel = max;
        list->scrollAnim = 0;
    }
    if (sel >= list->count) {
        sel = list->count - 1;
    }
    if (sel < 0) {
        sel = 0;
    }
    list->selected = sel;
}

// Draws the visible rows through MLE_DRAW (disabled rows at half brightness), the blinking
// cursor box (flags & 8) and the scroll arrows.
// Shape only: the game spills fadeIn, fadeA, fadeB and rowH to the stack and keeps sel in
// $fp; registers and statement order differ throughout.
void MenuList_Draw(MenuList *list, void *ot)
{
    MenuListEvent ev;
    GouraudQuad box;
    int arrow;
    int sel = list->selected;
    int rows = list->visibleRows;
    int rowH = list->itemGap + list->itemHeight;
    int fadeOut = -1;
    int fadeIn = -1;
    int end = rows;
    int scroll = list->scrollAnim;
    int y = list->y + scroll * (list->itemHeight + list->itemGap) / 8;
    int half = rows >> 1;
    int after = rows - half;
    int fadeB = scroll * 0x80 / 8;
    int fadeA;
    int first;
    int i;

    ev.list = list;
    ev.ot = ot;
    ev.row = 0;
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
    if (scroll < 0) {
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
                end = list->visibleRows;
                y = list->y;
                fadeOut = -1;
            }
        } else if ((sel < half) | (sel >= list->count - after)) {
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
                y = list->y;
                end = rows;
                fadeOut = first;
                first = 0;
            }
        } else if ((sel <= half) | (sel > list->count - after)) {
            first++;
            end = rows;
            y = list->y;
        }
    }
    end += first;
    y += list->itemHeight >> 1;
    for (i = first; i < end; i++) {
        if ((i >= 0) & (i < list->count)) {
            int brightness;

            if (((i == sel) & (list->state >= 0)) && (list->flags & 8)) {
                box.rect.y = y;
                box.rect.x = list->x;
                box.rect.y = y - (list->itemHeight >> 1);
                box.rect.w = list->cursorWidth;
                box.rect.h = list->itemHeight;
                box.top = 0xf0f0f;
                box.bottom = 0x363636;
                Menu_DrawCursorBox((uint *)ot, &box, list->blinkTimer * 0x80 / 60);
            }
            brightness = 0x80;
            if (i == fadeOut) {
                brightness = fadeA;
            }
            if (i == fadeIn) {
                brightness = fadeB;
            }
            ev.enabled = 1;
            if (list->callback(MLE_IS_ENABLED, &ev, i) == 0) {
                brightness >>= 1;
                ev.enabled = 0;
            }
            ev.brightness = brightness;
            ev.y = y;
            ev.x = list->x;
            if (!(list->skipEvents & 0x10)) {
                list->callback(MLE_DRAW, &ev, i);
            }
            ev.row++;
        }
        y += rowH;
    }
    if (list->state >= 0 && (signed char)list->hadInput != 0) {
        int anim = list->state;
        int x;

        if (list->flags & 0x20) {
            anim = 0;
        }
        x = list->x;
        if (first > 0) {
            Menu_DrawScrollArrow(&arrow, (uint *)ot, x, list->y - list->itemGap,
                                 (signed char)list->arrowWidth, -(signed char)list->arrowHeight, anim);
        }
        if (end < list->count) {
            Menu_DrawScrollArrow(&arrow, (uint *)ot, x, list->y + rowH * rows,
                                 (signed char)list->arrowWidth, (signed char)list->arrowHeight, anim);
        }
        Prim_AddDrawMode(ot, 0x20);
    }
}

}
