// OVL1 8001721c-80017720 (langselect): Language selection pages.
// The functions that don't match yet are the original assembly, with their C++
// under NON_MATCHING.
#include "common.h"
#include "gamestate.h"

extern "C" {
extern short (&g_LangSelect_Fade);
extern short (&g_LangSelect_Languages)[];
extern short (&g_LangSelect_DefaultRow)[];
extern MenuList (&g_LangSelect_List);
extern BaseMenuPage (&g_Page_LangSelectFadeOut);
extern SpriteDesc (&g_LangSelect_RowSprites)[];
extern short (&g_LangSelect_RowSprite)[];
extern short (&g_LangSelect_Done);
extern short (&g_LangSelectFadeOut_Timer);
extern game_pointer<BaseMenu> (&g_CurrentMenu);
void PushPage(BaseMenu *menu, BaseMenuPage *page);

// Draws a row's sprite, at half brightness when it isn't selected.
int LangSelect_ItemCallback(MenuListEventId event, MenuListEvent *ev, int index)
{
    MenuList *list = ev->list;
    void *ot = ev->ot;
    int x = ev->x;
    int y = ev->y;
    int result = 0;
    SpriteDesc *spr;
    int b;
    PrimSprite *p;

    switch (event) {
    case MLE_INIT:
    case MLE_REVEAL:
    case MLE_CLOSE:
    case MLE_UPDATE:
    case MLE_LEAVE:
    case MLE_ENTER:
    case MLE_OPEN:
        break;
    case MLE_DRAW:
        spr = &g_LangSelect_RowSprites[g_LangSelect_RowSprite[index]];
        b = (g_LangSelect_Fade << 7) / 20;
        if (index != list->selected) {
            b >>= 1;
        }
        p = Prim_AddSprite(ot, b | b << 8 | b << 16);
        *(int *)p = (x - (((short *)&spr->wh)[0] >> 1)) + ((y - (((short *)&spr->wh)[1] >> 1)) << 16);
        ((unsigned int *)p)[1] = spr->uvClut;
        ((unsigned int *)p)[2] = spr->wh;
        Prim_AddDrawMode(ot, spr->tpage);
        break;
    case MLE_IS_ENABLED:
        result = 1;
        break;
    }
    return result;
}

// Opens the list on the row of the game's language.
void LangSelect_Enter(int back)
{
    MenuList *list = &g_LangSelect_List;
    int lang;
    int row;

    MenuList_Init(list, LangSelect_ItemCallback, 0);
    MenuList_Open(list);
    lang = g_GameState.language;
    if (lang < 0) {
        lang = 0;
    }
    if (lang >= 7) {
        lang = 0;
    }
    g_LangSelect_Fade = 0;
    row = g_LangSelect_DefaultRow[lang];
    list->selected = row;
    g_LangSelect_Done = 0;
}

// Fades in over 20 frames, the list taking input once it has. Choosing a row
// sets the language and pushes the fade out page.
int LangSelect_Update(int arg)
{
    int result = 0;
    PadState *pad = (PadState *)((char *)g_CurrentMenu + 0x1a4);
    int row;
    int lang;

    if (g_LangSelect_Done == 0) {
        if (g_LangSelect_Fade < 20) {
            g_LangSelect_Fade++;
        }
        if (g_LangSelect_Fade >= 20) {
            g_LangSelect_Fade = 20;
        }
    } else if (g_LangSelect_Fade > 0) {
        g_LangSelect_Fade--;
    }
    if (g_LangSelect_Fade < 20) {
        pad = 0;
    }
    if (arg == 0) {
        pad = 0;
    }
    row = MenuList_Update(&g_LangSelect_List, pad);
    switch (row) {
    case -3:
        Sfx_PlayMenu(6);
        return result;
    case -2:
    case -1:
        return result;
    case -4:
        break;
    default:
        result = 1;
        g_LangSelect_Done = 1;
        g_LangSelect_Fade--;
        Sfx_PlayMenu(3);
        lang = g_LangSelect_Languages[row];
        g_GameState.language = lang;
        PushPage(g_CurrentMenu, &g_Page_LangSelectFadeOut);
        break;
    }
    return result;
}

// Once faded in: the list, then the four quads of the background picture.
void LangSelect_Draw(DrawLayer *layer)
{
    OtTag *ot = (OtTag *)layer->ot;
    int b;
    unsigned int rgb;
    TexQuad *q;

    if (g_LangSelect_Fade < 20) {
        return;
    }
    MenuList_Draw(&g_LangSelect_List, ot);
    b = (g_LangSelect_Fade << 7) / 20;
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

// Sets a 12-frame timer.
void LangSelectFadeOut_Enter(int back)
{
    g_LangSelectFadeOut_Timer = 0xc;
}

// Counts the timer down, then leaves the menu (4).
int LangSelectFadeOut_Update(int arg)
{
    return --g_LangSelectFadeOut_Timer < 1 ? 4 : 0;
}

// Empty page draw callback.
void Page_DrawNothing(DrawLayer *layer)
{
}
}
