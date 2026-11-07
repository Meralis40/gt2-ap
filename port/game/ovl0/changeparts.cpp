// OVL0 80052838-80057628 (changeparts): Parts change and settings pages.
// The functions that don't match yet are the original assembly.
#include "common.h"
#include "gamestate.h"

// A choice of a part (a row of the variant list).
struct PartsVariant {
    char *name;
    char *help;
    short part;
    short pad;
};

// A part of a group.
struct PartsItem {
    char *name;
    char *help;
    PartsVariant *variants;
    short slot; /* CarWork.partSlots */
    signed char icon;
    unsigned char flags; /* 0x80 the power is limited; 0x40 greyed, follows the event's turbo rule */
};

// A page of Change Parts.
struct PartsGroup {
    char *name;
    char *help;
    unsigned char rgb[4];
    signed char icon;
    unsigned char pad[3];
    PartsItem items[8];
};

// The Change Parts menu, at g_Settings_Work.
struct ChangePartsMenu {
    short x;
    short y;
    signed char state; /* 0 groups, 1 items, 2 variants */
    signed char group;
    signed char item;
    unsigned char pad7;
    short groupCount;
    short scroll;
    short arrowTimer;
    short open;
    short blink;
    unsigned char pad12[2];
    WipeBar bar;
    unsigned char prevRgb[4];
    unsigned char rgb[4];
    unsigned char graph[0x14]; /* PowerGraph */
    unsigned char graphBuffer[0x2a54 - 0x4c];
    short parts[2][0x20]; /* slot and variant, for PowerGraph_Start */
};

// A sprite: UV and CLUT, size, texture page.
struct PartsSprite {
    unsigned int uvClut;
    unsigned short w;
    unsigned short h;
    unsigned short tpage;
    unsigned short pad;
};

// A row of the value list of Parts Setting.
struct SettingRow {
    char *label;
    char *unit;
    unsigned char format;
    unsigned char pad9;
    SettingRange range;
    short state; /* -1 hidden */
    short blink;
    short pad16;
};

// A setting.
struct SettingItem {
    char *name;
    char *help;
    char *unit;
    signed char kind;
    unsigned char format; /* low bits Setting_Format's; 0x80: has a front and a rear value */
};

struct SettingEntry {
    SettingItem *item;
    short id;
    short pad;
};

// A page of Parts Setting.
struct SettingsGroup {
    char *name;
    char *help;
    unsigned char rgb[4];
    signed char icon;
    unsigned char pad[3];
    SettingEntry entries[16];
};

// The Parts Setting menu, at g_Settings_Work + 0x2a7c.
struct PartsSettingMenu {
    short x;
    short y;
    signed char state;
    signed char group;
    signed char item;
    unsigned char pad7;
    short groupCount;
    short open;
    short scroll;
    short arrowTimer;
    short blink;
    SettingRange values[16];
    signed char counts[16];
    unsigned char pad_a2[2];
    SettingRow rows[16];
    WipeBar bar;
    unsigned char prevRgb[4];
    unsigned char rgb[4];
};

// A car's setup for Settings_Apply: its position in the garage.
#define CAR_FROM_GARAGE(garage, index) ((char *)(garage) + (index) * 0xa4 + 4)

