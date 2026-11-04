// OVL1 8001779c-80017d74 (mainmenupage): Main menu page.
// The functions that don't match yet are the original assembly, with their C++
// under NON_MATCHING.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"
#include "gamestate.h"

extern "C" {
extern short g_MainMenu_Fade;
extern short g_MainMenu_FadeOut;
extern short g_MainMenu_State;
extern MenuList g_MainMenu_List;
extern short g_MainMenu_OpenDelay;
extern GameSaveFile g_SaveFileBuf;
extern int g_MainMenu_IdleFrames;
extern MenuSprite g_MainMenu_Items[];
extern short g_MainMenu_RowChoice[];
extern short g_MainMenu_RowSprite[];
extern MenuSpriteStyle g_MainMenu_ItemStyle;
extern SpriteDesc *g_MainMenu_SpriteTables[];
extern FlowMenuState g_Flow_IntroShown;
extern signed char g_Flow_MainMenuChoice;
extern BaseMenu *g_CurrentMenu;

#ifdef NON_MATCHING
// The rows are sprites. A row whose choice is negative does nothing.
int MainMenu_ItemCallback(MenuListEventId event, MenuListEvent *ev, int index)
{
    MenuSprite *item = &g_MainMenu_Items[index];
    int brightness = ev->brightness;
    MenuList *list = ev->list;
    int lang = g_GameState.language;
    int result = 0;
    int value;
    int selected;

    if (g_MainMenu_RowChoice[index] >= 0) {
        switch (event) {
        case MLE_INIT:
            MenuSprite_Init(item, &g_MainMenu_ItemStyle);
            item->sprite = &g_MainMenu_SpriteTables[lang][g_MainMenu_RowSprite[index]];
            break;
        case MLE_REVEAL:
            item->anim = 0;
            break;
        case MLE_CLOSE:
            break;
        case MLE_UPDATE:
            MenuSprite_Update(item);
            break;
        case MLE_DRAW:
            value = brightness * ((list->fade << 7) / list->fadeMax);
            selected = list->selected == index;
            brightness = value >> 7;
            if (!selected) {
                brightness = value >> 9;
            }
            item->brightness = brightness;
            item->x = ev->x;
            item->y = ev->y;
            MenuSprite_Draw(item, ev->ot, selected);
            break;
        case MLE_LEAVE:
            break;
        case MLE_ENTER:
            item->anim = 0xc;
            break;
        case MLE_OPEN:
            break;
        case MLE_IS_ENABLED:
            result = 1;
            break;
        }
    }
    return result;
}
#else
INCLUDE_ASM("asm/sim/ovl1/nonmatchings/mainmenupage", MainMenu_ItemCallback);
#endif

// g_Page_MainMenu enter: resets the fades and the idle counter and builds
// g_MainMenu_List (rows are sprites, see MainMenu_ItemCallback).
void MainMenu_Enter(int back)
{
    g_MainMenu_Fade = 0;
    g_MainMenu_FadeOut = 0x10;
    g_MainMenu_State = 0;
    g_MainMenu_OpenDelay = 0x10;
    MenuList_Init(&g_MainMenu_List, MainMenu_ItemCallback, (void *)0x0);
    g_MainMenu_IdleFrames = 0;
}

// Opens the list after a delay and fades it in. After 901 idle frames, or when a row
// is chosen, closes it and fades out: then leaves the menu (4).
int MainMenu_Update(int arg)
{
    PadState *pad = (PadState *)((char *)g_CurrentMenu + 0x1a4);
    int result = 0;
    MenuList *list;
    int old;
    int row;
    FlowMenuState *flow;

    switch (g_MainMenu_State) {
    case 0:
        if (--g_MainMenu_OpenDelay == 0) {
            g_MainMenu_State = 1;
            MenuList_Open(&g_MainMenu_List);
            g_MainMenu_List.revealDelay = -1;
        }
    case 1:
        if (++g_MainMenu_Fade > 12) {
            g_MainMenu_Fade = 12;
        }
        break;
    case 2:
        if (--g_MainMenu_Fade < 0) {
            g_MainMenu_Fade = 0;
        }
        if (--g_MainMenu_FadeOut < 0) {
            result = 4;
        }
        break;
    }
    if (pad->held != 0) {
        g_MainMenu_IdleFrames = 0;
    } else if (++g_MainMenu_IdleFrames > 0x384) {
        MenuList_Close(&g_MainMenu_List);
        g_MainMenu_State = 2;
        g_Flow_MainMenuChoice = 6;
    }
    if (arg == 0) {
        pad = 0;
    }
    list = &g_MainMenu_List;
    old = list->selected;
    row = MenuList_Update(list, pad);
    MenuList_ClampSelection(list, 1, 6);
    switch (row) {
    case -4:
        break;
    case -3:
        if (old != list->selected) {
            Sfx_PlayMenu(6);
        }
        break;
    case -2:
    case -1:
        break;
    default:
        Sfx_PlayMenu(3);
        MenuList_Close(&g_MainMenu_List);
        g_MainMenu_State = 2;
        flow = &g_Flow_IntroShown;
        ((signed char *)flow)[0x11] = 0;
        if ((pad->held & 0x10000) != 0) {
            ((signed char *)flow)[0x11] = 1;
        }
        flow->mainMenuChoice = g_MainMenu_RowChoice[row];
        break;
    }
    return result;
}

// The list, then the four quads of the background picture.
void MainMenu_Draw(DrawLayer *layer)
{
    OtTag *ot = (OtTag *)layer->ot;
    int b;
    unsigned int rgb;
    TexQuad *q;

    MenuList_Draw(&g_MainMenu_List, ot);
    b = (g_MainMenu_Fade << 7) / 12;
    rgb = b | b << 8 | b << 16;

    q = (TexQuad *)Prim_AddTexQuad(ot, rgb);
    q->x = 0x86;
    q->texX = 0;
    q->y = 0;
    q->u = 0;
    q->v = 0;
    q->w = 0x100;
    q->h = 0x100;
    q->clut = 0x7fd8;

    q = (TexQuad *)Prim_AddTexQuad(ot, rgb);
    q->x = 0x96;
    q->texX = 0;
    q->y = 0x100;
    q->u = 0;
    q->v = 0;
    q->w = 0x100;
    q->h = 0xe0;
    q->clut = 0x7fd8;

    q = (TexQuad *)Prim_AddTexQuad(ot, rgb);
    q->x = 0x88;
    q->texX = 0x100;
    q->y = 0;
    q->u = 0;
    q->v = 0;
    q->w = 0x60;
    q->h = 0x100;
    q->clut = 0x7fd8;

    q = (TexQuad *)Prim_AddTexQuad(ot, rgb);
    q->x = 0x98;
    q->texX = 0x100;
    q->y = 0x100;
    q->u = 0;
    q->v = 0;
    q->w = 0x60;
    q->h = 0xe0;
    q->clut = 0x7fd8;

    Prim_AddDrawMode(ot, 0x280);
}
}
