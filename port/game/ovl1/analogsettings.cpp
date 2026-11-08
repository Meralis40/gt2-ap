// OVL1 8001aad4-8001d4b0 (analogsettings): Analog controller settings and calibration.
// The functions that don't match yet are the original assembly, with their C++ under
// NON_MATCHING.
#include "gptr.h"
#include "common.h"
#include "pad.h"

// A calibration step: the slot (0 steering, 1 accelerator, 2 brake, 3 Exit), the value
// and its prompt.
struct AnalogStep {
    short kind;
    short sub;
    game_pointer<char> prompt;
};

extern "C" {
extern GameState (&g_GameState);
extern game_pointer<char> (&g_AnalogSettings_Prompt);
extern game_pointer<BaseMenu> (&g_CurrentMenu);
extern game_pointer<KanjiFont> (&g_OVL1_KanjiFont);
extern Font (&g_OVL1_FontLarge);
extern Font (&g_OVL1_FontMedium);
extern AnalogStep (&g_AnalogSettings_Steps)[];
extern int (&g_AnalogSettings_StepCount);
extern int (&g_AnalogSettings_SteerAnalog);
extern int (&g_AnalogSettings_AccelAnalog);
extern int (&g_AnalogSettings_BrakeAnalog);
extern short (&g_AnalogSettings_RawSteer);
extern short (&g_AnalogSettings_RawAccel);
extern short (&g_AnalogSettings_RawBrake);
extern unsigned char (&D_8004C31C)[];
extern unsigned char (&D_8004C34C)[];
extern int (&g_AnalogSettings_LockRight);
extern int (&g_AnalogSettings_LockLeft);
extern char (&s_TurnWheelToFarLeftAndPressR)[];
extern char (&s_PressROrStartToSetPosition)[];
extern char (&s_Steering)[];
extern char (&s_Acceleration)[];
extern char (&s_Brake)[];
extern char (&s_Exit)[];
void Options_KanjiDraw(uint *ot, char *sjis, int x, int y, int center, uint rgb);
SpriteDesc *KeyConfig_GetNeGconIcon(int code);
void SteerCalib_Save(NeGconCalib *calib, SteerCalib *c, int axis);
void PedalCalib_Save(NeGconCalib *calib, PedalCalib *c, int axis);
extern unsigned char (&D_8004C320)[];
extern unsigned char (&D_8004C324)[];
extern unsigned char (&D_8004C328)[];
extern unsigned char (&D_8004C32C)[];
extern unsigned char (&D_8004C330)[];
extern unsigned char (&D_8004C334)[];
extern unsigned char (&D_8004C338)[];
extern unsigned char (&D_8004C33C)[];
extern unsigned char (&D_8004C340)[];
extern unsigned char (&D_8004C344)[];
extern unsigned char (&D_8004C348)[];
extern WipeBar (&D_8004C350);
extern WipeBar (&D_8004C36C);
extern WipeBar (&D_8004C388);
extern char (&D_80022D7C)[]; // "%d"
extern char (&s_NA)[];
extern char (&s_Center)[];
extern char (&s_Max)[];
extern char (&s_Exit_2)[];
extern char (&s_SetTheCenterOfSteering)[];
extern char (&s_SetTheLockPositionOfAcceleration)[];
extern char (&s_SetTheLockPositionOfBraking)[];
extern char (&s_TurnWheelToFarRightAndPressR)[];
int sprintf(char *buf, const char *format, ...);
void SteerCalib_SetLimits(SteerCalib *c, int upper, int lower);
void SteerCalib_Load(NeGconCalib *calib, SteerCalib *c, int axis);
void PedalCalib_Load(NeGconCalib *calib, PedalCalib *c, int axis);
void AnalogSettings_Init(AnalogSettings *s, int port);
int Analog_GetSteerAxis(int port);
int Analog_GetAccelAxis(int port);
int Analog_GetBrakeAxis(int port);
int Analog_IsSteerAnalog(int port);
int Analog_IsAccelAnalog(int port);
int Analog_IsBrakeAnalog(int port);
int AnalogSettings_BuildSteps(int port);

// Japanese only: empties the kanji cache.
void Options_KanjiReset(void)
{
    KanjiFont *font = g_OVL1_KanjiFont;

    if (g_GameState.language == 0) {
        KanjiFont_Reset(font);
    }
}

// Japanese only: adds a string to the kanji cache.
void Options_KanjiCache(char *sjis)
{
    KanjiFont *font = g_OVL1_KanjiFont;

    if (g_GameState.language == 0) {
        KanjiFont_CacheString(font, sjis);
    }
}

// Draws SJIS text from the kanji cache, 12 pixels a character (centred on x if center is
// set), in Japanese only.
void Options_KanjiDraw(uint *ot, char *sjis, int x, int y, int center, uint rgb)
{
    KanjiFont *font = g_OVL1_KanjiFont;
    unsigned char *p;
    uint semi;
    int w = 0;

    if (g_GameState.language != 0) {
        return;
    }
    if (center) {
        unsigned char *s;

        for (s = (unsigned char *)sjis; *s != 0; s += 2) {
            w += 12;
        }
        x -= w >> 1;
    }
    p = (unsigned char *)sjis;
    semi = 0x2000000;
    while (*p != 0) {
        KanjiGlyph *glyph = KanjiFont_FindGlyph(font, p[0] << 8 | p[1]);

        if (glyph != 0) {
            PrimFT4 *q = (PrimFT4 *)KanjiFont_AddGlyphQuad(glyph, ot, rgb | semi);

            q->x0 = x;
            q->y0 = y;
            q->x1 = x + 15;
            q->y1 = y;
            q->x2 = x;
            q->y2 = y + 31;
            q->x3 = x + 15;
            q->y3 = y + 31;
            q->tpage = (q->tpage & ~0x60) | 0x20;
        }
        x += 12;
        p += 2;
    }
}

// Japanese only: caches the current prompt.
void AnalogSettings_CachePrompt(void)
{
    if (g_GameState.language == '\0') {
        Options_KanjiReset();
        Options_KanjiCache(g_AnalogSettings_Prompt);
    }
}

// Keeps the centre in 0x40-0xc0 and inside the limits, the range inside the limits, and
// the margin inside the range.
void SteerCalib_Clamp(SteerCalib *c)
{
    int t;
    int center = c->center;

    if (center < 0x40) {
        center = 0x40;
    }
    if (center > 0xc0) {
        center = 0xc0;
    }
    if (center < c->lower) {
        center = c->lower;
    }
    if (center > c->upper) {
        center = c->upper;
    }
    t = center;
    c->center = center;
    center -= c->max;
    if (center < c->lower) {
        c->max = t - c->lower;
    }
    t = c->center;
    center = t + c->max;
    if (center > c->upper) {
        c->max = c->upper - t;
    }
    if (c->max < 0) {
        c->max = 0;
    }
    if (c->max < c->margin) {
        c->margin = c->max;
    }
    if (c->margin < 0) {
        c->margin = 0;
    }
}

void SteerCalib_SetMargin(SteerCalib *c, int v)
{
    c->margin = v;
    SteerCalib_Clamp(c);
}

void SteerCalib_SetMax(SteerCalib *c, int v)
{
    c->max = v;
    SteerCalib_Clamp(c);
}

void SteerCalib_SetCenter(SteerCalib *c, int v)
{
    c->center = v;
    SteerCalib_Clamp(c);
}

// Sets the limits, in either order.
void SteerCalib_SetLimits(SteerCalib *c, int upper, int lower)
{
    if (upper < lower) {
        int t = upper;

        upper = lower;
        lower = t;
    }
    c->upper = upper;
    c->lower = lower;
    SteerCalib_Clamp(c);
}

// The steer line of a controller without an analog wheel: its title and "N/A".
void AnalogSettings_DrawSteerNA(SteerCalib *c, uint *ot, int x, int y, int brightness, char *title, int focus)
{
    TextCtx ctx;
    TextCtx *t;
    int range = 0x80;
    int left;

    if (focus >= 0) {
        range = 0x180;
    }
    Text_Init(&ctx, 0x1e);
    t = &ctx;
    t->ot = ot;
    *(uint *)&t->clut = (*(uint *)&t->clut & 0xff9fffff) | 0x200000;
    Text_SetFont(t, &g_OVL1_FontLarge);
    t->rgb = Rgb_Lerp(D_8004C31C, D_8004C348, brightness, range);
    left = x - 0x76;
    y += 0x2c;
    Text_DrawString(t, (signed char *)title, left, y, 1);
    Text_SetFont(t, &g_OVL1_FontMedium);
    t->rgb = Rgb_Lerp(D_8004C31C, D_8004C340, brightness, range);
    Text_DrawStringRight(t, (signed char *)s_NA, x + 0x6e, y, 1);
}

// The steering line's title and its three values, brightest in the one being set.
void AnalogSettings_DrawSteerValues(SteerCalib *c, uint *ot, int x, int y, int brightness, int focus, char *title)
{
    TextCtx ctx;
    char buf[64];
    TextCtx *t;
    int centerRange = 0x80;
    int marginRange = 0x80;
    int maxRange = 0x80;
    int labelX;
    int y1;
    int y2;

    if (focus >= 0) {
        centerRange = 0x180;
        marginRange = 0x180;
        maxRange = 0x180;
        switch (focus) {
        case 0:
            centerRange = 0x80;
            break;
        case 1:
            maxRange = 0x80;
            break;
        case 2:
            marginRange = 0x80;
            break;
        }
    }
    Text_Init(&ctx, 0x1e);
    t = &ctx;
    t->ot = ot;
    *(uint *)&t->clut = (*(uint *)&t->clut & 0xff9fffff) | 0x200000;
    Text_SetFont(t, &g_OVL1_FontLarge);
    t->rgb = Rgb_Lerp(D_8004C31C, D_8004C348, brightness, 0x80);
    Text_DrawString(t, (signed char *)title, x - 0x76, y + 0x2c, 1);
    Text_SetFont(t, &g_OVL1_FontMedium);
    labelX = x + 0x48;
    y += 0x1e;
    t->rgb = Rgb_Lerp(D_8004C31C, D_8004C340, brightness, centerRange);
    Text_DrawStringRight(t, (signed char *)s_Center, labelX, y, 1);
    t->rgb = Rgb_Lerp(D_8004C31C, D_8004C340, brightness, maxRange);
    y1 = y + 0x14;
    Text_DrawStringRight(t, (signed char *)s_Center + 7, labelX, y1, 1);
    t->rgb = Rgb_Lerp(D_8004C31C, D_8004C340, brightness, marginRange);
    y2 = y + 0x28;
    Text_DrawStringRight(t, (signed char *)s_Center + 12, labelX, y2, 1);
    x += 0x6e;
    sprintf(buf, D_80022D7C, c->center - 0x80);
    t->rgb = Rgb_Lerp(D_8004C31C, D_8004C344, brightness, centerRange);
    Text_DrawStringMonoDigitsRight(t, (signed char *)buf, x, y, 1, -2, 0);
    sprintf(buf, D_80022D7C, c->max);
    t->rgb = Rgb_Lerp(D_8004C31C, D_8004C344, brightness, maxRange);
    Text_DrawStringMonoDigitsRight(t, (signed char *)buf, x, y1, 1, -2, 0);
    sprintf(buf, D_80022D7C, c->margin);
    t->rgb = Rgb_Lerp(D_8004C31C, D_8004C344, brightness, marginRange);
    Text_DrawStringMonoDigitsRight(t, (signed char *)buf, x, y2, 1, -2, 0);
}

// The wheel's gauge: the values, the range and margin bands around the centre, the live
// position and, while setting the limits (state 2), the two ends so far. In state 1 the
// parts outside the limits are shaded.
void AnalogSettings_DrawSteerGauge(SteerCalib *c, uint *ot, char *title, int x, int y, int brightness, int raw,
                                   int focus, int state, int end0, int end1)
{
    RECT rect;
    int marks[9];
    RECT *tile;
    int left = x - 0x80;
    int top = y - 6;
    int w;

    marks[1] = left + raw;
    marks[2] = left + c->center;
    marks[3] = left + c->center + c->margin;
    marks[4] = left + c->center - c->margin;
    marks[5] = left + c->center + c->max;
    marks[6] = left + c->center - c->max;
    marks[7] = left + c->upper;
    marks[8] = left + c->lower;
    rect.x = x - 0x81;
    rect.y = y - 7;
    rect.w = 0x102;
    rect.h = 0xe;
    Prim_AddRectOutline(ot, Rgb_Lerp(D_8004C31C, D_8004C328, brightness, 0x80), &rect);
    rect.x = x - 0x82;
    rect.y = y - 8;
    rect.w = 0x104;
    rect.h = 0x10;
    Prim_AddRectOutline(ot, Rgb_Lerp(D_8004C31C, D_8004C328, brightness, 0x80), &rect);
    Prim_AddDrawMode(ot, 0x20);
    switch (state) {
    case 0:
    case 1:
        AnalogSettings_DrawSteerValues(c, ot, x, y, brightness, focus, title);
        if (raw >= 0) {
            tile = Prim_AddTile(ot, Rgb_Lerp(D_8004C320, D_8004C33C, brightness, 0x80));
            tile->w = 1;
            tile->h = 0xc;
            tile->y = top;
            tile->x = marks[1];
            Menu_DrawScrollArrow(marks, ot, marks[1], y - 0x16, 3, 0xe, 0);
        }
        tile = Prim_AddTile(ot, Rgb_Lerp(D_8004C320, D_8004C338, brightness, 0x80));
        tile->w = 1;
        tile->h = 6;
        tile->y = top;
        tile->x = marks[2];
        tile = Prim_AddTile(ot, Rgb_Lerp(D_8004C31C, D_8004C32C, brightness, 0x80));
        tile->x = marks[4];
        tile->h = 0xc;
        tile->y = top;
        tile->w = marks[3] - marks[4];
        tile = Prim_AddTile(ot, Rgb_Lerp(D_8004C31C, D_8004C330, brightness, 0x80));
        tile->x = marks[3];
        tile->w = marks[5] - marks[3];
        tile->y = top;
        tile->h = 0xc;
        tile = Prim_AddTile(ot, Rgb_Lerp(D_8004C31C, D_8004C330, brightness, 0x80));
        tile->x = marks[6];
        tile->w = marks[4] - marks[6];
        tile->y = top;
        tile->h = 0xc;
        if (state == 1) {
            w = 0x100 - (marks[7] - left);
            if (w > 0) {
                tile = Prim_AddTile(ot, Rgb_Lerp(D_8004C31C, D_8004C334, brightness, 0x80));
                tile->w = w;
                tile->y = top;
                tile->h = 0xc;
                tile->x = marks[7];
            }
            w = marks[8] - left;
            if (w > 0) {
                tile = Prim_AddTile(ot, Rgb_Lerp(D_8004C31C, D_8004C334, brightness, 0x80));
                tile->x = left;
                tile->w = w;
                tile->y = top;
                tile->h = 0xc;
            }
        }
        break;
    case 2:
        if (end0 >= 0) {
            int ex = left + end0;

            tile = Prim_AddTile(ot, Rgb_Lerp(D_8004C320, D_8004C33C, brightness, 0x80));
            tile->w = 1;
            tile->h = 0xc;
            tile->x = ex;
            tile->y = top;
        }
        if (end1 >= 0) {
            int ex = left + end1;

            tile = Prim_AddTile(ot, Rgb_Lerp(D_8004C320, D_8004C33C, brightness, 0x80));
            tile->w = 1;
            tile->h = 0xc;
            tile->x = ex;
            tile->y = top;
        }
        break;
    }
    tile = Prim_AddTile(ot, Rgb_Lerp(D_8004C31C, D_8004C324, brightness, 0x80));
    tile->w = 0x100;
    tile->h = 0xc;
    tile->x = left;
    tile->y = top;
    Prim_AddDrawMode(ot, 0);
}

// Keeps max in 2-0xff and the margin in 1 to max - 1.
void PedalCalib_Clamp(PedalCalib *c)
{
    if (c->max > 0xff) {
        c->max = 0xff;
    }
    if (c->max < 2) {
        c->max = 2;
    }
    if (c->margin >= c->max) {
        c->margin = c->max - 1;
    }
    if (c->margin <= 0) {
        c->margin = 1;
    }
}

void PedalCalib_SetMargin(PedalCalib *c, int v)
{
    c->margin = v;
    PedalCalib_Clamp(c);
}

void PedalCalib_SetMax(PedalCalib *c, int v)
{
    c->max = v;
    PedalCalib_Clamp(c);
}

// The pedal line of a controller without an analog pedal: its title and "N/A".
void AnalogSettings_DrawPedalNA(PedalCalib *c, uint *ot, int x, int y, int brightness, char *title, int focus)
{
    TextCtx ctx;
    TextCtx *t;
    int range = 0x80;
    int left;

    if (focus >= 0) {
        range = 0x180;
    }
    Text_Init(&ctx, 0x1e);
    t = &ctx;
    t->ot = ot;
    *(uint *)&t->clut = (*(uint *)&t->clut & 0xff9fffff) | 0x200000;
    Text_SetFont(t, &g_OVL1_FontLarge);
    t->rgb = Rgb_Lerp(D_8004C31C, D_8004C348, brightness, range);
    left = x - 0x76;
    y += 0x2c;
    Text_DrawString(t, (signed char *)title, left, y, 1);
    Text_SetFont(t, &g_OVL1_FontMedium);
    t->rgb = Rgb_Lerp(D_8004C31C, D_8004C340, brightness, range);
    Text_DrawStringRight(t, (signed char *)s_NA, x + 0x6e, y, 1);
}

// The pedal line's title and its two values, brightest in the one being set.
void AnalogSettings_DrawPedalValues(PedalCalib *c, uint *ot, int x, int y, int brightness, int focus, char *title)
{
    TextCtx ctx;
    char buf[64];
    TextCtx *t;
    int maxRange = 0x80;
    int marginRange = 0x80;
    int labelX;
    int y1;

    if (focus >= 0) {
        maxRange = 0x180;
        marginRange = 0x180;
        switch (focus) {
        case 0:
            maxRange = 0x80;
            break;
        case 1:
            marginRange = 0x80;
            break;
        }
    }
    Text_Init(&ctx, 0x1e);
    t = &ctx;
    t->ot = ot;
    *(uint *)&t->clut = (*(uint *)&t->clut & 0xff9fffff) | 0x200000;
    Text_SetFont(t, &g_OVL1_FontLarge);
    t->rgb = Rgb_Lerp(D_8004C31C, D_8004C348, brightness, 0x80);
    Text_DrawString(t, (signed char *)title, x - 0x76, y + 0x2c, 1);
    Text_SetFont(t, &g_OVL1_FontMedium);
    labelX = x + 0x48;
    y += 0x1e;
    t->rgb = Rgb_Lerp(D_8004C31C, D_8004C340, brightness, maxRange);
    Text_DrawStringRight(t, (signed char *)s_Max, labelX, y, 1);
    t->rgb = Rgb_Lerp(D_8004C31C, D_8004C340, brightness, marginRange);
    y1 = y + 0x14;
    Text_DrawStringRight(t, (signed char *)s_Max + 5, labelX, y1, 1);
    x += 0x6e;
    sprintf(buf, D_80022D7C, c->max);
    t->rgb = Rgb_Lerp(D_8004C31C, D_8004C344, brightness, maxRange);
    Text_DrawStringMonoDigitsRight(t, (signed char *)buf, x, y, 1, -2, 0);
    sprintf(buf, D_80022D7C, c->margin);
    t->rgb = Rgb_Lerp(D_8004C31C, D_8004C344, brightness, marginRange);
    Text_DrawStringMonoDigitsRight(t, (signed char *)buf, x, y1, 1, -2, 0);
}

// A pedal's gauge: the values, the margin and the range, and the live position.
void AnalogSettings_DrawPedalGauge(PedalCalib *c, uint *ot, char *title, int x, int y, int brightness, int raw,
                                   int focus)
{
    RECT rect;
    int marks[3];
    RECT *tile;
    int left = x - 0x80;
    int top = y - 6;
    int margin = left + c->margin;

    marks[1] = left + raw;
    marks[2] = left + c->max;
    AnalogSettings_DrawPedalValues(c, ot, x, y, brightness, focus, title);
    rect.x = x - 0x81;
    rect.y = y - 7;
    rect.w = 0x102;
    rect.h = 0xe;
    Prim_AddRectOutline(ot, Rgb_Lerp(D_8004C31C, D_8004C328, brightness, 0x80), &rect);
    rect.x = x - 0x82;
    rect.y = y - 8;
    rect.w = 0x104;
    rect.h = 0x10;
    Prim_AddRectOutline(ot, Rgb_Lerp(D_8004C31C, D_8004C328, brightness, 0x80), &rect);
    Prim_AddDrawMode(ot, 0x20);
    if (raw >= 0) {
        tile = Prim_AddTile(ot, Rgb_Lerp(D_8004C320, D_8004C33C, brightness, 0x80));
        tile->w = 1;
        tile->h = 0xc;
        tile->y = top;
        tile->x = marks[1];
        Menu_DrawScrollArrow(marks, ot, marks[1], y - 0x16, 3, 0xe, 0);
    }
    tile = Prim_AddTile(ot, Rgb_Lerp(D_8004C31C, D_8004C32C, brightness, 0x80));
    tile->x = left;
    tile->w = margin - left;
    tile->y = top;
    tile->h = 0xc;
    tile = Prim_AddTile(ot, Rgb_Lerp(D_8004C31C, D_8004C330, brightness, 0x80));
    tile->x = margin;
    tile->w = marks[2] - margin;
    tile->y = top;
    tile->h = 0xc;
    tile = Prim_AddTile(ot, Rgb_Lerp(D_8004C31C, D_8004C324, brightness, 0x80));
    tile->x = left;
    tile->w = 0x100;
    tile->y = top;
    tile->h = 0xc;
    Prim_AddDrawMode(ot, 0);
}

// NeGcon map of the steering slot.
int Analog_GetSteerMap(int port)
{
    return g_GameState.ports[port].buttonMaps[2][0];
}

// NeGcon map of the accelerator slot.
int Analog_GetAccelMap(int port)
{
    return g_GameState.ports[port].buttonMaps[2][2];
}

// NeGcon map of the brake slot.
int Analog_GetBrakeMap(int port)
{
    return g_GameState.ports[port].buttonMaps[2][3];
}

// The NeGcon axis of the steer slot, -1 when it's a button.
int Analog_GetSteerAxis(int port)
{
    int map = Analog_GetSteerMap(port);
    int a;

    if (map < 0x80) {
        return -1;
    }
    a = map - 0x80;
    if ((unsigned int)a < 4) {
        return a;
    }
    return -1;
}

// The NeGcon axis of the accel slot, -1 when it's a button.
int Analog_GetAccelAxis(int port)
{
    int map = Analog_GetAccelMap(port);
    int a;

    if (map < 0x80) {
        return -1;
    }
    a = map - 0x80;
    if ((unsigned int)a < 4) {
        return a;
    }
    return -1;
}

// The NeGcon axis of the brake slot, -1 when it's a button.
int Analog_GetBrakeAxis(int port)
{
    int map = Analog_GetBrakeMap(port);
    int a;

    if (map < 0x80) {
        return -1;
    }
    a = map - 0x80;
    if ((unsigned int)a < 4) {
        return a;
    }
    return -1;
}

// The port has a NeGcon with an axis on the steer slot.
int Analog_IsSteerAnalog(int port)
{
    return (port + g_CurrentMenu->pads)->type == 2 ? Analog_GetSteerMap(port) >= 0x80 : 0;
}

// The port has a NeGcon with an axis on the accel slot.
int Analog_IsAccelAnalog(int port)
{
    return (port + g_CurrentMenu->pads)->type == 2 ? Analog_GetAccelMap(port) >= 0x80 : 0;
}

// The port has a NeGcon with an axis on the brake slot.
int Analog_IsBrakeAnalog(int port)
{
    return (port + g_CurrentMenu->pads)->type == 2 ? Analog_GetBrakeMap(port) >= 0x80 : 0;
}

// The twist axis (axis 0) of a saved calibration; the limits are the axis's full range.
void SteerCalib_Load(NeGconCalib *calib, SteerCalib *c, int axis)
{
    NeGconCalib *saved = calib;

    if (axis == 0) {
        int center = (saved->twist[1] + saved->twist[2]) >> 1;
        int max = saved->twist[3] - center;
        int margin = saved->twist[2] - center;

        c->margin = margin;
        c->center = center;
        c->max = max;
        SteerCalib_SetLimits(c, 0xff, 0);
    }
}

// A pedal axis (1 to 3) of a saved calibration.
void PedalCalib_Load(NeGconCalib *calib, PedalCalib *c, int axis)
{
    axis -= 1;
    if ((unsigned int)axis < 3) {
        int max = calib->axes[axis].max;
        int min = calib->axes[axis].min;

        c->max = max;
        c->margin = min;
        PedalCalib_Clamp(c);
    }
}

// Back to the saved form: the twist axis's four points.
void SteerCalib_Save(NeGconCalib *calib, SteerCalib *c, int axis)
{
    if (axis == 0) {
        int a = c->center - c->max;
        int b;
        int d;
        int e;

        calib->twist[0] = a;
        b = c->center - c->margin;
        calib->twist[1] = b;
        d = c->center + c->margin;
        calib->twist[2] = d;
        e = c->center + c->max;
        calib->twist[3] = e;
    }
}

// Back to the saved form: a pedal axis's range.
void PedalCalib_Save(NeGconCalib *calib, PedalCalib *c, int axis)
{
    axis -= 1;
    if ((unsigned int)axis < 3) {
        int v = c->margin;

        calib->axes[axis].min = v;
        v = c->max;
        calib->axes[axis].max = v;
    }
}

// Lists the calibration steps of the port's NeGcon, then Exit; returns whether there is
// any to do.
int AnalogSettings_BuildSteps(int port)
{
    int count = 0;
    int type;

    g_AnalogSettings_SteerAnalog = 0;
    g_AnalogSettings_AccelAnalog = 0;
    g_AnalogSettings_BrakeAnalog = 0;
    type = (port + g_CurrentMenu->pads)->type;
    if (type != 2) {
        g_AnalogSettings_StepCount = 0;
        return count;
    }
    if (Analog_IsSteerAnalog(port)) {
        AnalogStep *a = g_AnalogSettings_Steps;
        AnalogStep *b;
        AnalogStep *d;

        count = 1;
        b = a + count;
        b->sub = count;
        count = 2;
        d = a + count;
        d->sub = count;
        count = 3;
        g_AnalogSettings_SteerAnalog = 1;
        g_AnalogSettings_Steps[0].kind = 0;
        a->prompt = s_SetTheCenterOfSteering;
        a->sub = 0;
        b->kind = 0;
        b->prompt = s_SetTheCenterOfSteering + 0x21;
        d->kind = 0;
        d->prompt = s_SetTheCenterOfSteering + 0x50;
    }
    if (Analog_IsAccelAnalog(port)) {
        AnalogStep *a = &g_AnalogSettings_Steps[count++];
        AnalogStep *b = &g_AnalogSettings_Steps[count++];

        g_AnalogSettings_AccelAnalog = 1;
        a->kind = 1;
        a->sub = 0;
        a->prompt = s_SetTheLockPositionOfAcceleration;
        b->kind = 1;
        b->sub = 1;
        b->prompt = s_SetTheLockPositionOfAcceleration + 0x31;
    }
    if (Analog_IsBrakeAnalog(port)) {
        AnalogStep *a = &g_AnalogSettings_Steps[count++];
        AnalogStep *b = &g_AnalogSettings_Steps[count++];

        g_AnalogSettings_BrakeAnalog = 1;
        a->kind = type;
        a->sub = 0;
        a->prompt = s_SetTheLockPositionOfBraking;
        b->kind = type;
        b->sub = 1;
        b->prompt = s_SetTheLockPositionOfBraking + 0x2b;
    }
    {
        AnalogStep *a = &g_AnalogSettings_Steps[count++];

        a->kind = 3;
        a->prompt = s_Exit_2;
    }
    g_AnalogSettings_StepCount = count;
    return count >= 2;
}

// Loads the port's calibration and resets the three gauges' bars.
void AnalogSettings_Init(AnalogSettings *s, int port)
{
    GameState *state = &g_GameState;
    NeGconCalib *calib;

    s->mode = 3;
    calib = &(port + state->ports)->negconCalib;
    s->port = port;
    SteerCalib_Load(calib, &s->steer, Analog_GetSteerAxis(port));
    PedalCalib_Load(calib, &s->accel, Analog_GetAccelAxis(port));
    PedalCalib_Load(calib, &s->brake, Analog_GetBrakeAxis(port));
    s->icons[0] = D_8004C350;
    s->icons[0].t = 0;
    s->icons[1] = D_8004C36C;
    s->icons[1].t = 0;
    s->icons[2] = D_8004C388;
    s->icons[2].t = 0;
}

// Init again, with the bars open.
void AnalogSettings_Reset(AnalogSettings *s)
{
    AnalogSettings_Init(s, s->port);
    s->icons[0].t = 0;
    s->icons[1].t = 0;
    s->icons[2].t = 0;
}

// Closes the three bars.
void AnalogSettings_HideIcons(AnalogSettings *s)
{
    WipeBar *bar0 = &s->icons[0];
    WipeBar *bar1;
    WipeBar *bar2;

    bar0->t = ~bar0->duration;
    bar1 = &s->icons[1];
    bar1->t = ~bar1->duration;
    bar2 = &s->icons[2];
    bar2->t = ~bar2->duration;
}

// mode = 3 (inactive).
void AnalogSettings_Close(AnalogSettings *s)
{
    s->mode = 3;
}

// Starts on the first step, if the port's controller has any; returns whether it does.
int AnalogSettings_Open(AnalogSettings *s)
{
    GameState *state;
    NeGconCalib *calib;

    if ((AnalogSettings_BuildSteps(s->port) ^ 1) != 0) {
        return 0;
    }
    g_AnalogSettings_RawSteer = 0;
    g_AnalogSettings_RawAccel = 0;
    s->step = 0;
    g_AnalogSettings_RawBrake = 0;
    if (g_AnalogSettings_SteerAnalog) {
        s->mode = 0;
        g_AnalogSettings_Prompt = s_TurnWheelToFarRightAndPressR;
    } else {
        s->mode = 2;
        g_AnalogSettings_Prompt = g_AnalogSettings_Steps[s->step].prompt;
    }
    AnalogSettings_CachePrompt();
    state = &g_GameState;
    calib = &(s->port + state->ports)->negconCalib;
    SteerCalib_Load(calib, &s->steer, Analog_GetSteerAxis(s->port));
    PedalCalib_Load(calib, &s->accel, Analog_GetAccelAxis(s->port));
    PedalCalib_Load(calib, &s->brake, Analog_GetBrakeAxis(s->port));
    return 1;
}

// Each frame. Mode 0 and 1 take the wheel's two ends, mode 2 goes through the steps;
// returns -2 when cancelled, 0 when saved, -1 otherwise.
int AnalogSettings_Update(AnalogSettings *s, PadState *unused)
{
    BaseMenu *menu = g_CurrentMenu;
    int port = s->port;
    PadDevice *dev = &menu->pads[port];
    PadState *pad = &menu->padState[port];
    PadState *other = &menu->padState[1 - port];
    volatile unsigned char *raw = g_PadSystem.raw[port].analog;
    int axis;
    int pressed;
    int step;
    int kind;
    int delta;
    AnalogStep *cur;
    GameState *state;
    NeGconCalib *calib;

    WipeBar_Update(&s->icons[0]);
    WipeBar_Update(&s->icons[1]);
    WipeBar_Update(&s->icons[2]);
    if (s->mode == 3) {
        return -1;
    }
    if (dev->type != 2) {
        Sfx_PlayMenu(0);
        s->mode = 3;
        return -2;
    }
    axis = Analog_GetSteerAxis(s->port);
    if (axis >= 0) {
        g_AnalogSettings_RawSteer = raw[axis];
    }
    axis = Analog_GetAccelAxis(s->port);
    if (axis >= 0) {
        g_AnalogSettings_RawAccel = raw[axis];
    }
    axis = Analog_GetBrakeAxis(s->port);
    if (axis >= 0) {
        g_AnalogSettings_RawBrake = raw[axis];
    }
    switch (s->mode) {
    case 0:
        pressed = pad->pressed | other->pressed;
        if (pressed & 0x11000) {
            g_AnalogSettings_Prompt = s_TurnWheelToFarLeftAndPressR;
            g_AnalogSettings_LockRight = (unsigned short)g_AnalogSettings_RawSteer;
            s->mode = 1;
            AnalogSettings_CachePrompt();
            Sfx_PlayMenu(1);
            return -1;
        }
        if (pressed & 0x500) {
            Sfx_PlayMenu(2);
            s->mode = 3;
            return -2;
        }
        break;
    case 1:
        pressed = pad->pressed | other->pressed;
        if (pressed & 0x11000) {
            g_AnalogSettings_LockLeft = (unsigned short)g_AnalogSettings_RawSteer;
            SteerCalib_SetLimits(&s->steer, g_AnalogSettings_LockRight, g_AnalogSettings_LockLeft);
            s->mode = 2;
            Sfx_PlayMenu(1);
            g_AnalogSettings_Prompt = g_AnalogSettings_Steps[s->step].prompt;
            AnalogSettings_CachePrompt();
            return -1;
        }
        if (pressed & 0x500) {
            Sfx_PlayMenu(2);
            s->mode = 3;
            return -2;
        }
        break;
    case 2:
        step = s->step;
        kind = g_AnalogSettings_Steps[step].kind;
        pressed = pad->pressed;
        delta = 0;
        if (kind == 3 && (pressed & 0xa00)) {
            state = &g_GameState;
            calib = &(s->port + state->ports)->negconCalib;
            SteerCalib_Save(calib, &s->steer, Analog_GetSteerAxis(s->port));
            PedalCalib_Save(calib, &s->accel, Analog_GetAccelAxis(s->port));
            PedalCalib_Save(calib, &s->brake, Analog_GetBrakeAxis(s->port));
            s->mode = kind;
            return 0;
        }
        if (pressed & 0x11000) {
            cur = &g_AnalogSettings_Steps[s->step];
            switch (cur->kind) {
            case 0: {
                int rawSteer = (unsigned short)g_AnalogSettings_RawSteer;
                int d = rawSteer - s->steer.center;

                if (d < 0) {
                    d = -d;
                }
                switch (cur->sub) {
                case 0:
                    SteerCalib_SetCenter(&s->steer, rawSteer);
                    break;
                case 1:
                    SteerCalib_SetMax(&s->steer, d);
                    break;
                case 2:
                    SteerCalib_SetMargin(&s->steer, d);
                    break;
                }
                break;
            }
            case 1:
                switch (cur->sub) {
                case 0:
                    PedalCalib_SetMax(&s->accel, (unsigned short)g_AnalogSettings_RawAccel);
                    break;
                case 1:
                    PedalCalib_SetMargin(&s->accel, (unsigned short)g_AnalogSettings_RawAccel);
                    break;
                }
                break;
            case 2:
                switch (cur->sub) {
                case 0:
                    PedalCalib_SetMax(&s->brake, (unsigned short)g_AnalogSettings_RawBrake);
                    break;
                case 1:
                    PedalCalib_SetMargin(&s->brake, (unsigned short)g_AnalogSettings_RawBrake);
                    break;
                }
                break;
            }
            Sfx_PlayMenu(1);
        }
        pressed = pad->pressed | pad->repeat;
        if (pressed & 4) {
            delta--;
        }
        if (pressed & 8) {
            delta++;
        }
        if (delta != 0) {
            cur = &g_AnalogSettings_Steps[s->step];
            switch (cur->kind) {
            case 0:
                switch (cur->sub) {
                case 0:
                    SteerCalib_SetCenter(&s->steer, s->steer.center + delta);
                    break;
                case 1:
                    SteerCalib_SetMax(&s->steer, s->steer.max + delta);
                    break;
                case 2:
                    SteerCalib_SetMargin(&s->steer, s->steer.margin + delta);
                    break;
                }
                break;
            case 1:
                switch (cur->sub) {
                case 0:
                    PedalCalib_SetMax(&s->accel, s->accel.max + delta);
                    break;
                case 1:
                    PedalCalib_SetMargin(&s->accel, s->accel.margin + delta);
                    break;
                }
                break;
            case 2:
                switch (cur->sub) {
                case 0:
                    PedalCalib_SetMax(&s->brake, s->brake.max + delta);
                    break;
                case 1:
                    PedalCalib_SetMargin(&s->brake, s->brake.margin + delta);
                    break;
                }
                break;
            }
            Sfx_PlayMenu(5);
        }
        pressed = pad->pressed | pad->repeat;
        if (pressed & 1) {
            step--;
            if (step < 0) {
                step = g_AnalogSettings_StepCount - 1;
            }
        }
        if (pressed & 2) {
            step++;
            if (step >= g_AnalogSettings_StepCount) {
                step = 0;
            }
        }
        if (s->step != step) {
            s->step = step;
            Sfx_PlayMenu(6);
            g_AnalogSettings_Prompt = g_AnalogSettings_Steps[s->step].prompt;
            AnalogSettings_CachePrompt();
        }
        break;
    }
    return -1;
}

// Each frame: the prompt, the three gauges (or N/A where the controller has no such
// axis), their bars and icons, and Exit.
void AnalogSettings_Draw(AnalogSettings *s, uint *ot, int x, int y, int brightness)
{
    TextCtx ctx;
    TextCtx *t;
    int rawSteer = (unsigned short)g_AnalogSettings_RawSteer;
    int rawAccel = (unsigned short)g_AnalogSettings_RawAccel;
    int rawBrake = (unsigned short)g_AnalogSettings_RawBrake;
    int showSteer = 1;
    int showAccel = 1;
    int showBrake = 1;
    int showExit = 0;
    int state = 0;
    int end0 = -1;
    int end1 = -1;
    int steerFocus = -1;
    int accelFocus = -1;
    int brakeFocus = -1;
    AnalogStep *cur;
    SpriteDesc *icon;
    uint *sprite;
    int range;

    Text_Init(&ctx, 0x1e);
    t = &ctx;
    t->ot = ot;
    *(uint *)&t->clut = (*(uint *)&t->clut & 0xff9fffff) | 0x200000;
    switch (s->mode) {
    case 2:
        Text_SetFont(t, &g_OVL1_FontMedium);
        t->rgb = Rgb_Lerp(D_8004C31C, D_8004C34C, brightness, 0x80);
        Text_DrawCentered(t, (signed char *)s_PressROrStartToSetPosition, 0xb0, 0x84, showBrake);
        // fall through
    case 0:
    case 1:
        Options_KanjiDraw(ot, g_AnalogSettings_Prompt, 0xb0, 0x1ae, showBrake, 0x747474);
        break;
    }
    switch (s->mode) {
    case 0:
        state = 2;
        end0 = rawSteer;
        showBrake = 0;
        showAccel = 0;
        showSteer = 0;
        break;
    case 1:
        end0 = g_AnalogSettings_LockRight;
        state = 2;
        showBrake = 0;
        showAccel = 0;
        showSteer = 0;
        end1 = rawSteer;
        break;
    case 2:
        showExit = 1;
        cur = &g_AnalogSettings_Steps[s->step];
        state = 1;
        switch (cur->kind) {
        case 0:
            steerFocus = cur->sub;
            accelFocus = 0x40;
            brakeFocus = 0x40;
            break;
        case 1:
            accelFocus = cur->sub;
            steerFocus = 0x40;
            brakeFocus = 0x40;
            break;
        case 2:
            steerFocus = 0x40;
            accelFocus = 0x40;
            brakeFocus = cur->sub;
            break;
        case 3:
            steerFocus = 0x40;
            accelFocus = 0x40;
            brakeFocus = 0x40;
            break;
        }
        break;
    case 3:
        rawSteer = -1;
        rawBrake = -1;
        rawAccel = -1;
        break;
    }
    if (Analog_IsSteerAnalog(s->port)) {
        AnalogSettings_DrawSteerGauge(&s->steer, ot, s_Steering, x, y, brightness, rawSteer, steerFocus, state, end0, end1);
    } else {
        AnalogSettings_DrawSteerNA(&s->steer, ot, x, y, brightness, s_Steering, steerFocus);
    }
    if (showSteer) {
        WipeBar_Draw(&s->icons[0], ot, x - 0x7a, y + 0x14);
        Prim_AddDrawMode(ot, 0x220);
    }
    if (showAccel) {
        if (Analog_IsAccelAnalog(s->port)) {
            AnalogSettings_DrawPedalGauge(&s->accel, ot, s_Acceleration, x, y + 0x60, brightness, rawAccel, accelFocus);
            icon = (SpriteDesc *)KeyConfig_GetNeGconIcon(Analog_GetAccelMap(s->port));
            if (icon != 0) {
                sprite = (uint *)Prim_AddSprite(ot, brightness | brightness << 8 | brightness << 16);
                sprite[0] = (x - ((short)icon->wh >> 1)) + ((y - (((short)(icon->wh >> 16) >> 1) - 0x84)) << 16);
                sprite[1] = icon->uvClut;
                sprite[2] = icon->wh;
                Prim_AddDrawMode(ot, icon->tpage | 0x20);
            }
        } else {
            AnalogSettings_DrawPedalNA(&s->accel, ot, x, y + 0x60, brightness, s_Acceleration, accelFocus);
        }
        WipeBar_Draw(&s->icons[1], ot, x - 0x7a, y + 0x74);
        Prim_AddDrawMode(ot, 0x220);
    }
    if (showBrake) {
        if (Analog_IsBrakeAnalog(s->port)) {
            AnalogSettings_DrawPedalGauge(&s->brake, ot, s_Brake, x, y + 0xb4, brightness, rawBrake, brakeFocus);
            icon = (SpriteDesc *)KeyConfig_GetNeGconIcon(Analog_GetBrakeMap(s->port));
            if (icon != 0) {
                sprite = (uint *)Prim_AddSprite(ot, brightness | brightness << 8 | brightness << 16);
                sprite[0] = (x - ((short)icon->wh >> 1)) + ((y - (((short)(icon->wh >> 16) >> 1) - 0xd8)) << 16);
                sprite[1] = icon->uvClut;
                sprite[2] = icon->wh;
                Prim_AddDrawMode(ot, icon->tpage | 0x20);
            }
        } else {
            AnalogSettings_DrawPedalNA(&s->brake, ot, x, y + 0xb4, brightness, s_Brake, brakeFocus);
        }
        WipeBar_Draw(&s->icons[2], ot, x - 0x7a, y + 0xc8);
        Prim_AddDrawMode(ot, 0x220);
    }
    if (showExit) {
        range = 0x180;
        if (g_AnalogSettings_Steps[s->step].kind == 3) {
            range = 0x80;
        }
        Text_SetFont(&ctx, &g_OVL1_FontMedium);
        ctx.rgb = Rgb_Lerp(D_8004C31C, D_8004C340, brightness, range);
        Text_DrawStringMonoDigitsRight(&ctx, (signed char *)s_Exit, x + 0x48, y - 0x10, 1, -2, 0);
    }
}
}