extern "C" {
extern void *g_Settings_Work;
extern GameState g_GameState;
extern RaceSetup g_RaceSetup;
extern char g_Race_IsDemo;
extern QuickMenu *g_QuickMenu;
extern void *g_OVL0_KanjiFont;
extern Font g_OVL0_FontA;
extern Font g_OVL0_FontB;
extern Font g_OVL0_FontD;
extern short g_ChangeParts_Group;
extern short g_ChangeParts_Item;
extern signed char g_ChangeParts_PowerLimited;
extern int g_ChangeParts_ShowHelp;
extern char *g_ChangeParts_HelpTitle;
extern char *g_ChangeParts_HelpText;
extern PartsGroup *g_ChangeParts_GroupsAll[];
extern PartsGroup *g_ChangeParts_GroupsTurbo[];
extern PartsGroup *g_ChangeParts_GroupsNa[];
extern MenuList g_ChangeParts_VariantList;
extern WipeBar g_ChangeParts_HelpBar;
extern PowerGraphView g_ChangeParts_PowerGraph;
extern short g_PartsSetting_Group;
extern short g_PartsSetting_Item;
extern short g_PartsSetting_ValueCount;
extern int g_PartsSetting_ShowHelp;
extern char *g_PartsSetting_HelpTitle;
extern char *g_PartsSetting_HelpText;
extern SettingsGroup *g_PartsSetting_Groups[];
extern MenuList g_PartsSetting_ValueList;
extern WipeBar g_PartsSetting_HelpBar;
extern QuickMenuPage g_Page_ChangeParts;
extern QuickMenuPage g_Page_PartsSetting;
extern WipeBar D_8005C3C0;
extern WipeBar D_8005D1A4;
extern short D_8005C428;
extern short D_8005D1A0;
extern PartsSprite D_8005B830[];
extern PartsSprite D_8005B89C;
extern PartsSprite D_8005B8A8;
extern PartsSprite D_8005B8B4[];
extern unsigned char D_8005B95C[];
extern unsigned char D_8005B960[];
extern unsigned int D_8005B964;
extern unsigned int D_8005B968;
extern unsigned char D_8005B96C[];
extern unsigned char D_8005B970[];
extern unsigned int D_8005B974;
extern unsigned char D_8005B978[];
extern unsigned char D_8005B97C[];
extern unsigned char D_8005C46C[];
extern unsigned char D_8005C470[];
extern unsigned int D_8005C474;
extern unsigned char D_8005C478[];
extern unsigned char D_8005C47C[];
extern unsigned char D_8005C480[];
extern unsigned char D_8005C484[];
extern unsigned char D_8005C488[];
extern unsigned char D_8005C48C[];
extern unsigned char D_8005C490[];
extern unsigned int D_8005C494;
extern unsigned int D_8005C498;
extern unsigned char D_8005C49C[];
extern unsigned int D_8005C4A0;
extern char *D_8005C4A4[];
extern SettingItem D_8005C5F0;
extern SettingItem D_8005C600;
extern SettingItem D_8005C610;
extern SettingItem D_8005C620;
extern SettingItem D_8005C630;
extern SettingItem D_8005C640;
extern SettingsGroup D_8005C650;
extern SettingsGroup D_8005C6E0;
extern SettingsGroup D_8005C770;
extern SettingsGroup D_8005CFE0;
extern SettingsGroup D_8005D070;
extern SettingsGroup *D_8005D100[];
extern SettingsGroup *D_8005D120[];
extern char D_8005A9A8[];
extern char D_8005A9AC[];
extern char D_8005A9B0[];
extern char D_8005A9B8[];
extern char D_8005A9C0[];
extern char D_8005A9E0[];
extern char sR_L1PARTSSETTING[];
extern char sR_R1CHANGEPARTS[];
extern char sR_NhpNrpm[];
extern char sR_PowerRestrictedToNhp[];
extern char sR_Sports_71ee[];
extern char sR_Soft[];
extern char sR_Hard[];
extern char sR_Front[];
extern char sR_Rear[];
extern char sR_Final[];

int PowerGraph_Step(void *g);
void PowerGraph_GetMax(void *g, int *out);
short *PowerGraph_GetPowerCurve(void *g, int i);
short *PowerGraph_GetTorqueCurve(void *g, int i);
void PowerGraph_GetPeak(void *g, unsigned short *out, int i);
int PowerGraph_GetPower(void *g, int i);
void PowerGraph_Start(void *g, int count, CarWork *work, void *parts);
void Kanji_DrawFaded(void *ot, char *s, int x, int y, int fade, unsigned int rgb, int mode);
void QuickMenu_ReplacePage(QuickMenu *menu, QuickMenuPage *page);
void ChangeParts_SetHelpText(char *sjis);
void ChangeParts_SelectItem(ChangePartsMenu *menu, int item);
void PartsSetting_SelectGroup(PartsSettingMenu *menu, int group);
void PartsSetting_SelectItem(PartsSettingMenu *menu, int item);
void Settings_Apply(void);
int ChangeParts_VariantCallback(MenuListEventId event, MenuListEvent *ev, int row);
int PartsSetting_ValueCallback(MenuListEventId event, MenuListEvent *ev, int row);
int ChangeParts_Update(ChangePartsMenu *menu, PadState *pad, int showHelp);
int PartsSetting_Update(PartsSettingMenu *menu, PadState *pad, int showHelp);

// The car being set up, in the race's memory, as the game reaches it.
static inline CarWork *Settings_CarWork(void)
{
    int offset = 0xc5397;
    char *base = &g_Race_IsDemo;

    return (CarWork *)((int)(base + offset) & -4);
}

// g_RaceSetup, as the game reaches it: g_GameState plus 0xbf7c, added at run time.
static inline RaceSetup *Settings_RaceSetup(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;

    return (RaceSetup *)(base + offset);
}

// Sets the help's title; in Japanese, also caches its glyphs and the text's.
void ChangeParts_SetHelpTitle(char *sjis)
{
    g_ChangeParts_HelpTitle = sjis;
    if (g_ChangeParts_ShowHelp != 0 && g_GameState.language == 0) {
        void *font = g_OVL0_KanjiFont;

        KanjiFont_Reset((KanjiFont *)font);
        KanjiFont_CacheString(font, sjis);
        KanjiFont_CacheString(font, g_ChangeParts_HelpText);
    }
}

// Sets the help's text; in Japanese, also caches its glyphs and the title's.
void ChangeParts_SetHelpText(char *sjis)
{
    g_ChangeParts_HelpText = sjis;
    if (g_ChangeParts_ShowHelp != 0 && g_GameState.language == 0) {
        void *font = g_OVL0_KanjiFont;

        KanjiFont_Reset((KanjiFont *)font);
        KanjiFont_CacheString(font, g_ChangeParts_HelpTitle);
        KanjiFont_CacheString(font, sjis);
    }
}

// CarWork_SetPartSlot(work, category, variant).
void ChangeParts_Install(int category, int variant, CarWork *work)
{
    CarWork_SetPartSlot(work, category, variant);
}

int PartsGroup_CountItems(PartsGroup *group)
{
    int i;

    for (i = 0; i < 8; i++) {
        if (group->items[i].name == 0) {
            return i;
        }
    }
    return 8;
}

int PartsGroup_CountVariants(PartsGroup *group, int item)
{
    int count = 0;
    PartsVariant *variant = group->items[item].variants;

    for (;;) {
        char *name = variant->name;

        if (name == 0) {
            break;
        }
        variant++;
        count++;
    }
    return count;
}

// The parts of a group with their installed variants.
void ChangeParts_DrawItems(PartsGroup *group, uint *ot, int selected, int x, int y, int fade, int blink, CarWork *work)
{
    TextCtx text;
    unsigned int rgb;
    int count = PartsGroup_CountItems(group);
    unsigned char *rgbBytes;
    TextCtx *ctx;
    int grey;
    int greyG;
    int greyB;
    int i;
    PartsItem *item = group->items;

    Text_Init(&text, 6);
    i = 0;
    rgbBytes = (unsigned char *)&rgb;
    grey = fade / 6;
    ctx = &text;
    *(unsigned int *)&ctx->clut = (*(unsigned int *)&ctx->clut & 0xff9fffff) | 0x200000;
    ctx->ot = ot;
    greyG = grey | (grey << 8);
    greyB = grey << 16;
    for (; i < count; i++) {
        int stage = work->partSlots[item->slot];
        PartsVariant *variants = item->variants;
        int offset = 0;
        int rowY;
        PartsSprite *sprite;
        PrimSprite *prim;

        rgb = 0x278500a;
        if ((item->flags >> 6) & 1) {
            rgb = 0x20c3060;
            offset = 8;
        }
        Text_SetFont(&text, &g_OVL0_FontB);
        ctx->rgb = Rgb_Lerp(D_8005B960, rgbBytes, fade, 0x80);
        rowY = y + offset;
        Text_DrawString(ctx, (signed char *)item->name, x + 4, rowY, 1);
        Text_SetFont(ctx, &g_OVL0_FontD);
        ctx->rgb = Rgb_Lerp(D_8005B960, D_8005B970, fade, 0x80);
        Text_DrawString(ctx, (signed char *)variants[stage].name, x + 0x94, rowY, 1);
        sprite = &D_8005B89C;
        if (i == selected) {
            int t = 8 - blink;

            if (t < 0) {
                t = 0;
            }
            rgb = Rgb_Lerp(D_8005B95C, group->rgb, fade, 0x180);
            rgb = Rgb_Lerp(rgbBytes, D_8005B96C, t, 8);
            prim = Prim_AddSprite(ot, rgb);
        } else {
            prim = Prim_AddSprite(ot, greyG | greyB);
        }
        *(int *)prim = x + ((rowY - 0x18) << 16) - 8;
        ((int *)prim)[1] = D_8005B89C.uvClut;
        ((int *)prim)[2] = *(int *)&sprite->w;
        item++;
        Prim_AddDrawMode(ot, sprite->tpage | 0x220);
        x += 4;
        y += 0x20;
    }
}

// A group, of the list for what the car can take.
void *ChangeParts_GetGroup(int index)
{
    CarWork *work = Settings_CarWork();
    PartsGroup **groups;

    if (CarWork_HasTurboKits(work) != 0) {
        if (CarWork_HasNaTune(work) != 0) {
            groups = g_ChangeParts_GroupsAll;
        } else {
            groups = g_ChangeParts_GroupsTurbo;
        }
    } else {
        groups = g_ChangeParts_GroupsNa;
    }
    return groups[index];
}

int ChangeParts_CountGroups(void)
{
    return CarWork_HasTurboKits(Settings_CarWork()) != 0 ? 8 : 7;
}

// The rows of the variant list: help, availability and drawing.
int ChangeParts_VariantCallback(MenuListEventId event, MenuListEvent *ev, int row)
{
    MenuList *list = ev->list;
    CarWork *work = Settings_CarWork();
    ChangePartsMenu *menu = (ChangePartsMenu *)list->userData;
    int result = 0;
    PartsGroup *group = (PartsGroup *)ChangeParts_GetGroup(g_ChangeParts_Group);
    PartsVariant *variant;
    int x;
    int textY;
    uint *ot;
    TextCtx text;
    unsigned int rgbGroup;
    unsigned int rgb;
    unsigned int rgbText;

    rgbGroup = *(unsigned int *)group->rgb;
    x = ev->x - 0x30;
    variant = &group->items[g_ChangeParts_Item].variants[row];
    textY = ev->y + 8;
    ot = (uint *)ev->ot;
    Text_Init(&text, 6);
    *(unsigned int *)&text.clut = (*(unsigned int *)&text.clut & 0xff9fffff) | 0x200000;
    text.ot = ot;
    Text_SetFont(&text, &g_OVL0_FontB);
    switch (event) {
    case 0:
        break;
    case 6:
    case 7:
        ChangeParts_SetHelpText(variant->help);
        return result;
    case 8: {
        int slot;
        RaceSetup *setup;
        int limit;

        if (list->state < 0) {
            break;
        }
        slot = group->items[g_ChangeParts_Item].slot;
        result = CarWork_GetPartIdx(work, slot, row) >= 0;
        if (row > 0) {
            char *garage;

            setup = Settings_RaceSetup();
            garage = (char *)&g_GameState + 0x7c9c;
            if (setup->unk582 == 0) {
                garage = (char *)&g_GameState + 0x3c74;
            }
            if (Car_HasPart((Car *)CAR_FROM_GARAGE(garage, setup->garageSlot), variant->part) ^ 1) {
                result = 0;
            }
        }
        limit = Settings_RaceSetup()->maxPower;
        if (limit > 0 && (g_ChangeParts_PowerLimited & 1)) {
            void *graph = menu->graph;

            if (PowerGraph_GetPower(graph, row + 1) < 0) {
                result = 0;
            }
            if (limit < PowerGraph_GetPower(graph, row + 1)) {
                result = 0;
            }
        }
        if ((unsigned int)(slot - 3) < 2) {
            if (Settings_RaceSetup()->flags & 1) {
                if (row != 7) {
                    return result;
                }
                result = 0;
            } else {
                if (row == 7) {
                    return result;
                }
                result = 0;
            }
        }
        break;
    }
    case 4: {
        int fade;
        int bright;
        int owned;
        int blink;
        unsigned int rgbTile;
        unsigned int rgbLine;

        if (list->state >= 0 && row == list->selected && g_ChangeParts_ShowHelp != 0) {
            if (g_GameState.language == 0) {
                Kanji_DrawFaded(ot, variant->help, 0xb0, 0x1a4, (list->fade << 7) / list->fadeMax, D_8005B974, 1);
            } else {
                Text_SetFont(&text, &g_OVL0_FontD);
                text.rgb = Rgb_Lerp(D_8005B95C, D_8005B970, (list->fade << 7) / list->fadeMax, 0x80);
                Text_DrawCentered(&text, (signed char *)variant->help, 0xb0, 0x1bc, 1);
            }
        }
        Text_SetFont(&text, &g_OVL0_FontB);
        {
            if (((list->state >= 0) | (list->state < -1)) == 0) {
                return result;
            }
        }
        fade = (list->fade << 7) / list->fadeMax;
        if (fade <= 0) {
            break;
        }
        blink = menu->blink;
        owned = CarWork_GetPartIdx(work, group->items[g_ChangeParts_Item].slot, row) >= 0;
        rgbText = *(unsigned int *)D_8005B970;
        if (list->state >= 0) {
            x -= fade - 0x80;
        }
        if (ev->row == 0) {
            int top = list->y;
            PartsSprite *icon = &D_8005B8B4[group->items[g_ChangeParts_Item].icon];
            PrimSprite *prim = Prim_AddSprite(ot, fade | (fade << 8) | (fade << 16) | 0x2000000);
            PrimLineF4 *line;

            *(int *)prim = x + (top << 16) - 0x2c;
            ((int *)prim)[1] = icon->uvClut;
            ((int *)prim)[2] = *(int *)&icon->w;
            Prim_AddDrawMode(ot, icon->tpage | 0x220);
            line = Prim_AddLineF4(ot, Rgb_Lerp(D_8005B95C, (unsigned char *)&rgbGroup, fade, 0x100));
            ((short *)line)[10] = ((short *)line)[8] = ((short *)line)[2] = x - 0x2d;
            ((short *)line)[6] = ((short *)line)[4] = x - 6;
            ((short *)line)[11] = ((short *)line)[5] = ((short *)line)[3] = top - 1;
            ((short *)line)[9] = ((short *)line)[7] = top + 0x42;
            Prim_AddDrawMode(ot, 0x220);
        }
        bright = fade * ev->brightness >> 7;
        if (ev->enabled == 0) {
            rgbText = D_8005B968;
        }
        if (owned == 0) {
            rgbText = D_8005B964;
        }
        rgbTile = Rgb_Lerp(D_8005B95C, (unsigned char *)&rgbGroup, bright, 0x300);
        rgbLine = Rgb_Lerp(D_8005B95C, (unsigned char *)&rgbGroup, bright, 0x180);
        if (row == list->selected) {
            int t = 0x18 - blink;

            if (t < 0) {
                t = 0;
            }
            rgbTile = Rgb_Lerp(D_8005B95C, (unsigned char *)&rgbGroup, bright, 0x180);
            rgbTile = Rgb_Lerp((unsigned char *)&rgbTile, D_8005B96C, t, 0x18);
        }
        text.rgb = Rgb_Lerp(D_8005B95C, (unsigned char *)&rgbText, bright, 0x80);
        Text_DrawString(&text, (signed char *)variant->name, x + 4, textY, 1);
        {
            RECT *tile = Prim_AddTile(ot, rgbTile);
            PrimLineF4 *line;
            int top;

            tile->x = x + 2;
            tile->y = textY - 0x14;
            tile->w = 0x5d;
            tile->h = 0x17;
            line = Prim_AddLineF4(ot, rgbLine);
            ((short *)line)[10] = ((short *)line)[8] = ((short *)line)[2] = x;
            ((short *)line)[6] = ((short *)line)[4] = x + 0x60;
            top = textY - 0x16;
            ((short *)line)[11] = ((short *)line)[5] = ((short *)line)[3] = top;
            ((short *)line)[9] = ((short *)line)[7] = textY + 4;
            Prim_AddDrawMode(ot, 0x220);
            tile = Prim_AddTile(ot, 0x2000000);
            tile->w = 0x60;
            tile->x = x;
            tile->y = top;
            tile->h = 0x1a;
            Prim_AddDrawMode(ot, 0x200);
        }
        break;
    }
    }
    return result;
}

// Selects a group: keeps the previous colour for the fade and updates the help title.
void ChangeParts_SelectGroup(void *menu, int group)
{
    void *pvVar1;
    unsigned int uVar2;

    pvVar1 = ChangeParts_GetGroup(group);
    *(unsigned int *)((int)menu + 0x30) = *(unsigned int *)((int)menu + 0x34);
    uVar2 = *(unsigned int *)((int)pvVar1 + 8);
    *(char *)((int)menu + 5) = (char)group;
    *(unsigned int *)((int)menu + 0x34) = uVar2;
    ChangeParts_SetHelpTitle(*(char **)((int)pvVar1 + 4));
}

void ChangeParts_SelectItem(ChangePartsMenu *menu, int item)
{
    PartsGroup *group;

    menu->item = item;
    menu->blink = 0;
    group = (PartsGroup *)ChangeParts_GetGroup(g_ChangeParts_Group);
    ChangeParts_SetHelpText(group->items[item].help);
}

void ChangeParts_Init(ChangePartsMenu *menu, int x, int y)
{
    menu->x = x;
    g_ChangeParts_Group = 0;
    g_ChangeParts_Item = 0;
    g_ChangeParts_ShowHelp = 1;
    g_ChangeParts_HelpTitle = D_8005A9A8;
    g_ChangeParts_HelpText = D_8005A9A8;
    menu->y = y;
    menu->state = 0;
    menu->open = -1;
    menu->arrowTimer = 0;
    menu->blink = 0;
    *(int *)menu->rgb = 0;
    ChangeParts_SelectGroup(menu, 0);
    ChangeParts_SelectItem(menu, 0);
    menu->groupCount = ChangeParts_CountGroups();
    menu->bar = D_8005C3C0;
    menu->bar.t = -1;
    MenuList_Init(&g_ChangeParts_VariantList, ChangeParts_VariantCallback, menu);
    D_8005C428 = -1;
    PowerGraph_Reset(menu->graph, menu->graphBuffer);
    PowerGraphView_Init(&g_ChangeParts_PowerGraph);
}

// Starts the open animation.
void ChangeParts_Open(void *menu)
{
    *(unsigned short *)((int)menu + 10) = 0;
    *(unsigned short *)((int)menu + 0x2c) = 0;
    *(unsigned short *)((int)menu + 0xe) = 0;
}

void ChangeParts_Close(ChangePartsMenu *menu)
{
    WipeBar *bar = &menu->bar;

    bar->t = ~bar->duration;
    menu->open = -1;
}

// One frame of the menu; returns the sound to play and page action, negative.
int ChangeParts_Update(ChangePartsMenu *menu, PadState *pad, int showHelp)
{
    int result = -1;
    CarWork *work = Settings_CarWork();
    void *graph = menu->graph;
    unsigned int buttons;
    int value;

    g_ChangeParts_ShowHelp = showHelp;
    if (PowerGraph_Step(graph) != 0) {
        int max[3];

        PowerGraph_GetMax(graph, max);
        g_ChangeParts_PowerGraph.maxRpm = max[0];
        g_ChangeParts_PowerGraph.maxPower = max[1];
        g_ChangeParts_PowerGraph.maxTorque = max[2];
        PowerGraphView_Show(&g_ChangeParts_PowerGraph);
    }
    PowerGraphView_Update(&g_ChangeParts_PowerGraph);
    WipeBar_Update(&menu->bar);
    if (menu->scroll < 0) {
        menu->scroll++;
    }
    if (menu->scroll > 0) {
        menu->scroll--;
    }
    if (++menu->blink > 0x3c) {
        menu->blink = 0;
    }
    if (++menu->arrowTimer > 0x2d) {
        menu->arrowTimer = 0;
    }
    WipeBar_Update(&g_ChangeParts_HelpBar);
    if (menu->open < 0) {
        MenuList_Update(&g_ChangeParts_VariantList, 0);
        return -1;
    }
    switch (menu->state) {
    case 0:
        MenuList_Update(&g_ChangeParts_VariantList, 0);
        if (pad == 0) {
            return result;
        }
        buttons = pad->pressed;
        if (buttons & 0x10) {
            goto toSetting;
        }
        if (buttons & 0x500) {
            result = -4;
            g_ChangeParts_ShowHelp = 0;
            break;
        }
        if (buttons & 0xa08) {
            result = -5;
            g_ChangeParts_Group = menu->group;
            menu->state = 1;
            ChangeParts_SelectItem(menu, 0);
            break;
        }
        buttons |= pad->repeat;
        value = menu->group;
        if (buttons & 1) {
            int old = value;

            value--;
            if (value < 0) {
                value = 0;
            }
            if (value != old) {
                menu->scroll = -6;
            }
        }
        if (buttons & 2) {
            value++;
            if (value >= menu->groupCount) {
                value = menu->groupCount - 1;
            }
            if (value != menu->group) {
                menu->scroll = 6;
            }
        }
        if (value != menu->group) {
            ChangeParts_SelectGroup(menu, value);
            result = -2;
        }
        break;
    case 1:
        MenuList_Update(&g_ChangeParts_VariantList, 0);
        if (pad == 0) {
            return result;
        }
        buttons = pad->pressed;
        if (buttons & 0x10) {
        toSetting:
            result = -8;
            g_ChangeParts_ShowHelp = 0;
            break;
        }
        if (buttons & 0x504) {
            result = -6;
            menu->state = 0;
            break;
        }
        if (buttons & 0xa00) {
            PartsGroup *group;
            int count;
            PartsItem *item;

            g_ChangeParts_Item = menu->item;
            group = (PartsGroup *)ChangeParts_GetGroup(g_ChangeParts_Group);
            result = group->items[g_ChangeParts_Item].slot;
            count = PartsGroup_CountVariants(group, g_ChangeParts_Item);
            item = &group->items[g_ChangeParts_Item];
            g_ChangeParts_PowerLimited = item->flags >> 7;
            if ((item->flags >> 6 ^ 1) & 1) {
                switch ((Settings_RaceSetup()->flags >> 1) & 3) {
                case 1:
                    if ((unsigned int)(result - 0xf) < 2) {
                        return -7;
                    }
                    break;
                case 2:
                    if (result == 0xd) {
                        return -7;
                    }
                    break;
                }
                D_8005C428 = 0;
                g_ChangeParts_VariantList.count = count;
                g_ChangeParts_VariantList.selected = work->partSlots[result];
                MenuList_Open(&g_ChangeParts_VariantList);
                if (g_ChangeParts_PowerLimited != 0) {
                    int i;

                    for (i = 0; i < count; i++) {
                        menu->parts[i][0] = result;
                        menu->parts[i][1] = i;
                    }
                    PowerGraph_Start(graph, count, work, menu->parts);
                }
                menu->blink = 0;
                menu->state = 2;
                return -5;
            }
            return -7;
        }
        buttons |= pad->repeat;
        value = menu->item;
        if (buttons & 1) {
            value--;
            if (value < 0) {
                value = 0;
            }
        }
        {
            int count = PartsGroup_CountItems((PartsGroup *)ChangeParts_GetGroup(g_ChangeParts_Group));

            if (buttons & 2) {
                value++;
                if (value >= count) {
                    value = count - 1;
                }
            }
        }
        if (value != menu->item) {
            ChangeParts_SelectItem(menu, value);
            result = -2;
        }
        break;
    case 2: {
        int chosen = MenuList_Update(&g_ChangeParts_VariantList, pad);

        switch (chosen) {
        case -4:
            return -7;
        case -3:
            menu->blink = 0;
            result = -2;
            break;
        case -2:
            break;
        case -1:
            menu->blink = 0;
            result = -6;
            g_ChangeParts_HelpBar.t = ~g_ChangeParts_HelpBar.duration;
            MenuList_Close(&g_ChangeParts_VariantList);
            PowerGraphView_Close(&g_ChangeParts_PowerGraph);
            PowerGraph_Clear(graph);
            menu->state = 1;
            ChangeParts_SelectItem(menu, menu->item);
            break;
        default: {
            PartsGroup *group = (PartsGroup *)ChangeParts_GetGroup(g_ChangeParts_Group);

            result = -5;
            ChangeParts_Install(group->items[g_ChangeParts_Item].slot, chosen, work);
            menu->blink = 0;
            g_ChangeParts_HelpBar.t = ~g_ChangeParts_HelpBar.duration;
            MenuList_Close(&g_ChangeParts_VariantList);
            PowerGraphView_Close(&g_ChangeParts_PowerGraph);
            PowerGraph_Clear(graph);
            menu->state = 1;
            ChangeParts_SelectItem(menu, menu->item);
            break;
        }
        }
        break;
    }
    default:
        return result;
    }
    return result;
}

void ChangeParts_Draw(void *self, unsigned int *ot)
{
    ChangePartsMenu *menu = (ChangePartsMenu *)self;
    TextCtx text;
    unsigned short peak[6];
    char buf[0x40];
    char line[0x80];
    int arrowA;
    int arrowB;
    int group = menu->group;
    PartsGroup *current = (PartsGroup *)ChangeParts_GetGroup(group);
    int rows = 4;
    int prev = -1;
    int next = -1;
    int scrollY = menu->y + menu->scroll * 10 - 0x1c;
    int menuX = menu->x;
    TextCtx *ctx;
    WipeBar *bar = &g_ChangeParts_HelpBar;
    MenuList *list = &g_ChangeParts_VariantList;
    void *graph = menu->graph;
    int level;
    int i;
    int fadeIn;
    int fadeOut;
    int selected;
    int itemBlink;
    uint *ot2;

    Text_Init(&text, 6);
    ctx = &text;
    *(unsigned int *)&ctx->clut = (*(unsigned int *)&ctx->clut & 0xff9fffff) | 0x200000;
    ctx->ot = ot;
    WipeBar_Draw(bar, ot, 0, 0xd2);
    Prim_AddDrawMode(ot, 0x220);
    MenuList_Draw(list, ot);
    if (bar->t >= 0) {
        level = WipeBar_GetLevel(bar);
        Text_SetFont(ctx, &g_OVL0_FontD);
        ctx->rgb = Rgb_Lerp(D_8005B95C, D_8005B970, level, 0x80);
        Text_DrawCentered(ctx, (signed char *)current->items[g_ChangeParts_Item].name, 0xb0, 0xec, 1);
    }
    if (((signed char *)graph)[0xa] != 0) {
        PowerGraphView_DrawCurve(&g_ChangeParts_PowerGraph, ot, PowerGraph_GetPowerCurve(graph, 0), 0, 0x30);
        PowerGraphView_DrawCurve(&g_ChangeParts_PowerGraph, ot, PowerGraph_GetTorqueCurve(graph, 0), 1, 0x30);
    }
    if (menu->state == 2) {
        int row = list->selected + 1;

        if (((signed char *)graph)[row + 0xa] != 0) {
            PowerGraphView_DrawCurve(&g_ChangeParts_PowerGraph, ot, PowerGraph_GetPowerCurve(graph, row), 0, 0x80);
            PowerGraphView_DrawCurve(&g_ChangeParts_PowerGraph, ot, PowerGraph_GetTorqueCurve(graph, row), 1, 0x80);
            PowerGraph_GetPeak(graph, peak, row);
            Text_SetFont(ctx, &g_OVL0_FontA);
            ctx->rgb = Rgb_Lerp(D_8005B95C, D_8005B978, (list->fade << 7) / list->fadeMax, 0x80);
            sprintf(buf, sR_NhpNrpm, peak[6 - 0], peak[7 - 0]);
            Text_DrawStringMonoDigits(ctx, (signed char *)buf, 0x78, 0x186, 0, -1, 0);
            sprintf(buf, sR_NhpNrpm + 0xb, peak[8] / 10, peak[8] % 10, peak[9]);
            Text_DrawStringMonoDigits(ctx, (signed char *)buf, 0x78, 0x19a, 0, -1, 0);
        }
    }
    PowerGraphView_DrawGrid(&g_ChangeParts_PowerGraph, ot);
    i = group - 1;
    fadeOut = (menu->scroll << 7) / 6;
    if (fadeOut < 0) {
        fadeOut = -fadeOut;
    }
    fadeIn = 0x80 - fadeOut;
    level = WipeBar_GetLevel(&menu->bar);
    menu->bar.rgbTop = Rgb_Lerp(menu->prevRgb, menu->rgb, fadeIn, 0x80);
    if ((menu->bar.t >= 0) | (menu->bar.t < -1)) {
        int bright = WipeBar_GetLevel(&menu->bar);

        fadeIn = bright * fadeIn >> 7;
        fadeOut = bright * fadeOut >> 7;
        if (menu->scroll < 0) {
            prev = group + 3;
            rows++;
        }
        if (menu->scroll > 0) {
            i = group - 2;
            next = group + 2;
            scrollY -= 0x3c;
            rows++;
        }
        rows += i;
        ot2 = ot + 1;
        for (; i < rows; i++) {
            if (i >= 0 && i < menu->groupCount) {
                PartsGroup *g = (PartsGroup *)ChangeParts_GetGroup(i);
                PartsSprite *icon = &D_8005B830[g->icon];
                int shade = bright;
                PrimSprite *prim;

                if (i == next) {
                    shade = fadeIn;
                }
                if (i == prev) {
                    shade = fadeOut;
                }
                if (i != menu->group) {
                    shade >>= 2;
                }
                prim = Prim_AddSprite(ot2, shade | (shade << 8) | (shade << 16));
                *(int *)prim = menuX - (((short)icon->w >> 1) - 0x2a) + ((scrollY - ((short)icon->h >> 1)) << 16);
                ((int *)prim)[1] = icon->uvClut;
                ((int *)prim)[2] = *(int *)&icon->w;
                Prim_AddDrawMode(ot2, icon->tpage | 0x20);
            }
            scrollY += 0x3c;
        }
        if (menu->state >= 0 && menu->state < 2 && g_ChangeParts_ShowHelp != 0) {
            int width;

            *(unsigned int *)&ctx->clut = (*(unsigned int *)&ctx->clut & 0xff9fffff) | 0x200000;
            Text_SetFont(ctx, &g_OVL0_FontB);
            ctx->rgb = 0x2503c28;
            width = Text_DrawStringRight(ctx, (signed char *)sR_L1PARTSSETTING, 0x14c, 0x68, 0);
            Menu_DrawSideArrow(&arrowA, ot, 0x149 - width, 0x5f, -5, 10, menu->arrowTimer);
        }
        {
            int top = menu->y;
            int shade = level * 0x60 / 0x80;
            int titleX;

            Text_SetFont(ctx, &g_OVL0_FontD);
            titleX = menuX + 0x44;
            ctx->rgb = shade | (shade << 8) | (shade << 16) | 0x2000000;
            *(unsigned int *)&ctx->clut = (*(unsigned int *)&ctx->clut & 0xff9fffff) | 0x200000;
            Text_DrawString(ctx, (signed char *)current->name, titleX, top + 0x1e, 1);
            if (g_GameState.language == 0 && g_ChangeParts_ShowHelp != 0) {
                Kanji_DrawFaded(ot + 1, current->help, titleX, top + 0x26, 0, D_8005B974, 0);
            }
        }
        {
            uint *ot3 = ot + 1;
            RECT *tile = Prim_AddTile(ot3, 0);

            tile->x = menuX + 0x1a;
            tile->w = 0x20;
            tile->h = 0x38;
            tile->y = menu->y + 4;
            Prim_AddDrawMode(ot3, 0x200);
            WipeBar_Draw(&menu->bar, ot3, menuX, menu->y);
            Prim_AddDrawMode(ot3, 0x200);
        }
        selected = -1;
        itemBlink = menu->blink;
        switch (menu->state) {
        case 0:
            if (group > 0) {
                Menu_DrawScrollArrow((char *)&arrowA + 1, ot, menu->x + 0x2a, menu->y + 2, 6, -10, menu->arrowTimer);
            }
            if (group < menu->groupCount - 1) {
                Menu_DrawScrollArrow((char *)&arrowA + 1, ot, menu->x + 0x2a, menu->y + 0x3e, 6, 10, menu->arrowTimer);
            }
            break;
        case 1:
            selected = menu->item;
            if (g_ChangeParts_ShowHelp != 0) {
                if (g_GameState.language == 0) {
                    Kanji_DrawFaded(ot, g_ChangeParts_HelpText, 0xb0, 0x1a4, 0x80, 0x278643c, 0);
                } else {
                    Text_SetFont(ctx, &g_OVL0_FontD);
                    ctx->rgb = 0x278500a;
                    Text_DrawCentered(ctx, (signed char *)g_ChangeParts_HelpText, 0xb0, 0x1bc, 0);
                }
            }
            break;
        case 2: {
            int limit = Settings_RaceSetup()->maxPower;

            if (limit > 0) {
                sprintf(line, sR_PowerRestrictedToNhp, limit * 1000 / 1014);
                Text_SetFont(ctx, &g_OVL0_FontB);
                ctx->rgb = Rgb_Lerp(D_8005B95C, D_8005B97C, level, 0x80);
                Text_DrawStringMonoDigitsRight(ctx, (signed char *)line, 0x140, 0xca, 0, -3, 0);
            }
            selected = menu->item;
            itemBlink = 8;
            level -= ((list->fade << 7) / list->fadeMax) * 100 / 0x80;
            break;
        }
        }
        ChangeParts_DrawItems(current, ot, selected, menu->x + 0x48, menu->y + 0x64, level, itemBlink, Settings_CarWork());
    }
}

// Sets the help's title; in Japanese, also caches its glyphs and the text's.
void PartsSetting_SetHelpTitle(char *sjis)
{
    g_PartsSetting_HelpTitle = sjis;
    if (g_PartsSetting_ShowHelp != 0 && g_GameState.language == 0) {
        void *font = g_OVL0_KanjiFont;

        KanjiFont_Reset((KanjiFont *)font);
        KanjiFont_CacheString(font, sjis);
        KanjiFont_CacheString(font, g_PartsSetting_HelpText);
    }
}

// Sets the help's text; in Japanese, also caches its glyphs and the title's.
void PartsSetting_SetHelpText(char *sjis)
{
    g_PartsSetting_HelpText = sjis;
    if (g_PartsSetting_ShowHelp != 0 && g_GameState.language == 0) {
        void *font = g_OVL0_KanjiFont;

        KanjiFont_Reset((KanjiFont *)font);
        KanjiFont_CacheString(font, g_PartsSetting_HelpTitle);
        KanjiFont_CacheString(font, sjis);
    }
}

// A value's bar, with its end labels and, optionally, its step count.
void Slider_Draw(uint *ot, SettingRange *range, int fade, int x, int y, int blink, int width, char *left, char *right, int showSteps)
{
    TextCtx text;
    char buf[0x10];
    GouraudQuad quad;
    unsigned int rgb;
    int span = range->max - range->min;
    TextCtx *ctx;
    unsigned char *base;

    Text_Init(&text, 6);
    ctx = &text;
    *(unsigned int *)&ctx->clut = (*(unsigned int *)&ctx->clut & 0xff9fffff) | 0x200000;
    ctx->ot = ot;
    Text_SetFont(ctx, &g_OVL0_FontA);
    base = D_8005C46C;
    ctx->rgb = Rgb_Lerp(base, D_8005C47C, fade, 0x100);
    if (left != 0) {
        Text_DrawCentered(ctx, (signed char *)left, x + 8, y + 4, 1);
    }
    if (right != 0) {
        Text_DrawCentered(ctx, (signed char *)right, x + width - 8, y + 4, 1);
    }
    if (showSteps != 0) {
        sprintf(buf, D_8005A9AC, range->value - range->min + 1);
        Text_SetFont(ctx, &g_OVL0_FontB);
        ctx->rgb = Rgb_Lerp(base, D_8005C488, fade, 0x80);
        Text_DrawStringMonoDigits(ctx, (signed char *)buf, x + width + 2, y + 0x13, 0, -3, 0);
    }
    if (span > 0) {
        int thumb = width * (range->value - range->min) / span + x;
        RECT *tile = Prim_AddTile(ot, Rgb_Lerp(base, D_8005C48C, fade, 0x80));

        tile->y = y + 5;
        tile->w = 4;
        tile->x = thumb - 2;
        tile->h = 0xc;
    }
    rgb = D_8005C494;
    if (blink >= 0) {
        rgb = D_8005C498;
    }
    quad.rect.x = x + 1;
    quad.rect.y = y + 9;
    quad.rect.w = width - 2;
    quad.rect.h = 5;
    quad.top = Rgb_Lerp(base, (unsigned char *)&rgb, fade, 0x100);
    quad.bottom = Rgb_Lerp(base, (unsigned char *)&rgb, fade, 0x80);
    if (blink >= 0) {
        int t = 8 - blink;

        if (t < 0) {
            t = 0;
        }
        rgb = quad.top;
        quad.top = Rgb_Lerp((unsigned char *)&rgb, D_8005C480, t, 8);
        rgb = quad.bottom;
        quad.bottom = Rgb_Lerp((unsigned char *)&rgb, D_8005C480, t, 8);
    }
    Menu_AddGouraudQuadOpaque(ot, &quad);
    {
        short *line = (short *)Prim_AddLineF4(ot, Rgb_Lerp(D_8005C46C, D_8005C490, fade, 0x80));
        int end;

        line[10] = line[8] = line[2] = x;
        end = x + width - 1;
        line[6] = line[4] = end;
        line[11] = line[5] = line[3] = y + 8;
        line[9] = line[7] = y + 0xe;
        Prim_AddDrawMode(ot, 0x220);
    }
}

// Formats a value by the setting's format: 1, 2 or 3 decimals, or x5 with 2.
void Setting_Format(char *buf, int value, int format)
{
    if (value < 0) {
        value = -value;
        *buf++ = '-';
    }
    switch (format & 0xf) {
    case 0:
        sprintf(buf, D_8005A9AC, value);
        break;
    case 1:
        sprintf(buf, D_8005A9B0, value / 10, value % 10);
        break;
    case 4:
        value *= 5;
    case 2:
        sprintf(buf, D_8005A9B8, value / 100, value % 100);
        break;
    case 3:
        sprintf(buf, D_8005A9C0, value / 1000, value % 1000);
        break;
    }
}

void SettingRow_Hide(SettingRow *row)
{
    row->state = -1;
}

void SettingRow_Init(SettingRow *row, char *label, char *unit, int format, SettingRange *range)
{
    row->blink = 0;
    row->state = 0;
    row->label = label;
    row->unit = unit;
    row->format = format;
    row->range = *range;
}

// One frame of a row: left and right step it, by 10 with L1 or R1 held.
void SettingRow_Update(SettingRow *row, PadState *pad)
{
    if (row->state >= 0) {
        if (++row->blink > 0x3c) {
            row->blink = 0;
        }
        if (pad != 0) {
            unsigned int held = pad->held;
            int step = 0;
            int value = row->range.value;
            unsigned int buttons;

            if (held & 0x10) {
                step = -1;
            }
            if (held & 0x1000) {
                step++;
            }
            if (held & 0xc) {
                step *= 10;
            }
            buttons = pad->pressed | pad->repeat;
            if (buttons & 4) {
                step--;
            }
            if (buttons & 8) {
                step++;
            }
            if (step < 0) {
                value = step + value;
                if (value < row->range.min) {
                    value = row->range.min;
                }
            } else if (step > 0) {
                value = step + value;
                if (value > row->range.max) {
                    value = row->range.max;
                }
            }
            if (row->range.value != value) {
                row->range.value = value;
                Sfx_PlayMenu(8);
            }
        }
    }
}

// A row: its label or end labels by format, and its bar.
void SettingRow_Draw(SettingRow *row, uint *ot, int x, int y, int selected, int fade)
{
    TextCtx text;
    char buf[0x20];
    TextCtx *ctx;
    Font *font = &g_OVL0_FontB;
    int blink;

    Text_Init(&text, 6);
    ctx = &text;
    ctx->ot = ot;
    Text_SetFont(ctx, font);
    *(unsigned int *)&ctx->clut = (*(unsigned int *)&ctx->clut & 0xff9fffff) | 0x200000;
    switch (row->format & 0xf) {
    case 5:
        ctx->rgb = Rgb_Lerp(D_8005C46C, D_8005C47C, fade, 0x80);
        Text_DrawCentered(ctx, (signed char *)sR_Sports_71ee, x + 8, y, 1);
        Text_DrawCentered(ctx, (signed char *)sR_Sports_71ee + 0x848, x + 0x78, y, 1);
        break;
    case 7:
        ctx->rgb = Rgb_Lerp(D_8005C46C, D_8005C47C, fade, 0x80);
        Text_DrawCentered(ctx, (signed char *)sR_Soft, x + 8, y, 1);
        Text_DrawCentered(ctx, (signed char *)sR_Soft + 0x18, x + 0x78, y, 1);
        break;
    case 6:
        Text_SetFont(ctx, &g_OVL0_FontA);
        ctx->rgb = Rgb_Lerp(D_8005C46C, D_8005C47C, fade, 0x80);
        Text_DrawCentered(ctx, (signed char *)sR_Soft, x + 4, y, 1);
        Text_DrawCentered(ctx, (signed char *)sR_Soft + 0x18, x + 0x7c, y, 1);
        Text_SetFont(ctx, font);
        ctx->rgb = Rgb_Lerp(D_8005C46C, D_8005C470, fade, 0x80);
        Text_DrawCentered(ctx, (signed char *)row->label, x + 0x40, y, 1);
        break;
    default:
        ctx->rgb = Rgb_Lerp(D_8005C46C, D_8005C470, fade, 0x80);
        Text_DrawStringMonoDigits(ctx, (signed char *)row->label, x, y, 1, -3, 0);
        Setting_Format(buf, row->range.value, (signed char)row->format);
        ctx->rgb = Rgb_Lerp(D_8005C46C, D_8005C478, fade, 0x80);
        Text_DrawStringMonoDigitsRight(ctx, (signed char *)buf, x + 0x50, y, 1, -3, 0);
        ctx->rgb = Rgb_Lerp(D_8005C46C, D_8005C47C, fade, 0x80);
        Text_DrawStringMonoDigits(ctx, (signed char *)row->unit, x + 0x54, y, 0, -2, 0);
        break;
    }
    blink = -1;
    if (selected != 0) {
        blink = row->blink;
    }
    Slider_Draw(ot, &row->range, fade, x + 8, y, blink, 0x70, 0, 0, 0);
}

// Formats the value in the car's setup of a setting, front or rear.
void Setting_GetCurrent(SettingItem *item, CarWork *work, char *buf, int front)
{
    int value = 0;

    switch (item->kind) {
    case 0:
        value = work->setup.t12_60[1];
        if (front != 0) {
            value = work->setup.t12_60[0];
        }
        break;
    case 1:
        value = work->setup.t12_5a[3];
        if (front != 0) {
            value = work->setup.t12_5a[2];
        }
        break;
    case 3:
        value = work->setup.t12_60[8];
        if (front != 0) {
            value = work->setup.t12_60[4];
        }
        break;
    case 2:
        value = work->setup.t12_60[10];
        if (front != 0) {
            value = work->setup.t12_60[6];
        }
        break;
    case 4:
        value = work->setup.t12_5a[1];
        if (front != 0) {
            value = work->setup.t12_5a[0];
        }
        break;
    case 5:
        value = work->setup.setting_5e[1] - 0x80;
        if (front != 0) {
            value = work->setup.setting_5e[0] - 0x80;
        }
        break;
    case 6:
        value = work->setup.t12_60[13];
        if (front != 0) {
            value = work->setup.t12_60[12];
        }
        break;
    case 7:
        value = work->setup.t01_50[1];
        if (front != 0) {
            value = work->setup.t01_50[0];
        }
        break;
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
        value = (unsigned short)work->setup.t11_values[item->kind - 8];
        break;
    case 17:
        value = work->setup.t05_53;
        if (front != 0) {
            value = work->setup.t05_52;
        }
        break;
    case 22:
        value = work->setup.t1b_74;
        break;
    case 23:
        value = work->setup.t1c_75;
        break;
    }
    Setting_Format(buf, value, item->format);
}

// A setting's row of the page: its value or its slider.
void SettingItem_Draw(SettingItem *item, uint *ot, int x, int y, int fade, int selected)
{
    TextCtx text;
    char buf[0x40];
    SettingRange ranges[4];
    SettingRange range[3];
    unsigned int rgb;
    CarWork *work = Settings_CarWork();
    int sliderX = x + 0x6a;
    int found = 0;
    int idA = 0;
    int idB = 1;
    int setting = 10;
    int setting2 = 0xe;
    TextCtx *ctx;
    int count;
    int i;

    Text_Init(&text, 6);
    ctx = &text;
    *(unsigned int *)&ctx->clut = (*(unsigned int *)&ctx->clut & 0xff9fffff) | 0x200000;
    ctx->ot = ot;
    Text_SetFont(ctx, &g_OVL0_FontB);
    rgb = D_8005C474;
    if (selected != 0) {
        rgb = *(unsigned int *)D_8005C470;
    }
    ctx->rgb = Rgb_Lerp(D_8005C46C, (unsigned char *)&rgb, fade, 0x80);
    Text_DrawStringMonoDigits(ctx, (signed char *)item->name, x, y, 0, -3, 0);
    switch (item->kind) {
    case 8:
        CarWork_GetSettingRange(work, 9, ranges);
        Slider_Draw(ot, ranges, fade, sliderX, y - 0x10, -1, 0x70, sR_Sports_71ee, sR_Sports_71ee + 0x848, 1);
        return;
    case 23:
        setting2 = 0xf;
    case 22:
        count = CarWork_GetSettingRange(work, setting2, ranges);
        goto slider;
    case 21:
        count = CarWork_GetSettingRange(work, 0xd, ranges);
    slider:
        if (count > 0) {
            Slider_Draw(ot, ranges, fade, sliderX, y - 0x10, -1, 0x70, sR_Soft, sR_Soft + 0x18, 1);
        }
        return;
    case 18:
        idA = 0;
        idB = 1;
        setting = 10;
        found = 1;
    case 19:
        if (found == 0) {
            idA = 2;
            idB = 3;
            setting = 0xb;
            found = 1;
        }
    case 20:
        if (found == 0) {
            idA = 4;
            idB = 5;
            setting = 0xc;
        }
        sliderX = 0;
        {
            SettingRange *r = range;
            int rowY = y - 0x10;

            count = CarWork_GetSettingRange(work, setting, r);
            for (i = 0; i < count; i++) {
                int show = 0;

                if (r->id == idA) {
                    show = 1;
                }
                if (r->id == idB) {
                    show = 1;
                }
                if (show != 0) {
                    Slider_Draw(ot, r, fade, sliderX, rowY, -1, 0x34, sR_Soft, sR_Hard, 1);
                }
                r++;
            }
        }
        return;
    case 0:
        if (CarWork_SuspHasSetting0(work) ^ 1) {
            return;
        }
        break;
    }
    Setting_GetCurrent(item, work, buf, 1);
    ctx->rgb = Rgb_Lerp(D_8005C46C, D_8005C478, fade, 0x80);
    Text_DrawStringMonoDigitsRight(ctx, (signed char *)buf, x + 0x78, y, 0, -3, 0);
    ctx->rgb = Rgb_Lerp(D_8005C46C, D_8005C47C, fade, 0x80);
    Text_DrawStringMonoDigits(ctx, (signed char *)item->unit, x + 0x7a, y, 0, -2, 0);
    if (item->format & 0x80) {
        Setting_GetCurrent(item, work, buf, 0);
        ctx->rgb = Rgb_Lerp(D_8005C46C, D_8005C478, fade, 0x80);
        Text_DrawStringMonoDigitsRight(ctx, (signed char *)buf, x + 0xb8, y, 0, -3, 0);
        ctx->rgb = Rgb_Lerp(D_8005C46C, D_8005C47C, fade, 0x80);
        Text_DrawStringMonoDigits(ctx, (signed char *)item->unit, x + 0xba, y, 0, -2, 0);
    }
}

int SettingsGroup_CountItems(SettingsGroup *group)
{
    int i;

    for (i = 0; i < 0x10; i++) {
        SettingItem *item = group->entries[i].item;

        if (item == 0) {
            return i;
        }
    }
    return 0x10;
}

// The settings of a group, with their cursors.
void PartsSetting_DrawItems(SettingsGroup *group, uint *ot, int selected, int x, int y, int fade, int blink, CarWork *work, signed char *counts)
{
    unsigned int rgb;
    int count = SettingsGroup_CountItems(group);
    int on = fade / 5;
    int off = fade / 6;
    int i = 0;
    int xy = (y << 16) - 0x180008;

    for (; i < count; i++) {
        int n = *counts++;
        SettingItem *item = group->entries[i].item;
        PartsSprite *sprite;
        PrimSprite *prim;

        if (n > 0) {
            RECT *tile = Prim_AddTile(ot, Rgb_Lerp(D_8005C46C, D_8005C484, fade, 0x80));

            tile->x = x - 8;
            tile->y = y - 0xc;
            tile->w = 4;
            tile->h = 0xc;
        }
        SettingItem_Draw(item, ot, x, y, fade, n > 0);
        sprite = &D_8005B8A8;
        if (i == selected) {
            int t = 8 - blink;

            if (t < 0) {
                t = 0;
            }
            rgb = Rgb_Lerp(D_8005C46C, group->rgb, fade, 0x180);
            rgb = Rgb_Lerp((unsigned char *)&rgb, D_8005C480, t, 8);
            prim = Prim_AddSprite(ot, rgb);
        } else {
            int grey = n > 0 ? on : off;

            prim = Prim_AddSprite(ot, grey | (grey << 8) | (grey << 16));
        }
        *(int *)prim = x + xy;
        ((int *)prim)[1] = D_8005B8A8.uvClut;
        ((int *)prim)[2] = *(int *)&sprite->w;
        Prim_AddDrawMode(ot, sprite->tpage | 0x220);
        x += 4;
        xy += 0x1e0000;
        y += 0x1e;
    }
}

// Lists the groups the car can set, and returns how many.
int PartsSetting_BuildGroups(void)
{
    CarWork *work = Settings_CarWork();
    int gears = CarWork_GetGearCount(work);
    int count;
    int n;
    SettingsGroup **slot;

    g_PartsSetting_Groups[0] = &D_8005C6E0;
    if (CarWork_HasFullSuspension(work) != 0) {
        g_PartsSetting_Groups[0] = &D_8005C650;
    }
    count = 1;
    if (CarWork_HasBrakeBalance(work) != 0) {
        g_PartsSetting_Groups[1] = &D_8005C770;
        count = 2;
    }
    slot = &g_PartsSetting_Groups[count];
    *slot = D_8005D100[gears];
    if (CarWork_HasFullTransmission(work) != 0) {
        *slot = D_8005D120[gears];
    }
    count++;
    g_PartsSetting_Groups[count] = &D_8005CFE0;
    count++;
    n = 0;
    if (CarWork_HasYawControl(work) != 0) {
        n = 1;
        D_8005D070.entries[0].item = (SettingItem *)&D_8005C620;
        D_8005D070.entries[0].id = 0xd;
    } else if (CarWork_HasLsdSettings(work) != 0) {
        n = 3;
        D_8005D070.entries[0].item = (SettingItem *)&D_8005C5F0;
        D_8005D070.entries[0].id = 0xa;
        D_8005D070.entries[1].item = (SettingItem *)&D_8005C600;
        D_8005D070.entries[1].id = 0xb;
        D_8005D070.entries[2].item = (SettingItem *)&D_8005C610;
        D_8005D070.entries[2].id = 0xc;
    }
    if (CarWork_HasAsc(work) != 0) {
        D_8005D070.entries[n].item = (SettingItem *)&D_8005C630;
        D_8005D070.entries[n].id = 0xe;
        n++;
    }
    if (CarWork_HasTcs(work) != 0) {
        D_8005D070.entries[n].item = (SettingItem *)&D_8005C640;
        D_8005D070.entries[n].id = 0xf;
        n++;
    }
    D_8005D070.entries[n].item = 0;
    D_8005D070.entries[n].id = -1;
    if (n > 0) {
        g_PartsSetting_Groups[count] = &D_8005D070;
        count++;
    }
    return count;
}

// The rows of the value list.
int PartsSetting_ValueCallback(MenuListEventId event, MenuListEvent *ev, int row)
{
    int result = 0;
    int x = ev->x;
    MenuList *list = ev->list;
    uint *ot = (uint *)ev->ot;
    SettingRow *setting = &((PartsSettingMenu *)list->userData)->rows[row];
    int y = ev->y;
    PadState *pad = 0;

    switch (event) {
    case 8:
        result = 1;
        break;
    case 0:
    case 7:
        break;
    case 3:
        if (row == list->selected) {
            pad = ev->pad;
        }
        SettingRow_Update(setting, pad);
        return result;
    case 6:
        if (list->selected != row) {
            setting->blink = 0;
        }
        return result;
    case 4: {
        int visible = list->state >= 0;
        int active = visible | (list->state < -1);

        if (active) {
            int fade = (list->fade << 7) / list->fadeMax;
            int level = fade;

            if (level > 0) {
                if (visible) {
                    x -= level - 0x80;
                }
                fade = fade * ev->brightness >> 7;
                SettingRow_Draw(setting, ot, x, y, row == list->selected, fade);
            }
        }
        break;
    }
    }
    return result;
}

void PartsSetting_Init(PartsSettingMenu *menu, int x, int y)
{
    int count;
    int i;

    g_PartsSetting_ValueCount = 0;
    g_PartsSetting_Group = 0;
    g_PartsSetting_Item = 0;
    g_PartsSetting_ShowHelp = 1;
    g_PartsSetting_HelpTitle = D_8005A9E0;
    g_PartsSetting_HelpText = D_8005A9E0;
    count = PartsSetting_BuildGroups();
    menu->x = x;
    menu->y = y;
    menu->state = 0;
    menu->open = -1;
    menu->arrowTimer = 0;
    menu->blink = 0;
    *(int *)menu->rgb = 0;
    PartsSetting_SelectGroup(menu, 0);
    menu->groupCount = count;
    for (i = 0; i < 0x10; i++) {
        SettingRow_Hide(&menu->rows[i]);
    }
    MenuList_Init(&g_PartsSetting_ValueList, PartsSetting_ValueCallback, menu);
    D_8005D1A0 = -1;
    menu->bar = D_8005D1A4;
    menu->bar.t = -1;
}

// Starts the open animation.
void PartsSetting_Open(void *menu)
{
    *(unsigned short *)((int)menu + 0xc) = 0;
    *(unsigned short *)((int)menu + 0x23c) = 0;
    *(unsigned short *)((int)menu + 10) = 0;
}

void PartsSetting_Close(PartsSettingMenu *menu)
{
    WipeBar *bar = &menu->bar;

    bar->t = ~bar->duration;
    menu->open = -1;
}

// Selects a group and reads how many values each of its settings has.
void PartsSetting_SelectGroup(PartsSettingMenu *menu, int group)
{
    CarWork *work = Settings_CarWork();
    int gearsDone = 0;
    int gears = 0;
    int i = 0;
    int gearId = 8;
    SettingsGroup *current = g_PartsSetting_Groups[group];

    *(int *)menu->prevRgb = *(int *)menu->rgb;
    {
        int rgb = *(int *)current->rgb;

        menu->group = group;
        *(int *)menu->rgb = rgb;
    }
    while (i < SettingsGroup_CountItems(current)) {
        short id2 = current->entries[i].id;
        int id = id2;

        if (id2 == gearId) {
            if (gearsDone == 0) {
                menu->counts[i] = CarWork_GetSettingRange(work, 8, menu->values);
                gears = menu->counts[i];
                gearsDone = 1;
            } else {
                menu->counts[i] = gears;
            }
        } else {
            menu->counts[i] = CarWork_GetSettingRange(work, id, menu->values);
        }
        i++;
    }
    PartsSetting_SetHelpTitle(current->help);
}

void PartsSetting_SelectItem(PartsSettingMenu *menu, int item)
{
    int group;

    menu->item = item;
    group = g_PartsSetting_Group;
    menu->blink = 0;
    PartsSetting_SetHelpText(g_PartsSetting_Groups[group]->entries[item].item->help);
}

int PartsSetting_Update(PartsSettingMenu *menu, PadState *pad, int showHelp)
{
    int result = -1;
    CarWork *work = Settings_CarWork();
    unsigned int buttons;
    int value;

    g_PartsSetting_ShowHelp = showHelp;
    WipeBar_Update(&menu->bar);
    if (menu->scroll < 0) {
        menu->scroll++;
    }
    if (menu->scroll > 0) {
        menu->scroll--;
    }
    if (++menu->blink > 0x3c) {
        menu->blink = 0;
    }
    if (++menu->arrowTimer > 0x2d) {
        menu->arrowTimer = 0;
    }
    WipeBar_Update(&g_PartsSetting_HelpBar);
    if (menu->open < 0) {
        MenuList_Update(&g_PartsSetting_ValueList, 0);
        return -1;
    }
    switch (menu->state) {
    case 0:
        MenuList_Update(&g_PartsSetting_ValueList, 0);
        if (pad == 0) {
            return result;
        }
        buttons = pad->pressed;
        if (buttons & 0x1000) {
            goto toParts;
        }
        if (buttons & 0x500) {
            result = -4;
            g_PartsSetting_ShowHelp = 0;
            break;
        }
        if (buttons & 0xa08) {
            result = -5;
            g_PartsSetting_Group = menu->group;
            menu->state = 1;
            PartsSetting_SelectItem(menu, 0);
            return result;
        }
        buttons |= pad->repeat;
        value = menu->group;
        if (buttons & 1) {
            int old = value;

            value--;
            if (value < 0) {
                value = 0;
            }
            if (value != old) {
                menu->scroll = -6;
            }
        }
        if (buttons & 2) {
            value++;
            if (value >= menu->groupCount) {
                value = menu->groupCount - 1;
            }
            if (value != menu->group) {
                menu->scroll = 6;
            }
        }
        if (value != menu->group) {
            PartsSetting_SelectGroup(menu, value);
            result = -2;
        }
        break;
    case 1:
        MenuList_Update(&g_PartsSetting_ValueList, 0);
        if (pad == 0) {
            return result;
        }
        buttons = pad->pressed;
        if (buttons & 0x1000) {
        toParts:
            result = -8;
            g_PartsSetting_ShowHelp = 0;
            break;
        }
        if (buttons & 0x504) {
            result = -6;
            menu->state = 0;
            break;
        }
        if (buttons & 0x10000) {
            CarWork_ResetSetting(work, g_PartsSetting_Groups[g_PartsSetting_Group]->entries[menu->item].id);
            return result;
        }
        if (buttons & 0xa00) {
            char *label = D_8005A9E0;
            SettingEntry *entry = &g_PartsSetting_Groups[g_PartsSetting_Group]->entries[menu->item];
            int id = entry->id;
            SettingItem *item = entry->item;
            int selected = 0;
            int count;
            int i;

            result = -7;
            count = CarWork_GetSettingRange(work, id, menu->values);
            if (count <= 0) {
                break;
            }
            for (i = 0; i < count; i++) {
                switch (id) {
                case 8:
                    label = D_8005C4A4[i];
                    if (i == count - 1) {
                        label = sR_Final;
                    }
                    selected = menu->item;
                    break;
                case 9:
                    label = D_8005A9E0;
                    break;
                case 10:
                case 11:
                case 12:
                    switch (menu->values[i].id) {
                    case 0:
                    case 2:
                    case 4:
                        label = sR_Front;
                        break;
                    case 1:
                    case 3:
                    case 5:
                        label = sR_Rear;
                        break;
                    }
                    break;
                default:
                    label = sR_Front;
                    if (i > 0) {
                        label = sR_Rear;
                    }
                    break;
                }
                SettingRow_Init(&menu->rows[i], label, item->unit, item->format & 0xf, &menu->values[i]);
            }
            g_PartsSetting_ValueCount = count;
            menu->state = 2;
            g_PartsSetting_ValueList.count = count;
            g_PartsSetting_ValueList.selected = selected;
            MenuList_Open(&g_PartsSetting_ValueList);
            D_8005D1A0 = 0;
            return -5;
        }
        buttons |= pad->repeat;
        value = menu->item;
        if (buttons & 1) {
            value--;
            if (value < 0) {
                value = 0;
            }
        }
        {
            int count = SettingsGroup_CountItems(g_PartsSetting_Groups[g_PartsSetting_Group]);

            if (buttons & 2) {
                value++;
                if (value >= count) {
                    value = count - 1;
                }
            }
        }
        if (value != menu->item) {
            PartsSetting_SelectItem(menu, value);
            result = -2;
        }
        break;
    case 2: {
        int chosen = MenuList_Update(&g_PartsSetting_ValueList, pad);

        if (chosen == -2) {
            break;
        }
        if (chosen == -3) {
            result = -2;
            break;
        }
        if (chosen == -1) {
            menu->blink = 0;
            result = -6;
            g_PartsSetting_HelpBar.t = ~g_PartsSetting_HelpBar.duration;
            MenuList_Close(&g_PartsSetting_ValueList);
            menu->state = 1;
            break;
        }
        {
            int id = g_PartsSetting_Groups[g_PartsSetting_Group]->entries[menu->item].id;
            int i;

            for (i = 0; i < g_PartsSetting_ValueCount; i++) {
                menu->values[i] = menu->rows[i].range;
            }
            CarWork_ApplySetting(work, id, menu->values, g_PartsSetting_ValueCount);
            result = -6;
            menu->blink = 0;
            g_PartsSetting_HelpBar.t = ~g_PartsSetting_HelpBar.duration;
            MenuList_Close(&g_PartsSetting_ValueList);
            menu->state = 1;
        }
        break;
    }
    default:
        return result;
    }
    return result;
}

void PartsSetting_Draw(void *self, unsigned int *ot)
{
    PartsSettingMenu *menu = (PartsSettingMenu *)self;
    TextCtx text;
    int arrow;
    int group = menu->group;
    SettingsGroup *current;
    int menuX;
    int scrollY = menu->y + menu->scroll * 10 - 0x1c;
    int prev = -1;
    int next = -1;
    int rows = 4;
    int i;
    int fadeIn;
    int fadeOut;
    int bright;
    int level;
    TextCtx *ctx;
    uint *ot2;
    MenuList *list = &g_PartsSetting_ValueList;
    WipeBar *bar = &g_PartsSetting_HelpBar;
    int selected;
    int itemBlink;

    current = g_PartsSetting_Groups[group];
    menuX = menu->x;
    Text_Init(&text, 6);
    ctx = &text;
    ctx->ot = ot;
    Text_SetFont(ctx, &g_OVL0_FontB);
    *(unsigned int *)&ctx->clut = (*(unsigned int *)&ctx->clut & 0xff9fffff) | 0x200000;
    i = group - 1;
    fadeOut = (menu->scroll << 7) / 6;
    if (fadeOut < 0) {
        fadeOut = -fadeOut;
    }
    fadeIn = 0x80 - fadeOut;
    level = WipeBar_GetLevel(&menu->bar);
    menu->bar.rgbTop = Rgb_Lerp(menu->prevRgb, menu->rgb, fadeIn, 0x80);
    if ((menu->bar.t >= 0) | (menu->bar.t < -1)) {
        bright = WipeBar_GetLevel(&menu->bar);
        fadeIn = bright * fadeIn >> 7;
        fadeOut = bright * fadeOut >> 7;
        if (menu->scroll < 0) {
            rows = 5;
            prev = group + 3;
        }
        if (menu->scroll > 0) {
            scrollY -= 0x3c;
            rows++;
            i = group - 2;
            next = group + 2;
        }
        rows += i;
        ot2 = ot + 1;
        for (; i < rows; i++) {
            if (i >= 0 && i < menu->groupCount) {
                PartsSprite *icon = &D_8005B830[g_PartsSetting_Groups[i]->icon];
                int shade = bright;
                PrimSprite *prim;

                if (i == next) {
                    shade = fadeIn;
                }
                if (i == prev) {
                    shade = fadeOut;
                }
                if (i != menu->group) {
                    shade >>= 2;
                }
                prim = Prim_AddSprite(ot2, shade | (shade << 8) | (shade << 16));
                *(int *)prim = menuX - (((short)icon->w >> 1) - 0x2a) + ((scrollY - ((short)icon->h >> 1)) << 16);
                ((int *)prim)[1] = icon->uvClut;
                ((int *)prim)[2] = *(int *)&icon->w;
                Prim_AddDrawMode(ot2, icon->tpage | 0x20);
            }
            scrollY += 0x3c;
        }
        if (menu->state >= 0 && menu->state < 2 && g_PartsSetting_ShowHelp != 0) {
            *(unsigned int *)&ctx->clut = (*(unsigned int *)&ctx->clut & 0xff9fffff) | 0x200000;
            Text_SetFont(ctx, &g_OVL0_FontB);
            ctx->rgb = 0x2503c28;
            Text_DrawStringRight(ctx, (signed char *)sR_R1CHANGEPARTS, 0x14c, 0x68, 0);
            Menu_DrawSideArrow(&arrow, ot, 0x14f, 0x5f, 5, 10, menu->arrowTimer);
        }
        {
            int top = menu->y;
            int shade = level * 0x60 / 0x80;
            int titleX;

            Text_SetFont(ctx, &g_OVL0_FontD);
            titleX = menuX + 0x44;
            ctx->rgb = shade | (shade << 8) | (shade << 16) | 0x2000000;
            *(unsigned int *)&ctx->clut = (*(unsigned int *)&ctx->clut & 0xff9fffff) | 0x200000;
            Text_DrawString(ctx, (signed char *)current->name, titleX, top + 0x1e, 1);
            if (g_GameState.language == 0 && g_PartsSetting_ShowHelp != 0) {
                Kanji_DrawFaded(ot + 1, current->help, titleX, top + 0x26, 0, D_8005C4A0, 0);
            }
            ot2 = ot + 1;
            {
                RECT *tile = Prim_AddTile(ot2, 0);

                tile->x = menuX + 0x1a;
                tile->y = top + 4;
                tile->w = 0x20;
                tile->h = 0x38;
                Prim_AddDrawMode(ot2, 0x200);
            }
            WipeBar_Draw(&menu->bar, ot2, menuX, top);
            Prim_AddDrawMode(ot2, 0x200);
            MenuList_Draw(list, ot);
            WipeBar_Draw(bar, ot, 0, 0xbe);
            Prim_AddDrawMode(ot, 0x220);
            if (bar->t >= 0) {
                Text_SetFont(ctx, &g_OVL0_FontD);
                ctx->rgb = Rgb_Lerp(D_8005C46C, D_8005C49C, WipeBar_GetLevel(bar), 0x80);
                Text_DrawCentered(ctx, (signed char *)current->entries[menu->item].item->name, 0xb0, 0xd8, 1);
            }
            selected = -1;
            itemBlink = menu->blink;
            switch (menu->state) {
            case 0:
                if (group > 0) {
                    Menu_DrawScrollArrow((char *)&arrow + 1, ot, menu->x + 0x2a, top + 2, 6, -10, menu->arrowTimer);
                }
                if (group < menu->groupCount - 1) {
                    Menu_DrawScrollArrow((char *)&arrow + 1, ot, menu->x + 0x2a, top + 0x3e, 6, 10, menu->arrowTimer);
                }
                break;
            case 1:
                selected = menu->item;
                goto help;
            case 2:
                selected = menu->item;
                level -= ((list->fade << 7) / list->fadeMax) * 100 / 0x80;
                itemBlink = 8;
            help:
                if (g_PartsSetting_ShowHelp != 0) {
                    if (g_GameState.language == 0) {
                        Kanji_DrawFaded(ot, g_PartsSetting_HelpText, 0xb0, 0x1a4, 0x80, 0x2808080, 0);
                    } else {
                        Text_SetFont(ctx, &g_OVL0_FontD);
                        ctx->rgb = 0x2808080;
                        Text_DrawCentered(ctx, (signed char *)g_PartsSetting_HelpText, 0xb0, 0x1bc, 0);
                    }
                }
                break;
            }
            PartsSetting_DrawItems(current, ot, selected, menu->x + 0x48, top + 0x64, level, itemBlink, Settings_CarWork(), menu->counts);
        }
    }
}

// Sets g_Settings_Work (Change Parts menu; the Parts Setting menu is at +0x2a7c) and
// returns work + 0x2cc4 (free space after it).
void *Settings_SetWork(void *work)
{
    g_Settings_Work = work;
    return (void *)((int)work + 0x2cc4);
}

// Rebuilds the car's parameters after a change, and stores its setup in the race and
// in the garage.
void Settings_Apply(void)
{
    struct Params {
        char data[0x1c0];
    };
    CarSpec spec;
    Params saved;
    unsigned short stats[4];
    CarWork *work = Settings_CarWork();
    char *base = (char *)&g_GameState;
    Params *params = (Params *)(base + 0x14fda);
    RaceSetup *setup = Settings_RaceSetup();
    RaceCar *raceCar = setup->cars;
    char *garage;
    int slot;

    saved = *params;
    CarSpec_FromCarWork(work, &spec);
    CarParams_Build(&spec, params);
    CarSetup_CopySoundFromSpec(&work->setup, &spec);
    *(CarSetup *)((char *)raceCar + 8) = work->setup;
    slot = setup->garageSlot;
    garage = (char *)&g_GameState + 0x7c9c;
    switch (setup->unk582) {
    case 0:
        garage = (char *)&g_GameState + 0x3c74;
    case 1: {
        char *car;
        unsigned short bits;

        CarWork_ComputeStats(work, stats);
        car = CAR_FROM_GARAGE(garage, slot);
        *(CarSetup *)(car + 8) = work->setup;
        *(unsigned short *)(car + 0x98) = (*(unsigned short *)(car + 0x98) & 0xc000) | stats[1];
        *(unsigned short *)(car + 0x96) = stats[2];
        bits = (*(unsigned short *)(car + 0x94) & 0xe000) | stats[0];
        *(unsigned short *)(car + 0x94) = bits;
        *(unsigned short *)(car + 0x94) = (bits & 0x1fff) | (*(unsigned char *)&stats[3] << 13);
        break;
    }
    }
    if (setup->raceType == 1) {
        *(CarWork *)(((int)&g_GameState + 0x10bd8) & -4) = *work;
    }
}

void ChangePartsPage_Enter(void)
{
    g_QuickMenu->activePage->timer = 0xc;
    ChangeParts_Init((ChangePartsMenu *)g_Settings_Work, 0, 0x64);
    {
        char *base = &g_Race_IsDemo;

        base[0x11d701] = 0;
    }
}

int ChangePartsPage_Update(int active)
{
    QuickMenu *quick = g_QuickMenu;
    int result = 0;
    PadState *pad = &quick->padState[0];
    QuickMenuPage *page = quick->activePage;

    if (active == 0) {
        pad = 0;
    }
    if (page->timer > 0) {
        if (--page->timer == 0) {
            ChangeParts_Open(g_Settings_Work);
        }
    }
    switch (ChangeParts_Update((ChangePartsMenu *)g_Settings_Work, pad, active)) {
    case -2:
        Sfx_PlayMenu(6);
        break;
    case -4:
        Settings_Apply();
        result = 2;
        ChangeParts_Close((ChangePartsMenu *)g_Settings_Work);
        Sfx_PlayMenu(4);
        break;
    case -8:
        QuickMenu_ReplacePage(g_QuickMenu, &g_Page_PartsSetting);
        Settings_Apply();
        result = 5;
        ChangeParts_Close((ChangePartsMenu *)g_Settings_Work);
        Sfx_PlayMenu(3);
        break;
    case -5:
        Sfx_PlayMenu(1);
        break;
    case -6:
        Sfx_PlayMenu(2);
        break;
    case -7:
        Sfx_PlayMenu(0);
        break;
    case -1:
        break;
    }
    return result;
}

// ChangeParts_Draw.
void ChangePartsPage_Draw(DrawLayer *layer)
{
    ChangeParts_Draw(g_Settings_Work, (unsigned int *)((int)layer->ot + 0x10));
}

void PartsSettingPage_Enter(void)
{
    g_QuickMenu->activePage->timer = 0xc;
    PartsSetting_Init((PartsSettingMenu *)((char *)g_Settings_Work + 0x2a7c), 0, 0x64);
}

int PartsSettingPage_Update(int active)
{
    QuickMenu *quick = g_QuickMenu;
    int result = 0;
    PadState *pad = &quick->padState[0];
    QuickMenuPage *page = quick->activePage;

    if (active == 0) {
        pad = 0;
    }
    if (page->timer > 0) {
        if (--page->timer == 0) {
            PartsSetting_Open((char *)g_Settings_Work + 0x2a7c);
        }
    }
    switch (PartsSetting_Update((PartsSettingMenu *)((char *)g_Settings_Work + 0x2a7c), pad, active)) {
    case -2:
        Sfx_PlayMenu(6);
        break;
    case -4:
        Settings_Apply();
        result = 2;
        PartsSetting_Close((PartsSettingMenu *)((char *)g_Settings_Work + 0x2a7c));
        Sfx_PlayMenu(4);
        break;
    case -8:
        QuickMenu_ReplacePage(g_QuickMenu, &g_Page_ChangeParts);
        Settings_Apply();
        result = 6;
        PartsSetting_Close((PartsSettingMenu *)((char *)g_Settings_Work + 0x2a7c));
        Sfx_PlayMenu(3);
        break;
    case -5:
        Sfx_PlayMenu(1);
        break;
    case -6:
        Sfx_PlayMenu(2);
        break;
    case -7:
        Sfx_PlayMenu(0);
        break;
    case -1:
        break;
    }
    return result;
}

// PartsSetting_Draw.
void PartsSettingPage_Draw(DrawLayer *layer)
{
    PartsSetting_Draw((void *)((int)g_Settings_Work + 0x2a7c), (unsigned int *)((int)layer->ot + 0x10));
}
}
