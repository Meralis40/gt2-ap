// OVL4 8001e22c-8001ec0c (mapcursor): Map cursor.
#include "common.h"

extern "C" {
// g_MapCursor_Sprite, then g_MapCursor_SpriteActive (over a hotspot).
extern SpriteDesc (&g_MapCursor_Sprite)[2];

// Centres the cursor at (0x100, 0xfc) with no hotspot.
void MapCursor_Init(MapCursor *cursor)
{
    cursor->x = 0x100;
    cursor->prevX = 0x100;
    cursor->drawX = 0x100;
    cursor->y = 0xfc;
    cursor->prevY = 0xfc;
    cursor->drawY = 0xfc;
    cursor->moveFrames = '\0';
    cursor->posX = 0;
    cursor->posY = 0;
    cursor->velX = 0;
    cursor->velY = 0;
    cursor->blink = '\0';
    cursor->hotspot = (MenuElement *)0x0;
}

// Puts the cursor on its target and pushes it the other way from dir, for a
// move with no hotspot to go to.
void MapCursor_Bump(MapCursor *cursor, int dir)
{
    int velX = 0;
    int velY = 0;

    cursor->drawX = cursor->x;
    cursor->drawY = cursor->y;
    cursor->posX = cursor->drawX << 8;
    cursor->posY = cursor->drawY << 8;
    switch (dir) {
    case 4:
        velX = 10000000;
        break;
    case 7:
        velX = 10000000;
        velY = 10000000;
        break;
    case 6:
        velX = -10000000;
        velY = 10000000;
        break;
    case 2:
        velY = 10000000;
        break;
    case 5:
        velX = -10000000;
        break;
    case 8:
        velX = -10000000;
        velY = -10000000;
        break;
    case 9:
        velX = 10000000;
        velY = -10000000;
        break;
    case 3:
        velY = -10000000;
        break;
    }
    cursor->velX = velX;
    cursor->velY = velY;
}

// Moves the cursor with the pad (input 0 ignores it): a direction goes to the
// nearest hotspot that way, a second direction held during the move turns it
// diagonal, and the draw position follows the target on a spring. Circle or
// cross runs the hotspot, square or triangle goes back.
void MapCursor_Update(MapCursor *cursor, GTMenu *menu, int input)
{
    PadState *pad = &menu->padState[0];
    unsigned int held;
    unsigned int buttons;
    int dir;
    int turn;
    MenuElement *e;
    int velX;
    int velY;

    if (++cursor->blink > 64)
        cursor->blink = 0;
    if (cursor->moveFrames == 0) {
        cursor->drawX = cursor->x;
        cursor->velX = 0;
        cursor->velY = 0;
        cursor->drawY = cursor->y;
        cursor->posX = cursor->drawX << 8;
        cursor->posY = cursor->drawY << 8;
    }
    if (cursor->moveFrames < 2) {
        held = menu->padState[0].held;      // from menu, where the rest is through pad
        buttons = pad->pressed | pad->repeat;
        if (input == 0) {
            held = 0;
            buttons = 0;
        }
        dir = 0;
        if (buttons & PAD_L1)
            dir = 7;
        if (buttons & PAD_L2)
            dir = 9;
        if (buttons & PAD_R1)
            dir = 6;
        if (buttons & PAD_R2)
            dir = 8;
        if (buttons & PAD_UP) {
            dir = 2;
            if (held & PAD_LEFT)
                dir = 7;
            if (held & PAD_RIGHT)
                dir = 6;
        }
        if (buttons & PAD_DOWN) {
            dir = 3;
            if (held & PAD_LEFT)
                dir = 9;
            if (held & PAD_RIGHT)
                dir = 8;
        }
        if (buttons & PAD_LEFT) {
            dir = 4;
            if (held & PAD_UP)
                dir = 7;
            if (held & PAD_DOWN)
                dir = 9;
        }
        if (buttons & PAD_RIGHT) {
            dir = 5;
            if (held & PAD_UP)
                dir = 6;
            if (held & PAD_DOWN)
                dir = 8;
        }
        if (dir != 0) {
            e = MenuView_FindHotspotInDir(&menu->view, cursor->x, cursor->y, dir, cursor->hotspot, 1);
            cursor->dir = dir;
            if (e == 0) {
                Sfx_PlayMenu(0);
                MapCursor_Bump(cursor, dir);
                dir = 1;
                cursor->prevX = cursor->x;
                cursor->prevY = cursor->y;
            } else {
                Sfx_PlayMenu(5);
                cursor->prevX = cursor->x;
                cursor->prevY = cursor->y;
                cursor->x = (e->x0 + e->x1) >> 1;
                cursor->y = (e->y0 + e->y1) >> 1;
                cursor->hotspot = e;
                cursor->blink = 0;
            }
            cursor->lastDir = dir;
            cursor->moveFrames = 8;
        }
    }
    if (cursor->moveFrames != 0) {
        cursor->moveFrames--;
        if (cursor->moveFrames >= 6) {
            buttons = pad->held;
            if (input == 0)
                buttons = 0;
            turn = cursor->dir;
            switch (turn) {
            case 2:
                if (buttons & PAD_LEFT)
                    turn = 7;
                if (buttons & PAD_RIGHT)
                    turn = 6;
                break;
            case 3:
                if (buttons & PAD_LEFT)
                    turn = 9;
                if (buttons & PAD_RIGHT)
                    turn = 8;
                break;
            case 4:
                if (buttons & PAD_UP)
                    turn = 7;
                if (buttons & PAD_DOWN)
                    turn = 9;
                break;
            case 5:
                if (buttons & PAD_UP)
                    turn = 6;
                if (buttons & PAD_DOWN)
                    turn = 8;
                break;
            }
            if (turn != cursor->dir) {
                e = MenuView_FindHotspotInDir(&menu->view, cursor->prevX, cursor->prevY, turn, 0, 1);
                if (e != 0) {
                    cursor->lastDir = turn;
                    cursor->dir = turn;
                    cursor->x = (e->x0 + e->x1) >> 1;
                    cursor->y = (e->y0 + e->y1) >> 1;
                    cursor->hotspot = e;
                    cursor->blink = 0;
                } else if (cursor->lastDir == 1) {
                    MapCursor_Bump(cursor, turn);
                    cursor->prevX = cursor->x;
                    cursor->prevY = cursor->y;
                }
            }
        }
    }
    velX = cursor->velX + ((cursor->x * 256 - cursor->posX) * 2500 - cursor->velX * 110) / 60 * 100 / 180;
    velY = cursor->velY + ((cursor->y * 256 - cursor->posY) * 2500 - cursor->velY * 110) / 60 * 100 / 180;
    cursor->velX = velX;
    cursor->velY = velY;
    cursor->posX += velX / 60;
    cursor->posY += velY / 60;
    cursor->drawX = cursor->posX / 256;
    cursor->drawY = cursor->posY / 256;
    buttons = pad->pressed;
    if (input == 0)
        buttons = 0;
    if ((buttons & (PAD_CIRCLE | PAD_CROSS)) && cursor->hotspot
        && SimMap_Update(menu, cursor->hotspot, (buttons >> 9) & 1))
        return;
    if (buttons & (PAD_SQUARE | PAD_TRIANGLE))
        GTMenu_GoBack(menu);
}

// Moves the cursor to the page's default element, or else takes the hotspot
// under it.
void MapCursor_ResetToDefault(MapCursor *cursor)
{
    if ((cursor->hotspot = MenuView_FindDefaultElement(&g_GTMenu->view)) == 0) {
        cursor->hotspot = MenuView_HotspotAt(&g_GTMenu->view, cursor->x, cursor->y);
    } else {
        cursor->prevX = cursor->x;
        cursor->prevY = cursor->y;
        cursor->x = (cursor->hotspot->x0 + cursor->hotspot->x1) >> 1;
        cursor->y = (cursor->hotspot->y0 + cursor->hotspot->y1) >> 1;
        cursor->moveFrames = 8;
    }
    cursor->blink = 0;
}

// Starts a move to element.
void MapCursor_MoveTo(MapCursor *cursor, MenuElement *element)
{
    cursor->hotspot = element;
    cursor->prevX = cursor->x;
    cursor->prevY = cursor->y;
    cursor->x = (cursor->hotspot->x0 + cursor->hotspot->x1) >> 1;
    cursor->y = (cursor->hotspot->y0 + cursor->hotspot->y1) >> 1;
    cursor->moveFrames = 8;
    cursor->blink = 0;
}

// Draws the cursor, with the other sprite over a hotspot.
void MapCursor_Draw(MapCursor *cursor, void *ot)
{
    SpriteDesc *sprite = g_MapCursor_Sprite;
    unsigned int *prim;

    if (cursor->hotspot != 0)
        sprite++;
    prim = (unsigned int *)Prim_AddSprite(ot, 0x808080);
    prim[0] = cursor->drawX + (cursor->drawY << 16);
    prim[1] = sprite->uvClut;
    prim[2] = sprite->wh;
    Prim_AddDrawMode(ot, sprite->tpage);
}

// A garage's car, for the game's addressing (the base added to i * 0xa4).
static inline Car *Garage_Car(Garage *garage, int i)
{
    return &garage->cars[i];
}

// Sorts the garage's cars by name into order, by insertion with a binary
// search. The car being inserted is reached by a byte offset kept beside i.
void FUN_OVL4__8001eab8(Garage *garage, short *order)
{
    int n = 0;
    int i = 0;
    int off = 0;
    int lo;
    int hi;
    int mid;
    int j;
    char *name;

    while (1) {
        if (i >= garage->carCount)
            return;
        name = CarInfoA_GetName(((Car *)((char *)garage->cars + off))->baseCarId);
        lo = 0;
        hi = n;
        mid = n;
        if (n > 0) {
            for (;;) {
                mid = (hi + lo) >> 1;
                if (strcmp(name, CarInfoA_GetName(Garage_Car(garage, order[mid])->baseCarId)) < 0) {
                    hi = mid;
                    if (lo == mid)
                        break;
                } else {
                    if (lo == mid) {
                        mid = hi;
                        break;
                    }
                    lo = mid;
                }
            }
        }
        for (j = n; mid < j; j--)
            order[j] = order[j - 1];
        order[mid] = i;
        n++;
        off += sizeof(Car);
        i++;
    }
}
}
