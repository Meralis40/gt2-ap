// OVL0 8002bcf0-8002ee98 (hud): HUD.
// The functions that don't match yet are the original assembly.
#include "common.h"
#include "gamestate.h"
#include "gte.h"

// An entry of g_Hud_Sprites.
struct HudSprite {
    unsigned char u;
    unsigned char v;
    unsigned short clut;
    unsigned short w;
    unsigned short h;
    unsigned short tpage;
    unsigned short pad;
};

// g_Hud_Sprites seen from its first w.
struct HudSpriteSize {
    unsigned short w;
    unsigned short h;
    unsigned short tpage;
    unsigned short pad;
    int uvClut;
};

// A gauge needle: its colours, then the half widths and lengths of its two ends.
struct HudNeedle {
    unsigned int rgb0;
    unsigned int rgb1;
    int x0;
    int x1;
    int y0;
    int y1;
};

// A car's HUD state (+0x880 of RaceVehicle).
struct VehicleHud {
    short index; /* the dial's sprite, also its TIM */
    short shiftRpm; /* the red zone starts here */
    short redline;
    short range; /* rpm at the end of the dial, a multiple of 1000 */
    short segments; /* of the dial, 4 per 1000 rpm */
    short redSegment;
    int dial[40][2]; /* inner and outer point of each segment's edge, x and y */
    unsigned char unk14c[0xc0];
    unsigned char lapInvalid;
    signed char message;
    short lapTimeTimer;
    short refTimeTimer;
    short messageTimer;
    int lapTimeShown;
    void *lapLabel;
    int refTime;
    int refTime2;
};
#define VEHICLE_HUD(car) ((VehicleHud *)(car)->unk880)

// Fills a sprite at (x, y), centred, with an entry of g_Hud_Sprites.
#define SPRITE_SET(prim, sprite, x, y)     do {         *(int *)(prim) = (x) - ((short)(sprite)->w >> 1) + (((y) - ((short)(sprite)->h >> 1)) << 16);         ((int *)(prim))[1] = *(int *)(sprite);         ((int *)(prim))[2] = *(int *)&(sprite)->w;     } while (0)

#define HUD_TEXT_BLEND(text, bits) (*(unsigned int *)&(text)->clut = (*(unsigned int *)&(text)->clut & 0xff9fffff) | (bits))
#define RACE_SETUP Hud_RaceSetup()

// A sprite of g_Hud_Sprites at (x, y) by index.
#define HUD_SIZES ((HudSpriteSize *)((char *)g_Hud_Sprites + 4))
#define SPRITE_SET_INDEX(prim, index, x, y)     do {         *(int *)(prim) = (x) - ((short)HUD_SIZES[index].w >> 1) + (((y) - ((short)HUD_SIZES[index].h >> 1)) << 16);         ((int *)(prim))[1] = *(int *)&g_Hud_Sprites[index];         ((int *)(prim))[2] = *(int *)&g_Hud_Sprites[index].w;     } while (0)

