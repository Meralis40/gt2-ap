// OVL0 8004c728-8004f8b4 (license): License tests: pages, medals and prize cars.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"
#include "gamestate.h"
#include "ovl2.h"
#include "ovl4.h"

// The menu loop, with what it keeps after the QuickMenu.
struct LoopMenu {
    QuickMenu menu;
    unsigned char raceDone;
};

// What the license menu shows of a test (License_LoadTestInfo).
struct LicenseTestInfo {
    unsigned short launchSpeed;    // in km/h
    unsigned int times[3];         // the medals' times
    char course[0x20];
    char code[0x40];
};

// A test's texts in the license description file: offsets from its start.
struct LicenseDesc {
    char unk0[8];
    unsigned short title;
    short lineCount;
    unsigned short lines[4];
};

// The result of the test, after g_RaceSetup.
struct LicenseResult {
    int unk0;
    int state;                     // 1 finished
    unsigned int time;
};

// A row of the license menu.
struct LicenseOption {
    char *label;
    signed char enabled;
    signed char action;
};

// A medal's sprite.
struct MedalSprite {
    unsigned int uvClut;
    short w;
    short h;
    unsigned short tpage;
};

// How InfoBar_Init sets up a bar.
struct InfoBarDesc;

// The license pages' work area (g_OVL0_EndFontInfo).
struct LicenseWork {
    signed char licensed;          // the license was just earned
    TextItem licensedText;         // 0x4
    signed char prizeCar;          // 0x2c: all gold, the prize car was just won
    TextItem prizeText;            // 0x30
    char acquiredDialog[0x94];     // 0x58: CardDialog
    char menuDialog[0x94];         // 0xec: CardDialog
    char nameEntry[0x78];          // 0x180: TextEntry
    TextItem items[8];             // 0x1f8
    WipeBar bars[8];               // 0x338
    InfoBar bar1;                  // 0x418
    InfoBar bar2;                  // 0x468
    short test;                    // 0x4b8: the test shown
    WipeBar titleBar;              // 0x4bc
    char *title;                   // 0x4d8
    char *lines[8];                // 0x4dc
    short lineCount;               // 0x4fc
    WipeBar descBar;               // 0x500
    short descFade;                // 0x51c
    char unk51e[0x688 - 0x51e];
    int showCar;                   // 0x688
    short carTimer;                // 0x68c
    ResultCar car;                 // 0x690
    char unk6a0[0x7d4 - 0x690 - sizeof(ResultCar)];
    ResultCarView view;            // 0x7d4
};

