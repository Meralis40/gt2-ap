// OVL2 80023690-8002445c (bonus): Bonus items pages.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

static inline int Half(unsigned short v)
{
    return (short)v >> 1;
}

// A sprite in a row, offset pixels left of its place.
static inline void Row_PutSprite(void *ot, ArcSprite *sprite, unsigned int rgb, int offset, int x,
                                 int y)
{
    PrimSprite *p = Prim_AddSprite(ot, rgb);

    *(int *)p = x - (Half(sprite->w) + offset) + ((y - Half(sprite->h)) << 16);
    *(int *)&p->u = *(int *)sprite;
    *(int *)&p->w = *(int *)&sprite->w;
    Prim_AddDrawMode(ot, sprite->tpage);
}

extern "C" {
extern Font D_80123810;
extern ArcBonusCourse g_ArcBonusCourses[];
extern MenuList g_ArcList_Bonus;
extern ArcMenuList g_ArcMenu_Bonus;
extern ArcMenuItem D_80052754[2];
extern ButtonPrompt g_Arc_BonusPrompt;
extern ButtonPromptDesc D_8005272C;
extern short g_Arc_BonusCourseCount;
extern short g_Arc_BonusChoice;
extern MenuPage g_ArcPage_BonusSelect;
extern MenuPage g_ArcPage_BonusLaunch;
extern unsigned char D_801C9998[]; // PlayerStats
extern unsigned int D_800526DC[7]; // the licences' colours
extern unsigned char D_800526B8[];
extern unsigned char D_800526BC[];
extern unsigned char D_800526C0[];
extern unsigned char D_800526C8[];
extern unsigned char D_800526CC[];
extern unsigned char D_800526D0[];
extern unsigned char D_800526D4[];
extern unsigned char D_800526D8[];
extern ArcSprite D_80052604;
extern ArcSprite D_80052628;
extern ArcSprite D_80052634;
extern ArcSprite D_80052640;
extern ArcSprite D_8005264C;
extern ArcSprite D_80052658[2];
extern ArcSprite D_8005267C[3]; // the medals
extern ArcSprite D_800526A0;
extern ArcSprite D_800526AC;
extern char D_800F87B9[];
int ArcBonus_AllCoursesWon(void);
int ArcBonus_IsStat349Set(void);


#ifdef NON_MATCHING
// A course's name, licence swatch, medal and lamps.
void BonusItems_DrawRow(ArcBonusCourse *course, ArcadeMenu *menu, void *ot, int x, int y,
                        int brightness)
{
    GouraudQuad quad;
    unsigned int color;
    int medal = -1;
    int stat;
    int gold;
    TextCtx *text;
    int show;
    int flag;
    ArcSprite *sprite;

    gold = course->gold != 0;
    text = menu->text;
    stat = course->stat;
    Text_SetFont(text, &D_80123810);
    text->ot = (uint *)ot;
    *(uint *)&text->clut = (*(uint *)&text->clut & 0xff9fffff) | 0x200000;
    text->rgb = Rgb_Lerp(D_800526B8, D_800526C0, brightness, 0x80);
    Text_DrawString(text, (signed char *)course->name, x - 0x48, y + 8, 1);
    color = D_800526DC[6];
    if (course->license >= 0) {
        color = D_800526DC[course->license];
    }
    quad.rect.x = x - 0x9a;
    quad.rect.y = y - 0xc;
    quad.rect.w = 0x1a;
    quad.rect.h = 0x18;
    quad.top = Rgb_Lerp(D_800526B8, (unsigned char *)&color, brightness, 0x200);
    quad.bottom = Rgb_Lerp(D_800526B8, (unsigned char *)&color, brightness, 0x80);
    Menu_AddGouraudQuadOpaque((uint *)ot, &quad);
    Prim_AddDrawMode(ot, 0x220);
    show = 0;
    flag = course->lamp != 0;
    if (stat >= 0) {
        show = flag & 1;
    }
    if (show) {
        ArcBonusFlags *flags;

        color = Rgb_Lerp(D_800526BC, D_800526D4, brightness, 0x80);
        flags = (ArcBonusFlags *)&D_801C9998[stat];
        if (flags->bit1 | flags->bit2) {
            color = Rgb_Lerp(D_800526BC, D_800526D8, brightness, 0x80);
        }
        Row_PutSprite(ot, &D_800526AC, color, -0x90, x, y);
    }
    if (stat >= 0) {
        ArcBonusFlags *flags = (ArcBonusFlags *)&D_801C9998[stat];

        if (flags->bit2) {
            medal = 2;
        } else if (flags->bit1) {
            medal = 1;
        } else if (flags->bit0) {
            medal = 0;
        }
        if (medal >= 0) {
            Row_PutSprite(ot, &D_8005267C[medal], brightness | brightness << 8 | brightness << 16,
                          0x6c, x, y);
        }
        sprite = &D_80052628;
        if (gold) {
            if (medal == 2) {
                Row_PutSprite(ot, &D_800526A0, brightness | brightness << 8 | brightness << 16,
                              0x5d, x, y);
            }
            sprite = &D_80052634;
        }
    } else {
        sprite = &D_80052640;
        if (gold) {
            Row_PutSprite(ot, &D_800526A0, brightness | brightness << 8 | brightness << 16, 0x5d,
                          x, y);
            sprite = &D_8005264C;
        }
    }
    color = Rgb_Lerp(D_800526BC, D_800526C8, brightness, 0x80);
    if (course->lit != 0) {
        color = Rgb_Lerp(D_800526BC, D_800526CC, brightness, 0x80);
    }
    Row_PutSprite(ot, sprite, color, 0, x, y);
    Prim_AddDrawMode(ot, 0x20);
}
#else
INCLUDE_ASM("asm/sim/ovl2/nonmatchings/bonus", BonusItems_DrawRow);
#endif

// Draws a row, every row being enabled.
int BonusItems_ListCallback(int event, MenuListEvent *ctx, int index)
{
    int result = 0;
    MenuList *list = ctx->list;
    int x = ctx->x;
    int y = ctx->y;
    void *ot = (char *)ctx->ot + 4;
    ArcadeMenu *menu = (ArcadeMenu *)list->userData;

    switch (event) {
    case MLE_INIT:
    case MLE_REVEAL:
    case MLE_CLOSE:
    case MLE_UPDATE:
        break;
    case MLE_DRAW: {
        int state;
        int shown;
        int closing;
        int level = ((list->fade << 7) / list->fadeMax) * ctx->brightness >> 7;

        if (level <= 0) {
            return result;
        }
        state = list->state;
        shown = state >= 0;
        closing = state < -1;
        if (shown | closing) {
            BonusItems_DrawRow(&g_ArcBonusCourses[index], menu, ot, x, y, level);
            return result;
        }
        break;
    }
    case MLE_LEAVE:
    case MLE_ENTER:
    case MLE_OPEN:
        break;
    case MLE_IS_ENABLED:
        result = 1;
        break;
    }
    return result;
}

#ifdef NON_MATCHING
// The list stops at the first course whose licence isn't earned.
void BonusItems_Enter(ArcadeMenu *menu, int back)
{
    int count = 0;
    ArcBonusCourse *course = g_ArcBonusCourses;
    ArcPage *page = (ArcPage *)menu->activePage;

    page->delay = 0x18;
    ButtonPrompt_Init(&g_Arc_BonusPrompt, &D_8005272C);
    while (course->name != 0) {
        if ((License_IsEarned_OVL2(course->license) ^ 1) != 0) {
            break;
        }
        course++;
        count++;
    }
    g_ArcList_Bonus.count = count;
    MenuList_Init(&g_ArcList_Bonus, (MenuListFn *)BonusItems_ListCallback, menu);
    g_Arc_BonusCourseCount = count;
}
#else
INCLUDE_ASM("asm/sim/ovl2/nonmatchings/bonus", BonusItems_Enter);
#endif

#ifdef NON_MATCHING
// Returns 2 to go back, and 1 once the next page is pushed.
int BonusItems_Update(ArcadeMenu *menu, int active)
{
    ArcPage *page = (ArcPage *)menu->activePage;
    PadState *pad = &menu->padState[0];
    int result = 0;
    int choice;

    if (page->delay > 0) {
        if (--page->delay == 0) {
            MenuList_Open(&g_ArcList_Bonus);
            ButtonPrompt_Open(&g_Arc_BonusPrompt);
        }
    }
    if (active == 0) {
        pad = 0;
    }
    choice = MenuList_Update(&g_ArcList_Bonus, pad);
    MenuList_ClampSelection(&g_ArcList_Bonus, 5, g_Arc_BonusCourseCount - 5);
    if (choice == -3) {
        Sfx_PlayMenu(6);
    }
    choice = ButtonPrompt_Update(&g_Arc_BonusPrompt, pad);
    switch (choice) {
    case -1:
        Sfx_PlayMenu(4);
        MenuList_Close(&g_ArcList_Bonus);
        ButtonPrompt_Close(&g_Arc_BonusPrompt);
        result = 2;
        break;
    case 0:
        Sfx_PlayMenu(3);
        MenuList_Close(&g_ArcList_Bonus);
        ButtonPrompt_Close(&g_Arc_BonusPrompt);
        ArcadeMenu_PushPage(menu, &g_ArcPage_BonusSelect);
        result = 1;
        break;
    }
    return result;
}
#else
INCLUDE_ASM("asm/sim/ovl2/nonmatchings/bonus", BonusItems_Update);
#endif

#ifdef NON_MATCHING
// The prompt, the list, the two sprites above and below it and the title.
void BonusItems_Draw(ArcadeMenu *menu, DrawLayer *layer)
{
    uint *ot = (uint *)layer->ot;
    TextCtx *text = menu->text;
    MenuList *list;
    int state;
    int shown;
    int closing;

    Text_Init(text, 0x1e);
    text->ot = ot;
    Text_SetFont(text, &D_80123810);
    *(uint *)&text->clut = (*(uint *)&text->clut & 0xff9fffff) | 0x200000;
    ButtonPrompt_Draw(&g_Arc_BonusPrompt, ot, text);
    list = &g_ArcList_Bonus;
    MenuList_Draw(list, ot + 2);
    state = list->state;
    shown = state >= 0;
    closing = state < -1;
    if (shown | closing) {
        int level = (list->fade << 7) / list->fadeMax;
        unsigned int rgb = level | level << 8 | level << 16;
        ArcSprite *sprite = D_80052658;
        int y = 0x66;
        PrimSprite *p = Prim_AddSprite(ot, rgb);

        *(int *)p = ((y - Half(sprite->h)) << 16) + 0x60 - Half(sprite->w);
        *(int *)&p->u = *(int *)sprite;
        *(int *)&p->w = *(int *)&sprite->w;
        Prim_AddDrawMode(ot, sprite->tpage);
        sprite++;
        p = Prim_AddSprite(ot, rgb);
        *(int *)p = ((y - Half(sprite->h)) << 16) + 0xfa - Half(sprite->w);
        *(int *)&p->u = *(int *)sprite;
        *(int *)&p->w = *(int *)&sprite->w;
        Prim_AddDrawMode(ot, sprite->tpage);
        text->rgb = Rgb_Lerp(D_800526B8, D_800526D0, level, 0x80);
        Text_DrawCentered(text, (signed char *)D_800F87B9, 0xb0, 0x8a, 1);
    }
}
#else
INCLUDE_ASM("asm/sim/ovl2/nonmatchings/bonus", BonusItems_Draw);
#endif

#ifdef NON_MATCHING
// Disables each entry until its condition is met, and starts the fade-in.
void BonusSelect_Enter(ArcadeMenu *menu, int back)
{
    ArcPage &page = *(ArcPage *)menu->activePage;

    page.delay = 0x18;
    *(unsigned char *)&D_80052754[0].kind |= 0x80;
    if (ArcBonus_AllCoursesWon()) {
        *(unsigned char *)&D_80052754[0].kind &= 0x7f;
    }
    *(unsigned char *)&D_80052754[1].kind |= 0x80;
    if (ArcBonus_IsStat349Set()) {
        *(unsigned char *)&D_80052754[1].kind &= 0x7f;
    }
    MenuList_Init_OVL2(&g_ArcMenu_Bonus);
    g_ArcMenu_Bonus.selected = 0;
    page.choice = 1;
}
#else
INCLUDE_ASM("asm/sim/ovl2/nonmatchings/bonus", BonusSelect_Enter);
#endif

// Returns 2 to go back, and 1 once the launch page is pushed.
int BonusSelect_Update(ArcadeMenu *menu, int active)
{
    ArcPage *page = (ArcPage *)menu->activePage;
    PadState *pad = &menu->padState[0];
    int result = 0;
    int fade;
    int choice;

    if (page->delay > 0) {
        if (--page->delay == 0) {
            MenuList_Open_OVL2(&g_ArcMenu_Bonus);
        }
    }
    fade = page->choice;
    if (fade > 0) {
        fade++;
        if (fade > 12) {
            fade = 12;
        }
    }
    if (fade < 0) {
        fade++;
    }
    page->choice = fade;
    if (active == 0) {
        pad = 0;
    }
    choice = MenuList_Update_OVL2(&g_ArcMenu_Bonus, pad);
    switch (choice) {
    case -4:
        Sfx_PlayMenu(0);
        return result;
    case -3:
        Sfx_PlayMenu(6);
        return result;
    case -2:
        break;
    case -1:
        Sfx_PlayMenu(4);
        page->choice = -0xc;
        MenuList_Close_OVL2(&g_ArcMenu_Bonus);
        result = 2;
        break;
    default:
        Sfx_PlayMenu(3);
        MenuList_Close_OVL2(&g_ArcMenu_Bonus);
        ArcadeBgm_Stop();
        g_Arc_BonusChoice = 0;
        if (choice != 0) {
            g_Arc_BonusChoice = 1;
        }
        ArcadeMenu_PushPage(menu, &g_ArcPage_BonusLaunch);
        result = 1;
        break;
    }
    return result;
}

// The list, then the title sprite fading with it.
void BonusSelect_Draw(ArcadeMenu *menu, DrawLayer *layer)
{
    uint *ot = (uint *)layer->ot;
    TextCtx *text = menu->text;
    ArcPage *page = (ArcPage *)menu->activePage;
    int level;
    int fade;
    int xy;
    PrimSprite *p;
    ArcSprite *sprite;

    MenuList_Draw_OVL2(&g_ArcMenu_Bonus, ot, text);
    fade = page->choice;
    if (fade < 0) {
        fade = -fade;
    }
    level = (fade << 7) / 12;
    p = Prim_AddSprite(ot, level | level << 8 | level << 16);
    sprite = &D_80052604;
    xy = ((0x96 - Half(sprite->h)) << 16) + 0xb0;
    *(int *)p = xy - Half(sprite->w);
    *(int *)&p->u = *(int *)sprite;
    *(int *)&p->w = *(int *)&sprite->w;
    Prim_AddDrawMode(ot, sprite->tpage);
}

// 24-frame delay.
void BonusLaunch_Enter(void *menu, int back)
{
    *(unsigned int *)(*(int *)((int)menu + 0x1c8) + 0x14) = 0x18;
}

// After the delay leaves the menu for the chosen bonus.
int BonusLaunch_Update(ArcadeMenu *menu, int active)
{
    ArcPage *page = (ArcPage *)menu->activePage;
    int result = 0;

    if (page->delay > 0) {
        if (--page->delay == 0) {
            result = 5;
            if (g_Arc_BonusChoice != 0) {
                result = 6;
            }
        }
    }
    return result;
}

// Empty.
void BonusLaunch_Draw(void *menu, DrawLayer *layer)
{
}
}
