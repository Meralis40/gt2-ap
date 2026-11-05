// OVL0 8004f8b4-8005019c (licenserecords): License test best times and the records page.
// The functions that don't match yet are the original assembly.
#include "common.h"
#include "gamestate.h"

// The best times page's part of the license menu work area (g_OVL0_EndFontInfo).
struct LicenseRecordsWork {
    unsigned char unk0[0x4b8];
    short menuTest;                // 0x4b8: test selected in the license menu
    unsigned char unk4ba[0x66];
    WipeBar bar;                   // 0x520
    unsigned char rows[5][0x20];   // 0x53c: RecordRow per best time
    short rowCount;                // 0x5dc
    short test;                    // 0x5de: test whose times are shown
    unsigned char unk5e0[0xa4];
    short fade;                    // 0x684: rows slide-in counter, -1 = hidden
};

// A rounded box for Prim_AddRoundedBox: its colour and code, then its rectangle.
struct RoundedBox {
    unsigned int rgb;
    RECT rect;
};

extern "C" {
extern QuickMenu *g_QuickMenu;
extern void *g_OVL0_EndFontInfo;
extern unsigned char g_License_RuleRgbFrom;
extern unsigned char D_8005B1DC;
extern unsigned char D_8005B1E0;
extern unsigned char D_8005B1E4;
extern char D_8005A97C[];        // "%d"
extern WipeBar D_8005B238;       // the records page's wipe bar
extern Font g_OVL0_FontB;
extern Font g_OVL0_FontD;
extern char sR_NoRecords[];

#define WORK ((LicenseRecordsWork *)g_OVL0_EndFontInfo)

// One row of the best times page: rank, the record row (or "- No Records -") and the
// driver name, on a rounded box.
// Register allocation differs (x, y, the colour pointer, ot and fade are permuted among s2-s6).
void LicenseTest_DrawBestTimeRow(uint *ot, void *rows, char *name, int rank, int x, int y, int fade)
{
    TextCtx ctx;
    char buf[64];
    RoundedBox box;
    TextCtx *c;
    unsigned int rgb;
    RECT *tile;
    int w;

    Text_Init(&ctx, 6);
    c = &ctx;
    c->unk1c = 1;
    c->ot = ot;
    Text_SetFont(c, &g_OVL0_FontD);
    *(uint *)&c->clut = (*(uint *)&c->clut & 0xff9fffff) | 0x200000;
    c->rgb = Rgb_Lerp(&g_License_RuleRgbFrom, &D_8005B1E0, fade, 0x80);
    sprintf(buf, D_8005A97C, rank + 1);
    Text_DrawStringMonoDigits(c, (signed char *)buf, x, y + 0xc, 1, -2, 0);
    if (rows == 0) {
        c->rgb = Rgb_Lerp(&g_License_RuleRgbFrom, &D_8005B1DC, fade, 0x80);
        Text_SetFont(c, &g_OVL0_FontB);
        Text_DrawCentered(c, (signed char *)sR_NoRecords, x + 0x97, y + 0x10, 2);
    } else {
        RecordRow_Draw((char *)rows + rank * 0x20, ot, c, x, y, fade);
        rgb = Rgb_Lerp(&g_License_RuleRgbFrom, &D_8005B1DC, fade, 0x80);
        c->rgb = rgb;
        Text_SetFont(c, &g_OVL0_FontB);
        // Text_DrawStringRight returns the x where the name starts.
        w = ((int (*)(TextCtx *, signed char *, int, int, int))Text_DrawStringRight)(
            c, (signed char *)name, x + 0x12e, y + 0x16, 2);
        w = x + 0x128 - w;
        tile = Prim_AddTile(ot, rgb);
        tile->y = y + 10;
        tile->w = 4;
        tile->x = w;
        tile->h = 6;
        Prim_AddDrawMode(ot, 0x220);
    }
    box.rect.w = 0x160;
    box.rect.h = 0x30;
    box.rect.x = x + 0xb8;
    box.rect.y = y;
    box.rgb = Rgb_Lerp(&g_License_RuleRgbFrom, &D_8005B1E4, fade, 0x80);
    Prim_AddRoundedBox(ot, &box, 0x18);
    Prim_AddDrawMode(ot, 0);
}

// The 5 best time rows of the shown test, sliding in one after the other.
void LicenseTest_DrawBestTimes(void *ot, int x, int y, int fade)
{
    GameState *gs;
    RaceSetup *setup;
    char *names;
    void *rows;
    int count;
    int rank;
    int t;
    int px;
    int nameOfs;
    int offset;

    rank = 0;
    nameOfs = 0x68;
    gs = &g_GameState;
    offset = 0xbf7c;
    setup = (RaceSetup *)((char *)gs + offset);
    rows = WORK->rows;
    names = (char *)&gs->licenses[setup->licenseId][WORK->test];
    count = WORK->rowCount;
    while (1) {
        if (rank >= 5) {
            break;
        }
        t = fade - rank * 4;
        if (t >= 0) {
            if (t > 8) {
                t = 8;
            }
            t = (t << 7) / 8;
            px = x + (0x80 - t);
            if (rank >= count) {
                rows = 0;
            }
            LicenseTest_DrawBestTimeRow((uint *)ot, rows, names + nameOfs, rank, px, y, t);
        }
        x += 4;
        y += 0x36;
        nameOfs += 0xc;
        rank++;
    }
}

// Loads the test description and fills the record rows from the test's best times.
// The record pointer and the hoisted %hi(g_OVL0_EndFontInfo) swap s4 and s5.
void LicenseTest_LoadBestTimes(int license, int test)
{
    GameState *gs;
    char *rec;
    uint colors[6];
    int i;
    int j;
    short count;
    int rowOfs;
    int timeOfs;

    count = 0;
    gs = &g_GameState;
    rec = (char *)&gs->licenses[license][test];
    License_LoadDescription(license, test);
    colors[0] = 0x2004650;
    for (j = 1; j < 5; j++) {
        colors[j] = 0x2505050;
    }
    colors[j] = 0x2003060;
    i = 0;
    rowOfs = 0x53c;
    timeOfs = 4;
    while (1) {
        if (i >= 5) {
            break;
        }
        if (~*(int *)(rec + timeOfs) != 0) {
            count++;
            RecordRow_Init((char *)g_OVL0_EndFontInfo + rowOfs, -1, rec + timeOfs, colors);
        }
        rowOfs += 0x20;
        timeOfs += 0x14;
        i++;
    }
    WORK->rowCount = count;
}

// Best times page of the license menu, starting on the test selected in the menu.
void LicenseRecords_Enter(int back)
{
    g_QuickMenu->activePage->timer = 0xc;
    WORK->fade = -1;
    WORK->test = WORK->menuTest;
    WORK->bar = D_8005B238;
    WORK->bar.t = 0;
}

// Left/Right change the test (reloading its times); any face button goes back (2).
int LicenseRecords_Update(int active)
{
    QuickMenu *menu;
    QuickMenuPage *page;
    PadState *pad;
    GameState *gs;
    RaceSetup *setup;
    int offset;
    int license;
    unsigned int buttons;       // first 1 while input is accepted, then the buttons
    int ret;
    WipeBar *bar;

    ret = 0;
    buttons = 1;
    menu = g_QuickMenu;
    gs = &g_GameState;
    offset = 0xbf7c;
    setup = (RaceSetup *)((char *)gs + offset);
    page = menu->activePage;
    license = setup->licenseId;
    pad = &menu->padState[0];
    if (++page->sel > 0x2d) {
        page->sel = 0;
    }
    page->mode = active;
    if (page->timer > 0) {
        if (--page->timer == 0) {
            WORK->fade = 0;
            LicenseTest_LoadBestTimes(license, WORK->test);
        }
        buttons = 0;
    }
    if (WORK->fade >= 0) {
        if (++WORK->fade > 0x80) {
            WORK->fade = 0x80;
        }
    }
    WipeBar_Update(&WORK->bar);
    if (active & buttons) {
        buttons = pad->pressed | pad->repeat;
        if (buttons & PAD_LEFT) {
            Sfx_PlayMenu(7);
            WORK->fade = 0;
            if (--WORK->test < 0) {
                WORK->test = 9;
            }
            LicenseTest_LoadBestTimes(license, WORK->test);
        }
        if (buttons & PAD_RIGHT) {
            Sfx_PlayMenu(7);
            WORK->fade = 0;
            if (++WORK->test > 9) {
                WORK->test = 0;
            }
            LicenseTest_LoadBestTimes(license, WORK->test);
        }
        buttons = pad->pressed;
        if (buttons & (PAD_CIRCLE | PAD_SQUARE | PAD_CROSS | PAD_TRIANGLE)) {
            page->mode = 0;
            WORK->fade = -1;
            Sfx_PlayMenu(3);
            ret = 2;
            bar = &WORK->bar;
            bar->t = ~bar->duration;
        }
        return ret;
    }
    return ret;
}

// The best time rows, the test title and the test selector.
void LicenseRecords_Draw(DrawLayer *layer)
{
    QuickMenuPage *page;
    uint *ot;
    int fade;
    GameState *gs;
    RaceSetup *setup;
    int offset;

    page = g_QuickMenu->activePage;
    ot = (uint *)layer->ot;
    if (WORK->fade >= 0) {
        WipeBar_GetLevel(&WORK->bar);
        LicenseTest_DrawBestTimes(ot, 0x10, 0xb0, WORK->fade);
        if (page->mode != 0) {
            License_DrawTitleKanji(ot, 0x6c, 0x68, 0x80);
            License_DrawTitleText(ot, 0x5e, 0x80, 0x80);
        }
    }
    fade = WipeBar_GetLevel(&WORK->bar);
    gs = &g_GameState;
    offset = 0xbf7c;
    setup = (RaceSetup *)((char *)gs + offset);
    License_DrawTestNumber(ot, 0x14, 0x82, setup->licenseId, WORK->test, fade, page->sel);
    WipeBar_Draw(&WORK->bar, ot + 2, 0, 0x66);
    Prim_AddDrawMode(ot + 2, 0x220);
}
}