extern "C" {
extern signed char g_LicenseResult_Fresh;
extern QuickMenu *g_QuickMenu;
extern LicenseWork *g_OVL0_EndFontInfo;
extern unsigned char g_License_RuleRgbFrom;
extern unsigned char g_License_RuleRgbTo;
extern unsigned char D_8005B1BC, D_8005B1C0, D_8005B1C8, D_8005B1CC, D_8005B1D0, D_8005B1D4;
extern unsigned int D_8005B1E8, D_8005B1EC;
extern MedalSprite D_8005B150[];
extern char *g_LicensePrefixes[];
extern char *g_LicenseRaceCodeFormats[];
extern char *g_LicenseTestLabels[];
extern char *g_LicensePrizeCarCodes[];
extern char *g_LicenseAcquiredTexts[];
extern LicenseOption g_LicenseMenu_Options[];
extern MenuList g_LicenseMenu_List;
extern TextStyle D_8005B2F4, D_8005B310, D_8005B32C;
extern WipeBar D_8005B348, D_8005B238, D_8005B294;
extern CardDialogDesc D_8005B2C4, D_8005AC20;
extern InfoBarDesc D_8005B254, D_8005B274;
extern TextEntry D_8005B2B0;
extern QuickMenuPage g_Page_LicenseMenu, g_Page_LicenseName, g_Page_LicenseAcquired, g_Page_LicenseRetry;
extern QuickMenuPage g_Page_LicenseDemo, g_Page_LicenseRecords, g_Page_SaveReplay, g_Page_SaveGame;
extern void *g_License_Work;
extern void *g_EventDataBuf;
extern int g_GameDataMode;
extern char *g_GtModeRaceData;
extern unsigned char D_80091620[];
extern RaceEventParams g_RaceEventParams;
extern KanjiFont *g_OVL0_KanjiFont;
extern Font g_OVL0_FontB, g_OVL0_FontC;
extern char g_Race_IsDemo;
extern void *D_80081B5C;
extern unsigned char D_801D156E;
extern char sR_Nhp[], sR_LaunchSpeedAtNMph[], sR_NewCarAcquired[], sR_LicenseTest[];

unsigned int PackedTime_ToMs(void *time, int index);
void QuickMenu_PushPage(QuickMenu *menu, QuickMenuPage *page);
void QuickMenu_SetupCardUiThen(QuickMenuPage *page, void *arg);
void InfoBar_Init(InfoBar *bar, InfoBarDesc *desc);
void InfoBar_Draw(InfoBar *bar, uint *ot, int x, int y, TextCtx *ctx);
void ResultCarView_Init(ResultCarView *view, int w, int h);

#define WORK g_OVL0_EndFontInfo
#define ACQUIRED_DIALOG ((CardDialog *)WORK->acquiredDialog)
#define MENU_DIALOG ((CardDialog *)WORK->menuDialog)
#define NAME_ENTRY ((TextEntry *)WORK->nameEntry)

static inline void Rect_Set(RECT *rect, int x, int y, int w, int h)
{
    rect->x = x;
    rect->y = y;
    rect->w = w;
    rect->h = h;
}

static inline void *RaceMem_Aligned(int offset)
{
    return (void *)((int)(&g_Race_IsDemo + offset) & ~3);
}

// The car on the menu's stage.
static inline void License_InitCar(LicenseWork *work, void *model)
{
    ResultCar *car = &work->car;

    car->brightness = 0x40;
    car->enabled = 1;
    work->car.unk00 = 0x7fd7;
    car->tpage = 9;
    car->model = model;
    GroundDisc_Init(&car->disc, 0xa0000);
}

static inline RaceSetup *RaceSetup_Get2(char *base)
{
    int offset = 0xbf7c;

    return (RaceSetup *)(base + offset);
}

int License_IdFromCode(char *code)
{
    char prefix[16];
    int id = 0;
    int i;

    strcpy(prefix, code);
    prefix[3] = 0;
    for (i = id; i < 6; i++) {
        if (strcmp(g_LicensePrefixes[i], prefix) == 0)
            id = i;
    }
    return id;
}

RaceSetup *License_SetupTest(char *raceCode)
{
    void *data = g_EventDataBuf;
    GameState *gs = &g_GameState;
    char *base = (char *)gs;
    int offset = 0xbf7c;
    RaceSetup *setup = (RaceSetup *)(base + offset);
    RaceEvent *event = (RaceEvent *)GameData_FindEvent((GameData *)data, raceCode);
    GameDataTable *table;
    unsigned int colorIdx;
    unsigned int carCode;
    unsigned int *entry;
    RaceCar *car;
    int two;
    int one;

    memset(setup, 0, sizeof(RaceSetup));
    two = 2;
    one = 1;
    *(RaceOptions *)setup->options = *(RaceOptions *)(base + 1);
    setup->frameStep = two;
    setup->game = one;
    setup->raceType = 3;
    setup->showIntro = event->noIntro == 0;
    setup->unk0e = 0;
    RaceSetup_SetEventCode(setup, GameData_GetString((GameData *)data, event->codeString));
    RaceSetup_SetCourseByName(setup, GameData_GetString((GameData *)data, event->courseString));
    strcpy(setup->eventName, GameData_GetString((GameData *)data, event->nameString));
    setup->licenseId = License_IdFromCode(raceCode);
    {
        int ones = ((signed char *)raceCode)[4];

        setup->licenseTest = (((signed char *)raceCode)[3] - '0') * 10 - '0' + ones;
    }
    setup->lapCount = event->lapCount;
    setup->eventIndex = -1;
    setup->unk582 = -1;
    setup->garageSlot = -1;
    setup->carCount = one;
    setup->maxPower = 0;
    setup->flags = (setup->flags | one) & ~6;
    setup->worldLeague = 0;
    if (g_GameDataMode == two) {
        table = (GameDataTable *)(g_GtModeRaceData + 0x10);
    } else {
        table = (GameDataTable *)((char *)data + 0x100);
    }
    carCode = *(unsigned int *)((char *)event + 4);
    colorIdx = carCode >> 26;
    carCode &= 0x3ffffff;
    entry = (unsigned int *)GameDataTable_At(table, carCode - 1);
    car = &setup->cars[0];
    memset(car, 0, sizeof(RaceCar));
    car->carId = *entry;
    car->colorCode = D_80091620[colorIdx];
    car->active = 1;
    car->gridSlot = 0;
    CarSetup_FromCarEntry(GameData_GetCarEntry(carCode), &setup->cars[0].setup);
    car->driverType = 3;
    car->unk8f = 0;
    {
        int idx = car->setup.idx_t05;

        if (idx > 0) {
            car->carId = ((Table5Entry *)GameData_GetEntry(5, idx))->carId;
        }
    }
    g_RaceEventParams = *(RaceEventParams *)((char *)event + 0x44);
    {
        char *base2 = (char *)&g_GameState;
        int offset2 = 0x14fda;

        CarParams_BuildFromSetup(&setup->cars[0].setup, (CarParams *)(base2 + offset2));
    }
    return setup;
}

void License_LoadTestInfo(LicenseTestInfo *info, int license, int test)
{
    char buf[16];
    char *code = buf;
    void *data;
    RaceEvent *event;
    unsigned char *times;

    sprintf(code, g_LicenseRaceCodeFormats[license], test);
    data = g_EventDataBuf;
    event = (RaceEvent *)GameData_FindEvent((GameData *)data, code);
    times = &event->noIntro;
    info->launchSpeed = event->noIntro;
    info->times[0] = PackedTime_ToMs(times, 1);
    info->times[1] = PackedTime_ToMs(times, 2);
    info->times[2] = PackedTime_ToMs(times, 3);
    strcpy(info->code, GameData_GetString((GameData *)data, event->codeString));
    strcpy(info->course, GameData_GetString((GameData *)data, event->courseString));
}

void License_DrawTestNumber(uint *ot, int x, int y, int license, int test, int fade, int arrow)
{
    char buf[64];
    TextCtx ctx;
    char arrows[8];
    TextCtx *c = &ctx;
    int w;

    Text_Init(c, 6);
    c->unk1c = 1;
    *(uint *)&ctx.clut = (*(uint *)&ctx.clut & 0xff9fffff) | 0x200000;
    Text_SetFont(c, &g_OVL0_FontC);
    ctx.ot = ot;
    ctx.rgb = Rgb_Lerp(&g_License_RuleRgbFrom, &D_8005B1CC, fade, 0x80);
    sprintf(buf, g_LicenseTestLabels[license], test + 1);
    w = Text_DrawStringMonoDigits(c, (signed char *)buf, x, y, 2, 1, 4);
    if (arrow >= 0) {
        Menu_DrawSideArrow(arrows, ot, x + w + 5, y - 0x16, 6, 10, arrow);
        Menu_DrawSideArrow(arrows, ot, x - 4, y - 0x16, -6, 10, arrow);
    }
    Prim_AddDrawMode(ot, 0x220);
}

#ifdef NON_MATCHING
void License_LoadDescription(int license, int test)
{
    char *data = (char *)RaceMem_Aligned(0xca397);
    int testOffset = test * 20;
    int licenseOffset = license * 200;
    LicenseDesc *desc = (LicenseDesc *)(data + (testOffset + licenseOffset));
    LicenseWork *work = WORK;
    int count;
    KanjiFont *font;

    work->title = data + desc->title;
    count = desc->lineCount;
    work->lineCount = count;
    font = g_OVL0_KanjiFont;
    if (g_GameState.language == 0) {
        int i = 0;
        char *s;

        *(void **)((char *)font + 0x8000 + 0x1824) = &D_80081B5C;
        KanjiFont_Reset(font);
        s = WORK->title;
        for (;;) {
            KanjiFont_CacheString(font, s);
            if (i >= WORK->lineCount) {
                break;
            }
            s = data + *(unsigned short *)(data + (testOffset + i * 2 + licenseOffset) + 0xc);
            WORK->lines[i] = s;
            i++;
        }
        *(void **)((char *)font + 0x8000 + 0x1824) = (void *)KanjiFont_RenderGlyph;
    } else {
        int i = 0;

    loop:
        if (i < work->lineCount) {
            work->lines[i] = data + *(unsigned short *)(data + (testOffset + i * 2 + licenseOffset) + 0xc);
            i++;
            goto loop;
        }
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", License_LoadDescription);
#endif

#ifdef NON_MATCHING
void License_DrawTitleKanji(void *ot, int x, int y, int fade)
{
    KanjiFont *font = g_OVL0_KanjiFont;

    if (g_GameState.language == 0) {
        unsigned char *s = (unsigned char *)WORK->title;
        int bottom = y + 0x18;

        while (s[0] != 0) {
            KanjiGlyph *glyph = KanjiFont_FindGlyph(font, s[0] << 8 | s[1]);

            if (glyph != 0) {
                KanjiQuad *q = KanjiFont_AddGlyphQuad(glyph, (uint *)ot, Rgb_Lerp(&g_License_RuleRgbFrom, &D_8005B1C8, fade, 0x80));

                q->x0 = x;
                q->y0 = y;
                q->x1 = x + 0xf;
                q->y1 = y;
                q->x2 = x;
                q->y2 = bottom;
                q->x3 = x + 0xf;
                q->y3 = bottom;
                q->tpage = q->tpage & 0xff9f | 0x20;
            }
            x += 0xc;
            s += 2;
        }
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", License_DrawTitleKanji);
#endif

void License_DrawTitleText(uint *ot, int x, int y, int brightness)
{
    if (g_GameState.language != 0) {
        TextCtx ctx;
        TextCtx *c;

        Text_Init(&ctx, 6);
        c = &ctx;
        c->unk1c = 1;
        c->ot = ot;
        Text_SetFont(c, &g_OVL0_FontB);
        ((unsigned char *)&c->rgb)[0] = brightness;
        ((unsigned char *)&c->rgb)[1] = brightness;
        ((unsigned char *)&c->rgb)[2] = brightness;
        Text_DrawString(c, (signed char *)WORK->title, x, y, 1);
    }
}

// One kanji glyph quad, 15x15, semi-transparent.
void Kanji_AddGlyph15(void *ot, unsigned int rgb, int x, int y, void *glyph)
{
    KanjiQuad *pKVar1;

    pKVar1 = KanjiFont_AddGlyphQuad(glyph, ot, rgb);
    pKVar1->y0 = y;
    pKVar1->y1 = y;
    pKVar1->x0 = x;
    pKVar1->x1 = x + 0xf;
    pKVar1->x2 = x;
    pKVar1->y2 = y + 0xf;
    pKVar1->x3 = x + 0xf;
    pKVar1->y3 = y + 0xf;
    pKVar1->tpage = pKVar1->tpage & 0xff9f | 0x20;
}

#ifdef NON_MATCHING
void License_DrawDescKanji(void *ot, int x, int y, int fade)
{
    KanjiFont *font = g_OVL0_KanjiFont;

    if (g_GameState.language == 0) {
        int row = 0;

        while (row < WORK->lineCount) {
            unsigned char *s = (unsigned char *)WORK->lines[row];
            int bright = fade * 0xc00;
            int dim = fade >> 2;
            int px = x;

            while (s[0] != 0) {
                KanjiGlyph *glyph = KanjiFont_FindGlyph(font, s[0] << 8 | s[1]);

                if (glyph != 0) {
                    Kanji_AddGlyph15(ot, Rgb_Lerp(&g_License_RuleRgbFrom, &D_8005B1C8, bright >> 12, 0x80), px, y, glyph);
                    Kanji_AddGlyph15(ot, Rgb_Lerp(&g_License_RuleRgbFrom, &D_8005B1C8, dim, 0x80), px, y - 1, glyph);
                    Kanji_AddGlyph15(ot, Rgb_Lerp(&g_License_RuleRgbFrom, &D_8005B1C8, dim, 0x80), px, y + 1, glyph);
                }
                px += 0xc;
                s += 2;
            }
            {
                RECT *tile = Prim_AddTile(ot, Rgb_Lerp(&g_License_RuleRgbFrom, &g_License_RuleRgbTo, fade, 0x180));

                tile->y = y + 0x10;
                tile->w = 0x144;
                tile->h = 1;
                tile->x = x;
                Prim_AddDrawMode(ot, 0x20);
            }
            y += 0x16;
            row++;
        }
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", License_DrawDescKanji);
#endif

#ifdef NON_MATCHING
void License_DrawDescText(void *ot, int x, int y, int fade)
{
    if (g_GameState.language != 0) {
        TextCtx ctx;
        TextCtx *c;
        int i;

        Text_Init(&ctx, 6);
        c = &ctx;
        i = 0;
        c->unk1c = 1;
        c->ot = (uint *)ot;
        *(uint *)&c->clut = (*(uint *)&c->clut & 0xff9fffff) | 0x200000;
        Text_SetFont(c, &g_OVL0_FontB);
        ((unsigned char *)&c->rgb)[0] = fade;
        ((unsigned char *)&c->rgb)[1] = fade;
        ((unsigned char *)&c->rgb)[2] = fade;
        c->rgb = Rgb_Lerp(&g_License_RuleRgbFrom, &D_8005B1C8, fade, 0x80);
        while (i < WORK->lineCount) {
            RECT *tile;

            Text_DrawString(&ctx, (signed char *)WORK->lines[i], x, y, 1);
            i++;
            tile = Prim_AddTile(ot, Rgb_Lerp(&g_License_RuleRgbFrom, &g_License_RuleRgbTo, fade, 0x180));
            tile->x = x;
            tile->y = y;
            tile->w = 0x144;
            tile->h = 1;
            Prim_AddDrawMode(ot, 0x20);
            y += 0x16;
        }
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", License_DrawDescText);
#endif

int LicenseMenu_ItemCallback(MenuListEventId event, MenuListEvent *e, int i)
{
    TextCtx ctx;
    int result = 0;
    void *ot = e->ot;
    int x = e->x;
    int y = e->y;
    TextItem *item = &WORK->items[i];
    WipeBar *bar = &WORK->bars[i];

    Text_Init(&ctx, 6);
    ctx.unk1c = 1;
    switch (event) {
    case MLE_INIT:
        TextItem_Init(item, &D_8005B32C, g_LicenseMenu_Options[i].label);
        *bar = D_8005B348;
        bar->t = -1;
        break;
    case MLE_REVEAL:
        TextItem_Show(item, -1);
        bar->t = 0;
        break;
    case MLE_CLOSE:
        TextItem_Hide(item);
        bar->t = ~bar->duration;
        break;
    case MLE_UPDATE:
        TextItem_Update(item);
        WipeBar_Update(bar);
        break;
    case MLE_DRAW:
        if (i != 0) {
            int brightness = e->brightness;

            item->brightness = brightness;
            TextItem_Draw(item, (uint *)ot, x, y + 0x10, &ctx);
            WipeBar_Draw(bar, (uint *)ot + 1, x - 8, y - 4);
            Prim_AddDrawMode((uint *)ot + 1, 0x220);
        }
        break;
    case MLE_LEAVE:
        *(uint *)item->rgbFrom = 0x707070;
        item->flags &= ~8;
        break;
    case MLE_ENTER:
        *(uint *)item->rgbFrom = 0x907040;
        item->flags |= 8;
        TextItem_ShowNow(item);
        break;
    case MLE_OPEN:
        *(uint *)item->rgbFrom = 0x907040;
        item->flags |= 8;
        break;
    case MLE_IS_ENABLED:
        result = g_LicenseMenu_Options[i].enabled;
        break;
    }
    return result;
}

void License_DrawMedalTime(void *ot, unsigned int ms, unsigned int rgb, int x, int fade, TextCtx *ctx)
{
    char buf[64];
    GouraudQuad quad;
    int w;

    ctx->rgb = Rgb_Lerp(&g_License_RuleRgbFrom, &D_8005B1D4, fade, 0x80);
    Time_FormatMs(buf, ms);
    w = ((int (*)(TextCtx *, signed char *, int, int, int, int, int, int))Text_DrawDigitsRight)(
        ctx, (signed char *)buf, x, 0x124, 6, 5, 0, 0);
    quad.rect.x = x - w - 10;
    quad.rect.y = 0x116;
    quad.rect.w = 8;
    quad.rect.h = 0xe;
    quad.top = rgb;
    Menu_DrawColorSwatch((uint *)ot, &quad, fade);
}

#ifdef NON_MATCHING
void License_DrawTestInfo(void *ot, LicenseTestInfo *info, int x, int fade, int license, int test)
{
    TextCtx ctx;
    char buf[64];
    char *base = (char *)&g_GameState;
    int offset = 0x10bd8;
    LicenseTestCar *table = (LicenseTestCar *)((int)(base + offset) & ~3);
    TextCtx *c;
    LicenseTestCar *car;
    unsigned int rgb;
    char *strings;
    char *text;

    Text_Init(&ctx, 6);
    c = &ctx;
    c->unk1c = 1;
    *(uint *)&c->clut = (*(uint *)&c->clut & 0xff9fffff) | 0x200000;
    Text_SetFont(c, &g_OVL0_FontB);
    c->ot = (uint *)ot;
    rgb = Rgb_Lerp(&g_License_RuleRgbFrom, &D_8005B1D0, fade, 0x80);
    c->rgb = rgb;
    car = &table[license * 10 + test];
    Text_DrawString(c, (signed char *)car->name, x + 10, 0xc0, 1);
    // The drivetrains' names are addressed from "%dhp", after them.
    strings = sR_Nhp;
    sprintf(buf, strings, (short)car->maxPower * 1000 / 1015);
    Text_DrawStringMonoDigits(c, (signed char *)buf, x + 0x64, 0xd6, 1, -3, 0);
    text = strings - 0x17;
    switch ((short)car->drivetrain) {
    case 0:
        text = strings - 0x12;
        break;
    case 3:
        text = strings - 0xd;
        break;
    case 2:
        text = strings - 9;
        break;
    case 4:
        text = strings - 5;
        break;
    }
    Text_DrawString(&ctx, (signed char *)text, x + 0x10, 0xd6, 1);
    ctx.rgb = rgb;
    sprintf(buf, sR_LaunchSpeedAtNMph, info->launchSpeed * 10000 / 16093);
    Text_DrawString(&ctx, (signed char *)buf, x + 10, 0x10e, 1);
    License_DrawMedalTime(ot, info->times[0], 0x1ad8, x + 0x48, fade, c);
    License_DrawMedalTime(ot, info->times[1], 0x5a6e, x + 0x90, fade, c);
    License_DrawMedalTime(ot, info->times[2], 0x935, x + 0xd8, fade, c);
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", License_DrawTestInfo);
#endif

#ifdef NON_MATCHING
void License_DrawMedalRow(void *ot, int x, int y, int fade, int license, int selected)
{
    int i = 0;
    int gray = fade | fade << 8;
    LicenseTest *test = g_GameState.licenses[license];

loop:
    if (i < 10) {
        unsigned int color = D_8005B1EC;
        int medal = (signed char)test->medal - 1;
        PrimLineF4 *line;

        if (selected == i) {
            color = D_8005B1E8;
        }
        line = Prim_AddLineF4(ot, Rgb_Lerp(&g_License_RuleRgbFrom, (unsigned char *)&color, fade, 0x80));
        line->x2 = line->x1 = x + 0x14;
        line->x4 = line->x3 = line->x0 = x;
        line->y4 = line->y1 = line->y0 = y;
        line->y3 = line->y2 = y + 0x1c;
        if (medal >= 0) {
            PrimSprite *sprite = Prim_AddSprite(ot, gray | fade << 16);
            MedalSprite *m = &D_8005B150[medal];

            *(int *)sprite = (x - ((m->w >> 1) - 10)) + ((y - ((m->h >> 1) - 14)) << 16);
            ((unsigned int *)sprite)[1] = m->uvClut;
            ((unsigned int *)sprite)[2] = *(unsigned int *)&m->w;
            Prim_AddDrawMode(ot, m->tpage | 0x20);
        }
        x += 0x16;
        test++;
        i++;
        goto loop;
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", License_DrawMedalRow);
#endif

int License_IsEarned_OVL0(int license)
{
    GameState *gs = &g_GameState;
    int earned = 1;
    int i = 0;
    LicenseTest *test = gs->licenses[license];

loop:
    if (i < 10) {
        if ((signed char)test->medal == MEDAL_NONE)
            earned = 0;
        test++;
        i++;
        goto loop;
    }
    return earned;
}

int License_IsAllGold(int license)
{
    GameState *gs = &g_GameState;
    int gold = 1;
    int i = 0;
    LicenseTest *test = gs->licenses[license];

loop:
    if (i < 10) {
        if ((signed char)test->medal < MEDAL_GOLD)
            gold = 0;
        test++;
        i++;
        goto loop;
    }
    return gold;
}

#ifdef NON_MATCHING
void License_AwardMedal(void)
{
    char *base = (char *)&g_GameState;
    RaceSetup *setup = RaceSetup_Get2(base);
    LicenseTest *test = &((GameState *)base)->licenses[setup->licenseId][setup->licenseTest];
    LicenseResult *result = (LicenseResult *)(base + 0xc508);
    RaceEvent *event = (RaceEvent *)GameData_FindEvent((GameData *)g_EventDataBuf, setup->eventCode);
    unsigned char *times = &event->noIntro;
    unsigned int time = result->time;
    int medal = time < PackedTime_ToMs(times, 4);
    unsigned int ms = PackedTime_ToMs(times, 3);
    unsigned int best = time;

    if (time < ms) {
        medal = 2;
    }
    if (time < PackedTime_ToMs(times, 2)) {
        medal = 3;
    }
    if (best < PackedTime_ToMs(times, 1)) {
        medal = 4;
    }
    if (((signed char)test->medal == 0) & (medal == 1)) {
        int misses, allowed;

        if (test->nearMisses < 0xff) {
            test->nearMisses++;
        }
        misses = test->nearMisses;
        allowed = times[0x30];
        if (misses >= allowed) {
            test->medal = 1;
        } else {
            medal = 0;
        }
    }
    if ((signed char)test->medal < medal) {
        test->medal = medal;
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", License_AwardMedal);
#endif

int License_GivePrizeCar(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xbcec;

    return Garage_AddCar((Garage *)(base + 0x3c74), (Car *)(base + offset));
}

// Sets g_LicenseResult_Fresh: the test was just driven (medals are only awarded then).
void LicenseResult_SetFresh(unsigned char fresh)
{
    g_LicenseResult_Fresh = fresh;
}

#ifdef NON_MATCHING
void License_LoadPrizeCar(int license)
{
    RECT rect;
    unsigned short clut[256];
    CarAsset *asset = CarAsset_Find(CarCode_Encode((signed char *)g_LicensePrizeCarCodes[license]));
    CarModelHeader *model;
    ResultCar *car;

    Vol_ReadFileAsync(asset->file + 1, g_License_Work);
    Rect_Set(&rect, 0x200, 0x100, 0x40, 0xe0);
    Gpu_TransferImage(&rect, (unsigned int *)((char *)g_License_Work + 0x43a0), 0);
    Gpu_DrawSync();
    Clut_Blend(clut, (unsigned short *)((char *)g_License_Work + 0x20), 0, 0);
    Clut_UploadToSlot(clut, 0x18, 0);
    model = (CarModelHeader *)RaceMem_Aligned(0xcf397);
    Vol_ReadFileAsync(asset->file, model);
    CarModel_Init(model, 0x18);
    License_InitCar(WORK, model);
    ResultCarView_Init(&WORK->view, 0x160, 0x1e0);
    WORK->view.drawDisc = 0;
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", License_LoadPrizeCar);
#endif

void LicenseRoot_Enter(int back)
{
    QuickMenuPage *page = g_QuickMenu->activePage;
    LicenseWork *work = WORK;
    void *cardWork;

    work->prizeCar = 0;
    work->licensed = 0;
    cardWork = (char *)WORK + 0x8b0;
    page->sel = back;
    page->timer = 0x14;
    g_License_Work = cardWork;
    QuickMenu_SetupCardUi(cardWork);
    QuickMenu_SetupCardUiThen(0, cardWork);
}

#ifdef NON_MATCHING
int LicenseRoot_Update(int active)
{
    int result = 0;
    QuickMenuPage *page = g_QuickMenu->activePage;
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;
    RaceSetup *setup = (RaceSetup *)(base + offset);
    int license = setup->licenseId;
    int music = 0x14;

    if (page->timer > 0) {
        page->timer--;
        if (page->timer == 0) {
            int offset2 = 0xc508;

            if (page->sel == 1) {
                result = 4;
            } else {
            QuickMenuPage *next;

            if (((LicenseResult *)(base + offset2))->state != 1) {
                g_LicenseResult_Fresh = 0;
            }
            if (g_LicenseResult_Fresh != 0) {
                int wasEarned = License_IsEarned_OVL0(license);
                int wasGold = License_IsAllGold(license);
                int earned, gold;

                License_AwardMedal();
                earned = License_IsEarned_OVL0(license);
                gold = License_IsAllGold(license);
                if (earned != 0) {
                    WORK->licensed = wasEarned == 0;
                }
                if (gold != 0) {
                    WORK->prizeCar = wasGold == 0;
                    if (WORK->prizeCar != 0) {
                        LicenseWork *work = WORK;

                        work->prizeCar = License_GivePrizeCar();
                    }
                }
                if ((unsigned char)WORK->licensed | (unsigned char)WORK->prizeCar) {
                    License_LoadPrizeCar(license);
                }
                {
                    char *b = (char *)&g_GameState;
                    int recordOffset = 0xc5b0;

                    if (LicenseTest_FindRank(&((GameState *)b)->licenses[setup->licenseId][setup->licenseTest],
                                             (TimeRecord *)(b + recordOffset)) >= 0) {
                        next = &g_Page_LicenseName;
                        music = 8;
                    } else {
                        next = &g_Page_LicenseMenu;
                    }
                }
            } else {
                next = &g_Page_LicenseMenu;
            }
            QuickMenu_PushPage(g_QuickMenu, next);
            result = 1;
            QuickMenu_PlayMusic(music);
            }
        }
    }
    return result;
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", LicenseRoot_Update);
#endif

// Empty.
void LicenseRoot_Draw(DrawLayer *layer)
{
}

// Leave page of the license menu: 20-frame timer and music stop.
void LicenseRetry_Enter(int back)
{
    g_QuickMenu->activePage->timer = 0x14;
    QuickMenu_StopMusic();
}

#ifdef NON_MATCHING
int LicenseRetry_Update(int active)
{
    char buf[64];
    QuickMenu *menu = g_QuickMenu;
    QuickMenuPage *page = menu->activePage;
    int result = 0;

    page->timer--;
    if (page->timer < 0) {
        if ((signed char)menu->unk7c == 1) {
            char *base = (char *)&g_GameState;
            int offset = 0xbf7c;
            RaceSetup *setup = (RaceSetup *)(base + offset);
            int license = setup->licenseId;
            char *race = &g_Race_IsDemo;
            LicenseWork *work = WORK;

            if (setup->licenseTest != work->test) {
                int raceOffset = 0x118000;
                char *code = buf;

                race += raceOffset;
                race[0x5700] = 0;
                race[0x5701] = 0;
                sprintf(code, g_LicenseRaceCodeFormats[license], work->test);
                License_SetupTest(code);
                setup->cars[0].unk8f = base[0x7c8e];
            }
        }
        result = 4;
    }
    return result;
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", LicenseRetry_Update);
#endif

// Empty.
void LicenseRetry_Draw(DrawLayer *layer)
{
}

// VOL file of the demonstration replay of a test (License_DemoFileId).
int License_GetDemoFileId(int license, int test)
{
    int iVar1;

    iVar1 = License_DemoFileId(license, test);
    return iVar1;
}

// "Demonstration" from the license menu: 20-frame timer and music stop.
void LicenseDemo_Enter(int back)
{
    g_QuickMenu->activePage->timer = 0x14;
    QuickMenu_StopMusic();
}

#ifdef NON_MATCHING
int LicenseDemo_Update(int active)
{
    char buf[16];
    LoopMenu *loop = (LoopMenu *)g_QuickMenu;
    QuickMenu *menu = &loop->menu;
    int test = WORK->test;
    QuickMenuPage *page = loop->menu.activePage;
    int result = 0;
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;
    RaceSetup *setup;

    page->timer--;
    setup = (RaceSetup *)(base + offset);
    if (page->timer < 0) {
        int offset2 = 0x10bd8;
        void *table = (void *)((int)(base + offset2) & ~3);

        loop->raceDone = 0;
        memcpy(g_License_Work, table, 0xff0);
        if (setup->licenseTest != test) {
            int raceOffset = 0x118000;
            char *race = &g_Race_IsDemo;
            char *code = buf;

            race += raceOffset;
            race[0x5700] = 0;
            race[0x5701] = 0;
            sprintf(code, g_LicenseRaceCodeFormats[setup->licenseId], WORK->test);
            License_SetupTest(code);
        }
        Vol_ReadFileAsync(License_GetDemoFileId(setup->licenseId, test), setup);
        ((void (*)(RaceSetup *))Replay_RestoreCarParams)(setup);
        memcpy(table, g_License_Work, 0xff0);
        result = 4;
        menu->unk7c = 0;
    }
    return result;
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", LicenseDemo_Update);
#endif

// Empty.
void LicenseDemo_Draw(DrawLayer *layer)
{
}

void LicenseName_Enter(int back)
{
    TextEntry *entry;
    char *base;

    g_QuickMenu->activePage->timer = 0x18;
    TextEntry_Init(NAME_ENTRY, &D_8005B2B0);
    base = (char *)&g_GameState;
    entry = NAME_ENTRY;
    entry->text = base + 0x7c8f;
    entry->maxLen = 0xb;
    entry->maxWidth = 0x100;
}

#ifdef NON_MATCHING
int LicenseName_Update(int active)
{
    QuickMenu *menu = g_QuickMenu;
    QuickMenuPage *page = menu->activePage;
    PadState *pad = menu->padState;
    int result = 0;

    if (page->timer > 0) {
        page->timer--;
        if (page->timer == 0) {
            TextEntry *entry;

            TextEntry_Open(NAME_ENTRY);
            entry = NAME_ENTRY;
            entry->cursorPos = strlen(entry->text);
        }
    }
    switch (TextEntry_Update(NAME_ENTRY, pad)) {
    case -1:
        Sfx_PlayMenu(0);
        break;
    case 0:
    {
        char *base = (char *)&g_GameState;
        int offset = 0xbf7c;
        RaceSetup *setup = (RaceSetup *)(base + offset);
        QuickMenuPage *next;

        LicenseTest_InsertTime(&((GameState *)base)->licenses[setup->licenseId][setup->licenseTest],
                               (TimeRecord *)(base + 0xc5b0), base + 0x7c8f);
        TextEntry_Close(NAME_ENTRY);
        next = &g_Page_LicenseMenu;
        if ((unsigned char)WORK->prizeCar | (unsigned char)WORK->licensed) {
            next = &g_Page_LicenseAcquired;
        }
        QuickMenu_PushPage(g_QuickMenu, next);
        result = 1;
        Sfx_PlayMenu(3);
        break;
    }
    }
    return result;
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", LicenseName_Update);
#endif

// TextEntry_Draw.
void LicenseName_Draw(DrawLayer *layer)
{
    TextEntry_Draw(NAME_ENTRY, layer->ot);
}

void PrizeCarView_ZoomIn(ResultCarView *view, int t)
{
    int rot;

    view->pos[2] = 0xc0000;
    if (t < 0x5a) {
        view->pos[2] = (t << 19) / 90 + 0x40000;
    }
    view->rotZ = 0;
    if (t < 0x5a) {
        view->rotZ = -((0x5a - t) * 6);
    }
    rot = (unsigned short)view->rotY;
    view->pos[1] = 0;
    view->rotX = -0x20;
    rot += 0x84;
    view->rotY = (rot - t) & 0x3fff;
}

#ifdef NON_MATCHING
void LicenseAcquired_Enter(int back)
{
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;
    RaceSetup *setup = (RaceSetup *)(base + offset);
    QuickMenuPage *page = g_QuickMenu->activePage;
    int license = setup->licenseId;

    page->timer = 0;
    if (WORK->licensed == 0) {
        page->timer = 0x3c;
    }
    page->sel = 0;
    QuickMenu_SetupCardUiThen(&g_Page_LicenseMenu, g_License_Work);
    TextItem_Init(&WORK->licensedText, &D_8005B2F4, g_LicenseAcquiredTexts[license]);
    TextItem_Init(&WORK->prizeText, &D_8005B310, sR_NewCarAcquired);
    CardDialog_Init(ACQUIRED_DIALOG, &D_8005B2C4, 0);
    WORK->showCar = 1;
    WORK->carTimer = 0;
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", LicenseAcquired_Enter);
#endif

int LicenseAcquired_Update(int active)
{
    QuickMenu *menu = g_QuickMenu;
    QuickMenuPage *page = menu->activePage;
    PadState *pad;
    int result = 0;
    QuickMenuPage *next;
    int frame;
    int inactive;

    TextItem_Update(&WORK->licensedText);
    TextItem_Update(&WORK->prizeText);
    pad = menu->padState;
    if (WORK->showCar != 0 && WORK->carTimer >= 0) {
        short t = ++WORK->carTimer;

        if (t >= 0x19) {
            if (t >= 0x91) {
                WORK->carTimer = 0x90;
            }
            PrizeCarView_ZoomIn(&WORK->view, WORK->carTimer - 0x18);
        }
    }
    frame = page->timer;
    if (page->sel == 0) {
        frame++;
    }
    switch (frame) {
    case 0x3c:
        if (WORK->licensed != 0) {
            Sfx_PlayMenu(3);
            TextItem_Show(&WORK->licensedText, -1);
        }
        break;
    case 0x78:
        if (WORK->prizeCar != 0) {
            Sfx_PlayMenu(3);
            TextItem_Show(&WORK->prizeText, -1);
        }
        break;
    case 0xb4:
        page->sel = 1;
        CardDialog_Open(ACQUIRED_DIALOG);
        frame = 0xb5;
        ACQUIRED_DIALOG->choice = 1;
        break;
    }
    page->timer = frame;
    next = &g_Page_LicenseMenu;
    inactive = frame < 0xb4;
    inactive |= active ^ 1;
    if (inactive) {
        pad = 0;
    }
    switch (CardDialog_Update(ACQUIRED_DIALOG, pad)) {
    case -1:
        Sfx_PlayMenu(0);
        break;
    case 0:
        next = &g_Page_SaveGame;
        // fall through
    case 1:
        Sfx_PlayMenu(3);
        if (WORK->licensed != 0) {
            TextItem_Hide(&WORK->licensedText);
        }
        if (WORK->prizeCar != 0) {
            TextItem_Hide(&WORK->prizeText);
        }
        result = 1;
        CardDialog_Close(ACQUIRED_DIALOG);
        WORK->carTimer = -1;
        QuickMenu_PushPage(menu, next);
        break;
    }
    return result;
}

// Dialog, both texts and the prize car (body only).
void LicenseAcquired_Draw(DrawLayer *layer)
{
    QuickMenu *pQVar1;
    TextCtx *ctx;
    unsigned int *ot;

    pQVar1 = g_QuickMenu;
    ot = layer->ot;
    ctx = &g_QuickMenu->text;
    CardDialog_Draw((CardDialog *)((int)g_OVL0_EndFontInfo + 0x58), ot, ctx);
    TextItem_Draw((TextItem *)((int)g_OVL0_EndFontInfo + 4), ot, 0xb0, 200, ctx);
    TextItem_Draw((TextItem *)((int)g_OVL0_EndFontInfo + 0x30), ot, 0xb0, 300, ctx);
    if ((*(int *)((int)g_OVL0_EndFontInfo + 0x688) != 0) &&
          (0x17 < *(short *)((int)g_OVL0_EndFontInfo + 0x68c))) {
        ResultCar_Draw((ResultCar *)((int)g_OVL0_EndFontInfo + 0x690), pQVar1->layers + 4, pQVar1->layers + 5,
                       (ResultCarView *)((int)g_OVL0_EndFontInfo + 0x7d4), 1);
    }
}

#ifdef NON_MATCHING
void LicenseMenu_Enter(int back)
{
    QuickMenu *menu = g_QuickMenu;
    QuickMenuPage *page = menu->activePage;
    LicenseOption *options;

    page->timer = 0xc;
    page->arg = 0;
    options = g_LicenseMenu_Options;
    options[1].enabled = 0;
    options[4].enabled = 0;
    if (((LoopMenu *)menu)->raceDone != 0) {
        options[1].enabled = 1;
        options[4].enabled = 1;
    }
    if (back == 0) {
        MenuList_Init(&g_LicenseMenu_List, LicenseMenu_ItemCallback, 0);
        g_LicenseMenu_List.selected = 2;
    }
    InfoBar_Init(&WORK->bar1, &D_8005B254);
    InfoBar_Init(&WORK->bar2, &D_8005B274);
    {
        char *base = (char *)&g_GameState;
        int offset = 0xbf7c;
        RaceSetup *setup = (RaceSetup *)(base + offset);
        LicenseWork *work = WORK;
        int test = setup->licenseTest;

        work->test = test;
        License_LoadDescription(setup->licenseId, test);
    }
    WORK->descFade = 8;
    CardDialog_Init(MENU_DIALOG, &D_8005AC20, 0);
    {
        LicenseWork *work = WORK;

        MENU_DIALOG->x = 0xb0;
        MENU_DIALOG->y = 0x19a;
        work->titleBar = D_8005B238;
        WORK->titleBar.t = 0;
        work->descBar = D_8005B294;
        WORK->descBar.t = 0;
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", LicenseMenu_Enter);
#endif

#ifdef NON_MATCHING
int LicenseMenu_Update(int active)
{
    int leaving = 0;
    int result = leaving;
    QuickMenu *menu = g_QuickMenu;
    QuickMenuPage *page = menu->activePage;
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;
    int license = ((RaceSetup *)(base + offset))->licenseId;
    PadState *pad = menu->padState;
    int choice;

    page->sel++;
    if (page->sel >= 0x2e) {
        page->sel = 0;
    }
    page->mode = active;
    if (page->timer > 0) {
        page->timer--;
        if (page->timer == 0) {
            InfoBar *bar;

            MenuList_Open(&g_LicenseMenu_List);
            g_LicenseMenu_List.revealDelay = -1;
            bar = &WORK->bar1;
            bar->bar.t = 0;
            TextItem_Show(&bar->label, -1);
            bar->anim = 0;
            bar = &WORK->bar2;
            bar->bar.t = 0;
            TextItem_Show(&bar->label, -1);
            bar->anim = 0;
        }
    }
    if (WORK->descFade > 0) {
        WORK->descFade--;
    }
    WipeBar_Update(&WORK->titleBar);
    WipeBar_Update(&WORK->descBar);
    InfoBar_Update(&WORK->bar1);
    InfoBar_Update(&WORK->bar2);
    if (active == 0) {
        pad = 0;
    }
    if (page->arg == 1) {
        MenuList_Update(&g_LicenseMenu_List, 0);
        choice = CardDialog_Update(MENU_DIALOG, pad);
        switch (choice) {
        case -3:
        case -2:
            break;
        case -1:
            Sfx_PlayMenu(2);
            page->arg = 0;
            break;
        default:
            ((RaceSetup *)(base + 0xbf7c))->cars[0].unk8f = choice;
            base[0x7c8e] = choice;
            Sfx_PlayMenu(3);
            leaving = 1;
            page->mode = 0;
            menu->unk7c = leaving;
            result = 1;
            QuickMenu_PushPage(menu, &g_Page_LicenseRetry);
            break;
        }
    } else {
        CardDialog_Update(MENU_DIALOG, 0);
        choice = MenuList_Update(&g_LicenseMenu_List, pad);
        switch (choice) {
        case -3:
            Sfx_PlayMenu(6);
            break;
        case -2:
            if (g_LicenseMenu_List.selected == 0 && pad != 0) {
                unsigned int buttons = pad->pressed | pad->repeat;

                if (buttons & 4) {
                    Sfx_PlayMenu(5);
                    WORK->test--;
                    if (WORK->test < 0) {
                        WORK->test = 9;
                    }
                    WORK->descFade = 8;
                    License_LoadDescription(license, WORK->test);
                }
                if (buttons & 8) {
                    Sfx_PlayMenu(5);
                    WORK->test++;
                    if (WORK->test >= 10) {
                        WORK->test = 0;
                    }
                    WORK->descFade = 8;
                    License_LoadDescription(license, WORK->test);
                }
            }
            break;
        case -4:
        case -1:
            Sfx_PlayMenu(0);
            break;
        default:
            if (choice == 0) {
                Sfx_PlayMenu(1);
                MenuList_SetSelection(&g_LicenseMenu_List, 2);
            } else if (choice == 2) {
                Sfx_PlayMenu(1);
                MENU_DIALOG->choice = D_801D156E != 0;
                CardDialog_Open(MENU_DIALOG);
                page->arg = 1;
            } else {
                int action;

                Sfx_PlayMenu(3);
                action = g_LicenseMenu_Options[choice].action;
                leaving = 1;
                switch (action) {
                case -4:
                    page->mode = 0;
                    result = 1;
                    QuickMenu_PushPage(menu, &g_Page_LicenseRecords);
                    break;
                case -2:
                    page->mode = 0;
                    QuickMenu_PushPage(menu, &g_Page_SaveReplay);
                    result = 1;
                    QuickMenu_SetupCardUi(g_License_Work);
                    break;
                case -5:
                    page->mode = 0;
                    result = 1;
                    QuickMenu_PushPage(menu, &g_Page_LicenseDemo);
                    break;
                default:
                    page->mode = 0;
                    menu->unk7c = g_LicenseMenu_Options[choice].action;
                    result = 1;
                    QuickMenu_PushPage(menu, &g_Page_LicenseRetry);
                    break;
                }
            }
            break;
        }
    }
    if (leaving) {
        WipeBar *bar;
        InfoBar *info;

        MenuList_Close(&g_LicenseMenu_List);
        bar = &WORK->titleBar;
        bar->t = ~bar->duration;
        bar = &WORK->descBar;
        bar->t = ~bar->duration;
        info = &WORK->bar1;
        info->bar.t = ~info->bar.duration;
        TextItem_Hide(&info->label);
        info->anim = -0x11;
        info = &WORK->bar2;
        info->bar.t = ~info->bar.duration;
        TextItem_Hide(&info->label);
        info->anim = -0x11;
    }
    return result;
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", LicenseMenu_Update);
#endif

// Horizontal separator line at y.
void License_DrawRule(void *ot, short y, int fade)
{
    unsigned int rgb;
    RECT *pRVar1;

    rgb = Rgb_Lerp(&g_License_RuleRgbFrom, &g_License_RuleRgbTo, fade, 0x80);
    pRVar1 = Prim_AddTile(ot, rgb);
    pRVar1->x = 0x7e;
    pRVar1->w = 0x160;
    pRVar1->y = y;
    pRVar1->h = 1;
}

#ifdef NON_MATCHING
void LicenseMenu_Draw(DrawLayer *layer)
{
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;
    RaceSetup *setup = (RaceSetup *)(base + offset);
    uint *ot = layer->ot;
    int license = setup->licenseId;
    LicenseTestInfo info;
    QuickMenuPage *page = g_QuickMenu->activePage;
    TextCtx *text = &g_QuickMenu->text;
    int dim = page->arg == 1;
    int fade;
    int arrow;
    RECT *tile;

    CardDialog_Draw(MENU_DIALOG, ot, text);
    fade = WipeBar_GetLevel(&WORK->titleBar);
    Text_Init(text, 6);
    text->unk1c = 1;
    text->ot = ot;
    *(uint *)&text->clut = (*(uint *)&text->clut & 0xff9fffff) | 0x200000;
    Text_SetFont(text, &g_OVL0_FontB);
    text->rgb = Rgb_Lerp(&g_License_RuleRgbFrom, &D_8005B1C8, fade, 0x80);
    Text_DrawStringRight(text, (signed char *)sR_LicenseTest, 0x154, 0x38, 1);
    tile = Prim_AddTile(ot, Rgb_Lerp(&D_8005B1BC, &D_8005B1C0, fade, 0x80));
    tile->y = 0x38;
    tile->w = 0x160;
    tile->x = 0;
    tile->h = 2;
    if (page->mode != 0) {
        License_DrawTitleKanji(ot, 0x6c, 0x52, fade);
        License_DrawTitleText(ot, 0x5e, 0x68, 0x80);
    }
    WipeBar_Draw(&WORK->titleBar, ot + 2, 0, 0x50);
    Prim_AddDrawMode(ot + 2, 0x220);
    MenuList_Draw(&g_LicenseMenu_List, ot);
    InfoBar_Draw(&WORK->bar1, ot, 0x78, 0x9e, text);
    InfoBar_Draw(&WORK->bar2, ot, 0x78, 0xec, text);
    fade = 0x80 - WORK->descFade * 0x80 / 8;
    if (fade < 0) {
        fade = 0;
    }
    fade = fade * WipeBar_GetLevel(&WORK->titleBar) >> 7;
    License_LoadTestInfo(&info, license, WORK->test);
    License_DrawTestInfo(ot, &info, 0x78, fade, license, WORK->test);
    fade = WipeBar_GetLevel(&WORK->titleBar);
    License_DrawRule(ot, 0xc0, fade);
    License_DrawRule(ot, 0xd6, fade);
    License_DrawRule(ot, 0x10e, fade);
    License_DrawRule(ot, 0x124, fade);
    License_DrawMedalRow(ot, 0x78, 0x72, fade, license, WORK->test);
    if (g_LicenseMenu_List.state != -1) {
        arrow = -1;
        if (g_LicenseMenu_List.selected == 0) {
            arrow = page->sel;
        }
        License_DrawTestNumber(ot, 0x14, 0x6c, setup->licenseId, WORK->test,
                               (g_LicenseMenu_List.fade << 7) / g_LicenseMenu_List.fadeMax, arrow);
    }
    WORK->descBar.rgbTop = 0x5a4a3e;
    WORK->descBar.rgbBottom = 0x3e362a;
    if (dim) {
        WORK->descBar.rgbTop = 0x26201a;
        WORK->descBar.rgbBottom = 0x1a150f;
        fade /= 3;
    }
    ot++;
    if (page->mode != 0) {
        License_DrawDescKanji(ot, 0xf, 0x136, fade);
        License_DrawDescText(ot, 0xf, 0x146, fade);
    }
    WipeBar_Draw(&WORK->descBar, ot, 0, 0x12c);
    Prim_AddDrawMode(ot, 0x220);
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/license", LicenseMenu_Draw);
#endif
}