extern "C" {
extern RECT g_Hud_TimClutRect0;
extern RECT g_Hud_TimImageRect0;
extern RECT g_Hud_TimClutRect1;
extern RECT g_Hud_TimImageRect1;
extern HudSprite g_Hud_Sprites[];
extern unsigned char g_Hud_ClockFrame;
extern int g_HudTexFile[];
extern signed char D_8002F868[];
extern signed char D_8002F874[];
extern HudNeedle D_8002F880;
extern HudNeedle D_8002F898;
extern HudNeedle D_8002F8B0;
extern HudNeedle D_8002F8C8;
extern GouraudQuad D_8002F8E0;
extern unsigned char D_8002F8F0[];
extern unsigned char D_8002F8F4[];
extern unsigned int D_8002F8F8[];
extern unsigned char D_8002F904[];
extern unsigned char D_8002F908[];
extern signed char D_8002F278[];
extern signed char D_8002F27C[];
extern char D_8002F280[];
extern int D_8002F2E8[];
extern signed char D_8002F318[];
extern unsigned char D_801C98A2;
extern short g_SinTable[];
extern Font D_80093124;
extern Font D_80093130;
extern GameState g_GameState;
extern RaceSetup g_RaceSetup;
extern RaceMem g_Race_IsDemo;
extern RaceWorld g_RaceWorld;
extern RaceRecord g_RaceRecords;
extern int g_RaceClock;
extern int g_TyreWearEnabled;
extern void *g_EventDataBuf;
extern char *g_PacketPtr;
extern char *g_PacketEnd;
extern char sR_Mph[];
extern signed char sR_Record[];
extern signed char sR_TotalTime[];
extern signed char sR_LapTime[];
extern signed char sR_Lap[];
extern signed char sR_TimeInvalid[];
extern signed char sR_Crash[];
extern signed char sR_Replay[];
extern signed char sR_WrongWay[];
extern signed char sR_TooFastToStop[];
extern signed char sR_SkippedPitStop[];
extern signed char sR_NoNeedToPitStop[];
extern signed char sR_LimitedSpeed[];
extern signed char sR_PitWorkInProgress[];
extern signed char sR_FullThrottle[];
extern signed char sR_Ready[];
extern signed char sR_Go[];
extern signed char sR_WARNINGWrongWay[];
extern signed char sR_WARNINGDonTGoBack[];
extern signed char sR_Exit[];

int Car_GetDisplayLap(CarPhys *car);
void Color_HeatRamp(signed char heat, int *r, int *g, int *b);
unsigned int PackedTime_ToMs(void *packed, int rank);
void Countdown_Init(short *cnt, short introFrames);
void Countdown_Update(short *cnt, void *view);
void Countdown_Draw(short *cnt, void *ot);
void Result_Init(short *res);
int Result_Update(short *res, PadState *pad);
void Result_Draw(short *res, uint *ot);
void Hud_DrawCourseMap(void *world, void *ot, int x, int y, int player);
void Hud_DrawLapPanel(uint *ot, RaceVehicle *car, int x, int y);
void Hud_DrawLapPopup(uint *ot, RaceVehicle *car, int x, int y);
void Hud_DrawMessage(uint *ot, RaceVehicle *car, int x, int y);
void Hud_DrawPedals(uint *ot, RaceVehicle *car, int x, int y);
void Hud_DrawRecordPanel(uint *ot, RaceVehicle *car, int x, int y);
void Hud_DrawSteering(uint *ot, RaceVehicle *car, int x, int y);
void Hud_DrawTyres(uint *ot, RaceVehicle *car, int x, int y);
void Hud_DrawBoostGauge(uint *ot, RaceVehicle *car, int x, int y);
void Tacho_DrawDial(uint *ot, RaceVehicle *car, int x, int y);
void Tacho_DrawNeedle(uint *ot, RaceVehicle *car, int x, int y);

// g_RaceSetup, as the game reaches it: g_GameState plus 0xbf7c, added at run time.
static inline RaceSetup *Hud_RaceSetup(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;

    return (RaceSetup *)(base + offset);
}

// Rotates (x, y) and stores it as a screen offset, 1/16 scale.
void Hud_RotatePoint(short *out, int angle, int x, int y)
{
    short *table = g_SinTable;
    int cosIndex;
    int c;
    int s;

    angle &= 0xfff;
    cosIndex = angle + 0x400;
    c = table[cosIndex];
    s = table[angle];

    out[0] = ((x * c >> 12) - (y * s >> 12) + 4) >> 4;
    out[1] = ((y * c >> 12) + (x * s >> 12) - 8) >> 4;
}

// Builds the rev counter of a car and returns the index of its digit TIM.
int Tacho_Init(RaceVehicle *car, int index)
{
    VehicleHud *hud = VEHICLE_HUD(car);
    int thousands;
    int i = 0;
    int j;
    int k;
    int angle;
    int outer = 0x204;

    hud->shiftRpm = car->phys.launchRpm;
    hud->redline = car->phys.engine.redline;
    VEHICLE_HUD(car)->index = index;
    thousands = (hud->redline + 999) / 1000;
    {
        int segments = thousands * 4;

        hud->range = segments * 250;
        hud->segments = segments;
        hud->redSegment = (short)hud->shiftRpm / 250;
        while (D_8002F868[i] < thousands) {
            if (D_8002F868[i] < 0) {
                break;
            }
            i++;
        }
        for (k = j = 0; j <= segments; j++) {
            int *point = hud->dial[j];

            angle = (short)(k / segments + 0x4fa);
            Hud_RotatePoint((short *)point, angle, 0x281, 0);
            Hud_RotatePoint((short *)(point + 1), angle, outer, 0);
            k += 0x96c;
        }
    }
    return i;
}

// Draws a needle at an angle, around the screen position xy.
void Hud_DrawNeedle(uint *ot, short angle, int xy, HudNeedle *needle)
{
    uint *prim = (uint *)Prim_AddPolyG4(ot);

    prim[0] = prim[4] = needle->rgb0 | 0x38000000;
    prim[2] = prim[6] = needle->rgb1;
    Hud_RotatePoint((short *)(prim + 1), angle, needle->x0, -needle->y0);
    Hud_RotatePoint((short *)(prim + 3), angle, needle->x0, needle->y0);
    Hud_RotatePoint((short *)(prim + 5), angle, needle->x1, -needle->y1);
    Hud_RotatePoint((short *)(prim + 7), angle, needle->x1, needle->y1);
    prim[1] += xy;
    prim[3] += xy;
    prim[5] += xy;
    prim[7] += xy;
}

// The rev counter's face and its coloured segments, the red zone from redSegment.
void Tacho_DrawDial(uint *ot, RaceVehicle *car, int x, int y)
{
    int xy;
    PrimSprite *sprite;
    unsigned int rgb;
    VehicleHud *hud = VEHICLE_HUD(car);
    int i;
    int segments;
    int red;

    xy = (y << 16) | x;
    sprite = Prim_AddSprite(ot, 0x2606060);
    SPRITE_SET_INDEX(sprite, VEHICLE_HUD(car)->index, x, y);
    Prim_AddDrawMode(ot, g_Hud_Sprites[VEHICLE_HUD(car)->index].tpage | 0x20);
    rgb = 0x2505050;
    segments = hud->segments;
    red = hud->redSegment;
    for (i = 0; i < segments; i++) {
        uint *quad;

        if (i >= red) {
            rgb = 0x20000f4;
        }
        quad = Prim_AddPolyF4(ot, rgb);
        quad[1] = hud->dial[i][0] + xy;
        quad[2] = hud->dial[i][1] + xy;
        quad[3] = hud->dial[i + 1][0] + xy;
        quad[4] = hud->dial[i + 1][1] + xy;
    }
    Prim_AddDrawMode(ot, 0);
}

// The rev counter's needle, the gear and the speed.
void Tacho_DrawNeedle(uint *ot, RaceVehicle *car, int x, int y)
{
    int angle = (short)(car->phys.rpm * 0x96c / VEHICLE_HUD(car)->range + 0x4fa);
    int xy = (y << 16) | x;
    unsigned int rgb;
    PrimSprite *sprite;
    HudSprite *digit;
    RECT *tile;
    int speed;
    int divisor = 100;
    int shown = 0;
    int i = 0;

    Hud_DrawNeedle(ot, angle, xy, &D_8002F880);
    Hud_DrawNeedle(ot, angle, xy, &D_8002F898);
    rgb = 0x284078;
    if (car->phys.clutchIn != 1) {
        rgb = 0x1c2644;
    }
    sprite = Prim_AddSprite(ot, rgb);
    digit = &g_Hud_Sprites[18];
    SPRITE_SET(sprite, &digit[D_8002F874[car->phys.gear]], x + 0x17, y + 2);
    Prim_AddDrawMode(ot, digit->tpage | 0x20);
    tile = Prim_AddTile(ot, 0x21e1e1e);
    tile->x = x + 0x10;
    tile->y = y - 6;
    tile->w = 0xe;
    tile->h = 0x10;
    Prim_AddDrawMode(ot, 0x40);
    sprite = Prim_AddSprite(ot, 0x284078);
    SPRITE_SET(sprite, &g_Hud_Sprites[17], x + 0x12, y + 0x28);
    Prim_AddDrawMode(ot, g_Hud_Sprites[17].tpage | 0x20);
    speed = (unsigned short)(car->phys.speedKmh / 100);
    x += 2;
    y += 0x18;
    do {
        int d = speed / divisor;

        speed -= d * divisor;
        if (d != 0 || shown != 0) {
            sprite = Prim_AddSprite(ot, 0x284078);
            SPRITE_SET(sprite, &g_Hud_Sprites[d + 6], x, y);
            shown = 1;
        }
        divisor /= 10;
        x += 0xc;
        if (divisor == 1) {
            shown = 1;
        }
        i++;
    } while (i < 3);
    Prim_AddDrawMode(ot, g_Hud_Sprites[6].tpage | 0x20);
}

// The boost gauge, for a car with a turbo; the scale is wider for a big one.
void Hud_DrawBoostGauge(uint *ot, RaceVehicle *car, int x, int y)
{
    int range = 0x2000;
    int unused = 0;
    int turbo = car->phys.engine.maxBoost;

    if (turbo != 0) {
        int boost = car->phys.boost - 0x1000;
        int delta;
        short angle;
        int xy;
        PrimSprite *sprite;

        if (turbo > 0x3000) {
            range = 0x4000;
        }
        if (boost < 0) {
            delta = boost * 0x555 / 0x1000;
        } else {
            delta = boost * 0x555 / range;
        }
        angle = delta + 0x6aa;
        xy = (y << 16) | x;
        Hud_DrawNeedle(ot, angle, xy, &D_8002F8B0);
        Hud_DrawNeedle(ot, angle, xy, &D_8002F8C8);
        sprite = Prim_AddSprite(ot, 0x2606060);
        SPRITE_SET(sprite, &g_Hud_Sprites[36], x, y);
        Prim_AddDrawMode(ot, g_Hud_Sprites[36].tpage | 0x20);
        sprite = Prim_AddSprite(ot, 0x2282828);
        SPRITE_SET(sprite, &g_Hud_Sprites[37], x, y);
        Prim_AddDrawMode(ot, g_Hud_Sprites[37].tpage);
    }
}

// The panel at the top left: place, times of the last laps, lap count.
void Hud_DrawLapPanel(uint *ot, RaceVehicle *car, int x, int y)
{
    TextCtx text;
    TextCtx *ctx;
    char buf[0x40];
    char number[8];
    RaceRecord *record;
    int finished;
    int laps;
    int type;
    int lines = 3;
    int textY;
    int i;

    record = (RaceRecord *)((char *)&g_GameState + 0xc5a8);
    finished = car->phys.aiStarted != 0;
    laps = RACE_SETUP->lapCount;
    type = RACE_SETUP->raceType;
    if (car->controlType == 3) {
        record = (RaceRecord *)((char *)&g_GameState + 0x10ac0);
    }
    switch (type) {
    case 0:
    case 2:
    case 4:
    case 11: {
        int place = (signed char)car->phys.racePos - 1;
        PrimSprite *sprite;

        if (place < 0) {
            place = 0;
        }
        if (place >= 6) {
            place = 5;
        }
        sprite = Prim_AddSprite(ot, 0x284078);
        SPRITE_SET(sprite, &g_Hud_Sprites[place + 30], x + 0x44, y + 0xe);
        Prim_AddDrawMode(ot, g_Hud_Sprites[place + 16].tpage);
        break;
    }
    }
    Text_Init(&text, -1);
    ctx = &text;
    ctx->ot = ot;
    Text_SetFont(ctx, &D_80093124);
    ctx->rgb = 0x284078;
    textY = y + 0x4c;
    switch (type) {
    case 0:
    case 2:
    case 4:
    case 11: {
        int width = Text_DrawString(&text, sR_TotalTime, x + 4, y + 0x26, 0);
        int width2 = Text_DrawString(&text, sR_TotalTime + 0xd, x + 4, y + 0x40, 0);
        unsigned int time;

        if (width < width2) {
            width = width2;
        }
        D_8002F8E0.rect.x = x + 2;
        D_8002F8E0.rect.w = width + 5;
        D_8002F8E0.rect.y = y + 0x21;
        Menu_AddGouraudQuadOpaque(ot, &D_8002F8E0);
        D_8002F8E0.rect.y = y + 0x3b;
        Menu_AddGouraudQuadOpaque(ot, &D_8002F8E0);
        time = (unsigned int)g_RaceClock / 3;
        if (finished) {
            time = record->unkf8;
        } else {
            time += (g_Hud_ClockFrame & 0xf) * 1000 / 900;
        }
        Time_FormatMs(buf, time);
        Text_DrawDigitsRight(&text, (signed char *)buf, x + 0x46, y + 0x31, 7, 5, 0, 2);
        break;
    }
    case 1:
    case 6:
    case 10: {
        int width;

        y -= 0x12;
        textY = y + 0x32;
        lines = 6;
        width = Text_DrawString(&text, sR_LapTime, x + 4, y + 0x26, 0);
        D_8002F8E0.rect.x = x + 2;
        D_8002F8E0.rect.w = width + 5;
        D_8002F8E0.rect.y = y + 0x21;
        Menu_AddGouraudQuadOpaque(ot, &D_8002F8E0);
        break;
    }
    case 3: {
        int width;

        textY = y + 0x26;
        if (D_801C98A2 < 4 && D_801C98A2 >= 2) {
            textY = y + 0x14;
        }
        width = Text_DrawString(&text, sR_LapTime, x + 4, textY, 0);
        D_8002F8E0.rect.x = x + 2;
        D_8002F8E0.rect.w = width + 5;
        D_8002F8E0.rect.y = textY - 5;
        Menu_AddGouraudQuadOpaque(ot, &D_8002F8E0);
        textY += 0xc;
        break;
    }
    }
    if (g_RaceWorld.isReplay == 0) {
        int count = record->lapsKept;

        if (finished) {
            i = count - 1 - (lines - 1);
        } else {
            i = count - (lines - 1);
        }
        for (; i < count; i++) {
            if (i >= 0) {
                Time_FormatMs(buf, record->laps[i].time);
                Text_DrawDigitsRight(&text, (signed char *)buf, x + 0x46, textY, 7, 5, 0, 2);
                textY += 9;
            }
        }
    } else {
        int lap = Car_GetDisplayLap(&car->phys);
        int width;

        Time_FormatMs(buf, Records_GetLapTime(record, lap - 1));
        width = ((int (*)(TextCtx *, signed char *, int, int, int, int, int, int))Text_DrawDigitsRight)(
            &text, (signed char *)buf, x + 0x46, textY, 7, 5, 0, 2);
        Text_DrawDigitsRight(&text, D_8002F278, x + 0x46 - width, textY, 7, 5, 0, 2);
        Text_DrawString(&text, D_8002F27C, x + 0x46, textY, 0);
        textY += 9;
    }
    if (!finished) {
        unsigned int time = (unsigned int)g_RaceClock / 3 - car->phys.raceTime;

        time += (g_Hud_ClockFrame & 0xf) * 1000 / 900;
        Time_FormatMs(buf, time);
        Text_DrawDigitsRight(&text, (signed char *)buf, x + 0x46, textY, 7, 5, 0, 2);
    }
    switch (type) {
    case 0:
    case 1:
    case 2:
    case 4:
    case 6:
    case 10:
    case 11: {
        int lap;
        signed char *p = (signed char *)buf;
        signed char *q;
        int d;

        Text_DrawString(&text, sR_Lap, x + 4, y + 8, 0);
        Text_SetFont(&text, &D_80093130);
        lap = car->phys.lap;
        if (lap <= 0) {
            lap = 1;
        }
        if (laps < lap) {
            lap = laps;
        }
        x += 0xc;
        switch (type) {
        case 1:
        case 6:
        case 10: {
            int length;

            if (lap >= 1000) {
                lap = 999;
            }
            sprintf(number, D_8002F280, lap);
            length = strlen(number);
            for (i = 0; i < length; i++) {
                *p++ = number[i] - '0';
            }
            x += (4 - length) * 8;
            break;
        }
        default: {
            int tens = lap / 10;

            if (tens != 0) {
                *p++ = tens;
            } else {
                x += 8;
            }
            *p++ = lap - tens * 10;
            *p++ = 0xb;
            tens = laps / 10;
            if (tens != 0) {
                *p++ = tens;
            }
            *p++ = laps - tens * 10;
            break;
        }
        }
        *p = -1;
        q = (signed char *)buf;
        for (d = *q++; d >= 0; d = *q++) {
            PrimSprite *sprite = Prim_AddSprite(ot, 0x284078);

            SPRITE_SET(sprite, &g_Hud_Sprites[d + 18], x, y + 0x13);
            x += 8;
        }
        Prim_AddDrawMode(ot, g_Hud_Sprites[18].tpage);
        break;
    }
    }
}

// A time and, when it's set, the speed under it.
void Hud_DrawTimeSpeed(TextCtx *ctx, TimeRecord *record, int x, int y)
{
    char buf[0x10];

    Time_FormatMs(buf, record->time);
    Text_DrawDigitsRight(ctx, (signed char *)buf, x + 3, y, 7, 5, 0, 2);
    Speed_Format(buf, (unsigned short)record->unk10);
    strcat(buf, sR_Mph);
    if (~record->time != 0) {
        Text_DrawStringMonoDigitsRight(ctx, (signed char *)buf, x, y + 9, 0, 3, -1);
    }
}

// A target time with its coloured marker, 0x36 pixels left of it.
void Hud_DrawTargetTime(TextCtx *ctx, unsigned int time, int x, int y, uint *ot, unsigned int rgb)
{
    char buf[0x10];
    RECT *tile;

    Time_FormatMs(buf, time);
    Text_DrawDigitsRight(ctx, (signed char *)buf, x + 3, y, 7, 5, 0, 2);
    x -= 0x36;
    tile = Prim_AddTile(ot, rgb);
    tile->x = x;
    tile->y = y - 6;
    tile->w = 4;
    tile->h = 4;
    tile = Prim_AddTile(ot, 0);
    tile->x = x - 1;
    tile->y = y - 7;
    tile->w = 6;
    tile->h = 6;
}

// The record of a machine test: a speed ("----" when empty) or a time.
void Hud_DrawMachineTestRecord(TextCtx *ctx, TimeRecord *record, int x, int y, int isSpeed)
{
    char buf[0x40];

    if (isSpeed != 0) {
        Speed_Format(buf, record->splits[0]);
        strcat(buf, sR_Mph);
        if (~record->splits[0] == 0) {
            __builtin_memcpy(buf, D_8002F2E8, 5);
        }
        Text_DrawStringMonoDigitsRight(ctx, (signed char *)buf, x, y, 0, 3, -1);
    } else {
        Time_FormatMs(buf, record->splits[0]);
        Text_DrawDigitsRight(ctx, (signed char *)buf, x + 3, y, 7, 5, 0, 2);
    }
}

// The records of the current race at the top right.
void Hud_DrawRecordPanel(uint *ot, RaceVehicle *car, int x, int y)
{
    TextCtx text;
    TextCtx *ctx;
    int isSpeed = 0;
    int showCourse = 1;
    int showBest = 1;
    int showTest = 0;
    TimeRecord *testRecord = 0;
    RaceSetup *setup = RACE_SETUP;
    int type = setup->raceType;
    int width;

    Text_Init(&text, -1);
    ctx = &text;
    ctx->ot = ot;
    Text_SetFont(ctx, &D_80093124);
    ctx->rgb = 0x284078;
    switch (type) {
    case 0:
        return;
    case 9:
        isSpeed = 1;
        showBest = 0;
        showCourse = 0;
        showTest = 1;
        testRecord = (TimeRecord *)((char *)&g_GameState + 0x3bd4);
        break;
    case 7:
        showBest = 0;
        showCourse = 0;
        showTest = 1;
        testRecord = (TimeRecord *)((char *)&g_GameState + 0x3a8c);
        break;
    case 8:
        showBest = 0;
        showCourse = 0;
        showTest = 1;
        testRecord = (TimeRecord *)((char *)&g_GameState + 0x3b30);
        break;
    case 3: {
        char *times;

        showCourse = 0;
        showBest = 0;
        Hud_DrawTimeSpeed(&text, (TimeRecord *)((char *)&g_GameState + setup->licenseId * 1640 + 0x1418 + setup->licenseTest * 164 + 4), x, y + 0x20);
        times = (char *)GameData_FindEvent((GameData *)g_EventDataBuf, setup->eventCode) + 0x44;
        Hud_DrawTargetTime(&text, PackedTime_ToMs(times, 1), x, y + 0x3a, ot, 0x64c0e0);
        Hud_DrawTargetTime(&text, PackedTime_ToMs(times, 2), x, y + 0x43, ot, 0xe0c070);
        Hud_DrawTargetTime(&text, PackedTime_ToMs(times, 3), x, y + 0x4c, ot, 0x1050a0);
        break;
    }
    }
    width = Text_DrawStringRight(&text, sR_Record, x, y + 0x14, 0);
    D_8002F8E0.rect.x = x - width - 2;
    D_8002F8E0.rect.w = width + 5;
    D_8002F8E0.rect.y = y + 0xf;
    Menu_AddGouraudQuadOpaque(ot, &D_8002F8E0);
    if (showBest) {
        width = Text_DrawStringRight(&text, sR_Record + 7, x, y + 0x3a, 0);
        D_8002F8E0.rect.x = x - width - 2;
        D_8002F8E0.rect.w = width + 5;
        D_8002F8E0.rect.y = y + 0x35;
        Menu_AddGouraudQuadOpaque(ot, &D_8002F8E0);
    }
    if (showCourse) {
        Hud_DrawTimeSpeed(&text, (TimeRecord *)g_RaceWorld.courseRecord, x, y + 0x20);
    }
    if (showBest) {
        Hud_DrawTimeSpeed(&text, &g_RaceRecords.best, x, y + 0x46);
    }
    if (showTest) {
        Hud_DrawMachineTestRecord(&text, testRecord, x, y + 0x20, isSpeed);
    }
}

// What a lap shows in the middle of the screen: the gap, its time, and a crash.
void Hud_DrawLapPopup(uint *ot, RaceVehicle *car, int x, int y)
{
    TextCtx text;
    char buf[0x10];
    unsigned int rgb[2];
    TextCtx *ctx;
    VehicleHud *hud = VEHICLE_HUD(car);
    int invalid = (signed char)hud->lapInvalid != 0;
    int timer;
    int fade;

    Text_Init(&text, -1);
    ctx = &text;
    ctx->ot = ot;
    HUD_TEXT_BLEND(ctx, 0x200000);
    timer = hud->refTimeTimer;
    if (timer > 0) {
        fade = 0;
        rgb[0] = 0x606060;
        if ((unsigned int)hud->refTime2 < (unsigned int)hud->refTime) {
            rgb[0] = 0x2e6010;
        }
        if ((unsigned int)hud->refTime < (unsigned int)hud->refTime2) {
            rgb[0] = 0x61060;
        }
        if (invalid) {
            rgb[0] = 0xa1e50;
        }
        if (timer < 0x1f) {
            rgb[0] |= 0x2000000;
            fade = ((0x1e - timer) << 7) / 30;
        }
        if (hud->refTime != -1) {
            Text_SetFont(&text, &D_80093124);
            text.rgb = Rgb_Lerp((unsigned char *)rgb, D_8002F8F0, fade, 0x80);
            Time_FormatDiff(buf, hud->refTime, hud->refTime2);
            Text_DrawDigitsCentered(&text, (signed char *)buf, x, y - 0x10, 7, 5, 0, 2);
        }
    }
    timer = hud->lapTimeTimer;
    if (timer > 0) {
        fade = 0;
        rgb[0] = 0x1e5a78;
        rgb[1] = 0x285a78;
        if (invalid) {
            rgb[0] = 0xa1e50;
        }
        if (timer < 0x1f) {
            fade = ((0x1e - timer) << 7) / 30;
            rgb[1] = 0x2285a78;
            rgb[0] |= 0x2000000;
        }
        if (hud->lapLabel != 0) {
            Text_SetFont(&text, &D_80093124);
            text.rgb = Rgb_Lerp((unsigned char *)&rgb[1], D_8002F8F0, fade, 0x80);
            Text_DrawCentered(&text, (signed char *)hud->lapLabel, x, y - 0x1a, 1);
        }
        Text_SetFont(&text, &D_80093130);
        text.rgb = Rgb_Lerp((unsigned char *)rgb, D_8002F8F0, fade, 0x80);
        Time_FormatMs(buf, hud->lapTimeShown);
        Text_DrawDigitsCentered(&text, (signed char *)buf, x, y, 8, 6, 0, 0);
        if (invalid) {
            Text_DrawCentered(&text, sR_TimeInvalid, x, y + 0x10, 0);
        }
    }
    timer = hud->messageTimer;
    if (timer > 0) {
        signed char *message = sR_Crash;

        if (hud->message == 2) {
            message += 0x15;
        }
        rgb[0] = 0xa1e50;
        fade = 0;
        if (timer < 0x1f) {
            fade = ((0x1e - timer) << 7) / 30;
            rgb[0] = 0x20a1e50;
        }
        Text_SetFont(&text, &D_80093130);
        text.rgb = Rgb_Lerp((unsigned char *)rgb, D_8002F8F0, fade, 0x80);
        Text_DrawCentered(&text, message, x, y + 0x20, 0);
    }
}

// Bars of the throttle, the brake and the clutch.
void Hud_DrawPedals(uint *ot, RaceVehicle *car, int x, int y)
{
    int width = 6;
    int values[3];
    int i;
    int *value = values;
    unsigned int *rgb = D_8002F8F8;
    RECT *tile;
    int height;
    PrimSprite *sprite;

    values[0] = car->phys.throttle;
    values[1] = car->phys.brake;
    values[2] = car->phys.unk614;
    i = 0;
    do {
        tile = Prim_AddTile(ot, *rgb);
        height = *value * 21 / 0x1000;
        value++;
        rgb++;
        tile->y = y - (height - 6);
        tile->w = width;
        tile->h = height;
        tile->x = (signed char)D_8002F8F4[i] + x;
        i++;
    } while (i < 3);
    sprite = Prim_AddSprite(ot, 0x808080);
    SPRITE_SET(sprite, &g_Hud_Sprites[40], x, y);
    Prim_AddDrawMode(ot, g_Hud_Sprites[40].tpage | 0x20);
}

// The two front wheels at their steering angle, and the marks of the slide.
void Hud_DrawSteering(uint *ot, RaceVehicle *car, int x, int y)
{
    int xy = ((y - 8) << 16) | (x - 7);
    int angles[2];
    int i;
    int value;
    short angle;
    uint *quad;
    int slide;
    unsigned int rgb;
    PrimSprite *sprite;
    HudSprite *mark;

    angles[0] = (short)car->phys.wheels[0].heading;
    angles[1] = (short)car->phys.wheels[1].heading;
    i = 0;
    do {
        value = -angles[i];
        i++;
        quad = Prim_AddPolyF4(ot, 0x3c6428);
        angle = value;
        Hud_RotatePoint((short *)(quad + 1), angle, -0x1c, -0x4c);
        Hud_RotatePoint((short *)(quad + 2), angle, 0x1c, -0x4c);
        Hud_RotatePoint((short *)(quad + 3), angle, -0x1c, 0x4c);
        Hud_RotatePoint((short *)(quad + 4), angle, 0x1c, 0x4c);
        quad[1] += xy;
        quad[2] += xy;
        quad[3] += xy;
        quad[4] += xy;
        quad = Prim_AddPolyF4(ot, 0);
        Hud_RotatePoint((short *)(quad + 1), angle, -0x33, -0x63);
        Hud_RotatePoint((short *)(quad + 2), angle, 0x33, -0x63);
        Hud_RotatePoint((short *)(quad + 3), angle, -0x33, 0x63);
        Hud_RotatePoint((short *)(quad + 4), angle, 0x33, 0x63);
        quad[1] += xy;
        quad[2] += xy;
        quad[3] += xy;
        quad[4] += xy;
        xy = ((y - 8) << 16) | (x + 7);
    } while (i < 2);
    mark = &g_Hud_Sprites[45];
    slide = (signed char)car->phys.unk768;
    rgb = 0x303030;
    if (slide > 0) {
        rgb = 0x4080;
    }
    sprite = Prim_AddSprite(ot, rgb);
    SPRITE_SET(sprite, mark, x - 6, y + 6);
    Prim_AddDrawMode(ot, mark->tpage);
    mark++;
    rgb = 0x303030;
    if (slide < 0) {
        rgb = 0x4080;
    }
    sprite = Prim_AddSprite(ot, rgb);
    SPRITE_SET(sprite, mark, x + 0xa, y + 6);
    Prim_AddDrawMode(ot, mark->tpage);
}

// Wear and temperature of the four tyres, with tyre wear or that collision rule.
void Hud_DrawTyres(uint *ot, RaceVehicle *car, int x, int y)
{
    int wear[4];
    int r[4];
    int g[4];
    int b[4];
    int i;
    int axle;
    int rowY;

    if (g_TyreWearEnabled == 0 && Race_GetCollisionRule() != 2) {
        return;
    }
    wear[0] = car->phys.wheels[0].dirt;
    wear[1] = car->phys.wheels[1].dirt;
    wear[2] = car->phys.wheels[2].dirt;
    wear[3] = car->phys.wheels[3].dirt;
    for (i = 0; i < 4; i++) {
        Color_HeatRamp(car->phys.wheels[i].wearLevel, &r[i], &g[i], &b[i]);
    }
    rowY = y - 8;
    for (axle = 0; axle < 2; axle++) {
        HudSprite *sprite = &g_Hud_Sprites[42];
        PrimSprite *prim;
        int w = axle * 2;

        prim = Prim_AddSprite(ot, Rgb_Lerp(D_8002F904, D_8002F908, wear[w], 0x100));
        SPRITE_SET(prim, sprite, x, rowY);
        Prim_AddDrawMode(ot, sprite->tpage);
        sprite--;
        prim = Prim_AddSprite(ot, r[w] | (g[w] << 8) | (b[w] << 16));
        SPRITE_SET(prim, sprite, x, rowY);
        Prim_AddDrawMode(ot, sprite->tpage);
        sprite += 2;
        prim = Prim_AddSprite(ot, Rgb_Lerp(D_8002F904, D_8002F908, wear[w + 1], 0x100));
        SPRITE_SET(prim, sprite, x, rowY);
        Prim_AddDrawMode(ot, sprite->tpage);
        sprite++;
        prim = Prim_AddSprite(ot, r[w + 1] | (g[w + 1] << 8) | (b[w + 1] << 16));
        SPRITE_SET(prim, sprite, x, rowY);
        Prim_AddDrawMode(ot, sprite->tpage);
        rowY = y + 8;
    }
}

// The race message of a car, centred.
void Hud_DrawMessage(uint *ot, RaceVehicle *car, int x, int y)
{
    TextCtx text;
    TextCtx *ctx;
    signed char *message = D_8002F318;

    Text_Init(&text, -1);
    ctx = &text;
    ctx->ot = ot;
    Text_SetFont(ctx, &D_80093130);
    {
        unsigned int mask = 0xff9fffff;

        ctx->rgb = 0x20c1879;
        *(unsigned int *)&ctx->clut &= mask;
    }
    switch (car->phys.aiMessage) {
    case 1:
        message = sR_WrongWay;
        break;
    case 7:
        message = sR_TooFastToStop;
        break;
    case 8:
        message = sR_SkippedPitStop;
        break;
    case 9:
        message = sR_NoNeedToPitStop;
        break;
    case 10:
        message = sR_LimitedSpeed;
        break;
    case 11:
        message = sR_PitWorkInProgress;
        break;
    case 4:
        message = sR_FullThrottle;
        break;
    case 5:
        message = sR_Ready;
        break;
    case 6:
        message = sR_Go;
        break;
    case 2:
        message = sR_WARNINGWrongWay;
        break;
    case 3:
        message = sR_WARNINGDonTGoBack;
        break;
    case 12:
        message = sR_Exit;
        break;
    }
    Text_DrawString(&text, message, x - (Text_MeasureString(&text, message, 0) >> 1), y, 0);
}

// Loads the HUD's textures on the first race of a view, and builds the cars' dials.
void Hud_Init(RaceViewLoop *view)
{
    RECT rect;
    int *file = g_HudTexFile;
    int i;
    RaceSetup *setup = &g_RaceSetup;
    RaceWorld *world = &g_RaceWorld;
    HudSprite *sprite = g_Hud_Sprites;
    RaceMem *race;

    g_Hud_ClockFrame = 0;
    if (view->unk2e7 == 0) {
        short *tim;

        VolId_ReadFileAsync(0x26, file);
        tim = (short *)((char *)file + file[1] + 8);
        rect.x = 0x200;
        rect.y = 0;
        rect.w = tim[4];
        rect.h = tim[5];
        Gpu_LoadImageBlock((int *)tim, &rect);
    }
    for (i = 0; i < setup->carCount; i++) {
        RaceVehicle *car = &world->vehicles[i];
        int tim;

        car->messageTimer = 0;
        car->refTimeTimer = 0;
        car->lapTimeTimer = 0;
        tim = Tacho_Init(car, i);
        if (view->unk2e7 == 0) {
            int *header = (int *)((char *)file + file[tim + 2]);
            short *image = (short *)(header + 2);

            if ((header[1] >> 3) & 1) {
                image = (short *)((char *)image + header[2]);
            }
            rect.x = (sprite->u >> 2) + 0x200;
            rect.y = sprite->v;
            rect.w = image[4];
            rect.h = image[5];
            Gpu_LoadImageBlock((int *)image, &rect);
        }
        sprite++;
    }
    view->unk2e7 = 1;
    race = &g_Race_IsDemo;
    Countdown_Init(&race->world.countdown, race->world.introTimer);
    Result_Init(race->world.resultAnim);
}

// Counts down the cars' popups and steps the clock's frames, the countdown and the
// result banner.
int Hud_Update(RaceViewLoop *view, PadState *pad)
{
    int i = 0;
    RaceSetup *setup = &g_RaceSetup;
    RaceMem *race = &g_Race_IsDemo;
    RaceWorld *world = &race->world;
    RaceWorld *cars = world;

    for (; i < setup->carCount; i++) {
        VehicleHud *hud = VEHICLE_HUD((RaceVehicle *)(i * sizeof(RaceVehicle) + 0x184 + (char *)cars));
        VehicleHud *last = hud;

        if (hud->lapTimeTimer > 0) {
            hud->lapTimeTimer--;
        }
        if (hud->refTimeTimer > 0) {
            hud->refTimeTimer--;
        }
        if (last->messageTimer > 0) {
            last->messageTimer--;
        }
    }
    if (g_RaceClock != 0) {
        g_Hud_ClockFrame++;
    }
    Countdown_Update(&world->countdown, view);
    return Result_Update(world->resultAnim, pad);
}

// The HUD of a one-player race.
void Hud_DrawFull(RaceViewLoop *view, RaceVehicle *car, RaceCam *cam)
{
    RaceMem *race = &g_Race_IsDemo;
    uint *ot = (uint *)view->layers[0].ot;
    RaceWorld *world = &race->world;

    if (RACE_SETUP->raceType == 0xc) {
        return;
    }
    Countdown_Draw(&race->world.countdown, ot);
    Result_Draw(race->world.resultAnim, ot);
    Hud_DrawMessage(ot, car, 0xa0, 0xb6);
    Hud_DrawLapPopup(ot, car, 0xa0, 0x60);
    if (world->isReplay != 0) {
        int mode;

        switch (mode = cam->replayOpt) {
        case 0:
            goto lapPanel;
        case 1:
            break;
        case 2:
            Hud_DrawPedals(ot, car, 0xc0, 0xd0);
            Hud_DrawSteering(ot, car, 0x96, 0xcc);
            break;
        default:
            return;
        }
    }
    Hud_DrawRecordPanel(ot, car, 0x134, 0x10);
    Tacho_DrawNeedle(ot, car, 0x114, 0xb4);
    Tacho_DrawDial(ot, car, 0x114, 0xb4);
    Hud_DrawBoostGauge(ot, car, 0xe0, 0xcc);
    Hud_DrawTyres(ot, car, 0x128, 0x7c);
    {
        RaceSetup *setup = RACE_SETUP;

        if (setup->raceType == 3 && setup->licenseId != 0) {
            goto lapPanel;
        }
    }
    Hud_DrawCourseMap(world, ot, 0x10, 0x90, 0);
lapPanel:
    Hud_DrawLapPanel(ot, car, 0xc, 0x10);
}

// "REPLAY" and the name of the car.
void Hud_DrawReplayLabel(RaceViewLoop *view, int index)
{
    if (g_RaceWorld.isReplay != 0) {
        TextCtx text;
        TextCtx *ctx;
        RaceCar *raceCar = &RACE_SETUP->cars[index];
        uint *ot = (uint *)view->layers[0].ot;

        Text_Init(&text, -1);
        ctx = &text;
        ctx->ot = ot;
        Text_SetFont(ctx, &D_80093124);
        {
            unsigned int mask = 0xff9fffff;

            ctx->rgb = 0x606060;
            *(unsigned int *)&ctx->clut = (*(unsigned int *)&ctx->clut & mask) | 0x200000;
        }
        Text_DrawString(ctx, sR_Replay, 0x10, 0xd2, 0);
        Text_DrawString(ctx, (signed char *)raceCar + 0x90, 0x10, 0xde, 0);
    }
}

// The HUD of one car of a two-player race; player 2's is in the lower half.
void Hud_DrawSplit(RaceViewLoop *view, RaceVehicle *car, RaceCam *cam)
{
    RaceMem *race = &g_Race_IsDemo;
    int index = car->index;
    uint *ot = (uint *)view->layers[0].ot;
    RaceWorld *world = &race->world;
    RECT *tile;
    int y;

    if (index == 0) {
        Countdown_Draw(&race->world.countdown, ot);
        Result_Draw(race->world.resultAnim, ot);
    }
    tile = Prim_AddTile(ot, 0xc6c6c6);
    tile->y = 0x77;
    tile->w = 0x140;
    tile->x = 0;
    tile->h = 2;
    Hud_DrawMessage(ot, car, 0xa0, index == 0 ? 0x50 : 0xc0);
    Hud_DrawLapPopup(ot, car, 0xa0, index == 0 ? 0x30 : 0x9c);
    if (world->isReplay != 0) {
        switch (cam->replayOpt) {
        case 0:
            goto lapPanel;
        case 1:
            break;
        case 2:
            Hud_DrawPedals(ot, car, 0xc0, index == 0 ? 0x60 : 0xd0);
            Hud_DrawSteering(ot, car, 0x96, index == 0 ? 0x62 : 0xd2);
            break;
        default:
            return;
        }
    }
    Hud_DrawRecordPanel(ot, car, 0x134, index == 0 ? 0x10 : 0x7c);
    y = index == 0 ? 0x44 : 0xb4;
    Tacho_DrawNeedle(ot, car, 0x114, y);
    Tacho_DrawDial(ot, car, 0x114, y);
    Hud_DrawBoostGauge(ot, car, 0xe0, index == 0 ? 0x5c : 0xcc);
    Hud_DrawTyres(ot, car, 0x128, index == 0 ? 0x1c : 0x8c);
lapPanel:
    Hud_DrawLapPanel(ot, car, 0xc, index == 0 ? 0x10 : 0x7c);
}

// Loads two TIMs of pack (offsets +0x1c, +0x28) to fixed CLUT and image rects
// (Tim_LoadClutAndImage).
void Hud_LoadCluts(void *view, int pack)
{
    Tim_LoadClutAndImage((void *)(pack + *(int *)(pack + 0x1c)), &g_Hud_TimClutRect0, &g_Hud_TimImageRect0);
    Tim_LoadClutAndImage((void *)(pack + *(int *)(pack + 0x28)), &g_Hud_TimClutRect1, &g_Hud_TimImageRect1);
}

// Handwritten: the smoke particles as textured quads, turned by their seed and sized by
// the GTE's depth cue. The original keeps the packet writes in a single pass.
void Particles_Draw(ParticlePool *pool, RaceCam *cam)
{
    ViewScratch *scratch = (ViewScratch *)0x1f800000;
    int *src = (int *)((char *)cam + 8);
    int *dst = (int *)scratch;
    short *xy = (short *)&scratch->otShift + 1;
    int i;
    int negX;
    int negY;
    int negZ;
    char *packet;
    char *end;
    int index;

    for (i = 0; i < 0x68 / 4; i++) {
        dst[i] = src[i];
    }
    Gte_LoadScaled(scratch, 0, 0, 0);
    gte_SetCtrlReg(GTE_TRX, 0);
    gte_SetCtrlReg(GTE_TRY, 0);
    gte_SetCtrlReg(GTE_TRZ, 0);
    gte_SetCtrlReg(GTE_DQB, 0);
    packet = g_PacketPtr;
    end = g_PacketEnd;
    negX = scratch->negPos[0];
    negY = scratch->negPos[1];
    negZ = scratch->negPos[2];
    for (index = pool->activeHead; index >= 0; index = pool->particles[index].next) {
        Particle *p = &pool->particles[index];
        int vx;
        int vy;
        int vz;
        int a;
        int s;
        int c;
        int brightness;
        int radius;
        int sx;
        int sy;
        int z;
        unsigned int *ot;
        unsigned int *prim;

        if (p->size == 0) {
            continue;
        }
        vx = (p->pos[0] + negX) >> 10;
        vy = (p->pos[1] + negY) >> 10;
        vz = (p->pos[2] + negZ) >> 10;
        if ((abs(vx) | abs(vy) | abs(vz)) > 0x7fff) {
            continue;
        }
        xy[0] = vx;
        xy[1] = vy;
        gte_LoadDataReg(GTE_VXY0, xy);
        gte_SetDataReg(GTE_VZ0, vz);
        gte_SetCtrlReg(GTE_DQA, p->size);
        gte_rtps();
        a = p->seed & 0xfff;
        s = g_SinTable[a];
        c = g_SinTable[a + 0x400];
        brightness = p->life >> (p->kind != 0 ? 3 : 5);
        ot = scratch->ot;
        i = gte_GetCtrlReg(GTE_FLAG);
        gte_StoreDataReg(GTE_SXY2, xy);
        if (i < 0) {
            continue;
        }
        radius = gte_GetDataReg(GTE_MAC0) >> 16;
        z = gte_GetDataReg(GTE_SZ3) >> 3;
        if (radius >= 0x100) {
            continue;
        }
        s = radius * s >> 12;
        c = radius * c >> 12;
        sx = xy[0];
        sy = xy[1];
        if (z >= 0x1000) {
            z = 0xfff;
        }
        ot += z;
        prim = (unsigned int *)packet;
        prim[0] = 0x9000000;
        packet[7] = 0x2e;
        packet[4] = packet[5] = packet[6] = brightness;
        if (p->kind == 0) {
            prim[3] = 0x7f960080;
            prim[5] = 0x29009f;
            prim[7] = 0x1f80;
            prim[9] = 0x1f9f;
        } else {
            prim[3] = 0x7f9700c0;
            prim[5] = 0x2900ef;
            prim[7] = 0x2fc0;
            prim[9] = 0x2fef;
        }
        ((short *)packet)[4] = sx - c + s;
        ((short *)packet)[5] = sy - s - c;
        ((short *)packet)[8] = sx + c + s;
        ((short *)packet)[9] = sy + s - c;
        ((short *)packet)[12] = sx - c - s;
        ((short *)packet)[13] = sy - s + c;
        ((short *)packet)[16] = sx + c - s;
        ((short *)packet)[17] = sy + s + c;
        prim[0] = (prim[0] & 0xff000000) | (*ot & 0xffffff);
        *ot = (*ot & 0xff000000) | ((unsigned int)prim & 0xffffff);
        packet += 0x28;
        if (packet >= end) {
            break;
        }
    }
    g_PacketPtr = packet;
}
}
