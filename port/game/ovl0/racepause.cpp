// OVL0 80029d58-8002bcf0 (racepause): Pause menu, countdown and result banners.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "common.h"
#include "gamestate.h"

// A rounded box: colour, then x, y, width, height.
struct ResultBox {
    unsigned int rgb;
    short x;
    short y;
    short w;
    short h;
};

// An image of the HUD texture.
struct HudSprite {
    unsigned char u;
    unsigned char v;
    unsigned short clut;
    unsigned short w;
    unsigned short h;
    unsigned short tpage;
    unsigned short pad;
};

// What the race left at g_GameState + 0xc508.
struct RaceOutcome {
    int place;
    int status; /* 1: finished */
    unsigned int time;
};

// A saved license test result.
struct LicenseTestRecord {
    signed char unk0;
    signed char failed;
    unsigned char prize;
    unsigned char pad[0xa4 - 3];
};

// g_RaceSetup, as the game reaches it: g_GameState plus 0xbf7c, added at run time.
static inline RaceSetup *RaceSetup_Get(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;

    return (RaceSetup *)(base + offset);
}

static inline RaceOutcome *Race_Outcome(char *base)
{
    int offset = 0xc508;

    return (RaceOutcome *)(base + offset);
}

static inline ChampRaceList *Race_Champ(char *base)
{
    int offset = 0xc514;

    return (ChampRaceList *)(base + offset);
}

static inline RaceSetup *Race_Setup(char *base)
{
    int offset = 0xbf7c;

    return (RaceSetup *)(base + offset);
}

static inline int Race_Type(void)
{
    return RaceSetup_Get()->raceType;
}

// Centered text in a colour.
static inline void Text_DrawCenteredRgb(TextCtx *ctx, signed char *s, int x, int y, unsigned int rgb)
{
    ctx->rgb = rgb;
    Text_DrawCentered(ctx, s, x, y, 0);
}

extern "C" unsigned char (&g_Race_EndRequest);

static inline void Race_RequestEnd(int mode)
{
    g_Race_EndRequest = mode;
}

extern "C" {
extern signed char (&g_PauseMenu_Cursor);
extern signed char (&g_PauseMenu_Blink);
extern game_pointer<signed char> (&g_PauseMenu_Items)[];
extern unsigned char (&g_Race_EndRequest);
extern unsigned char (&g_Race_IsDemo);
extern RaceSetup (&g_RaceSetup);
extern RaceWorld (&g_RaceWorld);
extern game_pointer<void> (&g_EventDataBuf);
extern Font (&D_80093130);
extern Font (&D_8009313C);
extern unsigned char (&D_8002F5E8)[];
extern unsigned char (&D_8002F5FC)[];
extern unsigned int (&D_8002F600);
extern unsigned char (&D_8002F604)[];
extern unsigned char (&D_8002F608)[];
extern unsigned char (&D_8002F60C)[];
extern unsigned int (&D_8002F610);
extern unsigned int (&D_8002F614);
extern unsigned char (&D_8002F618)[];
extern unsigned char (&D_8002F61C)[];
extern unsigned int (&D_8002F620);
extern unsigned int (&D_8002F624);
extern unsigned int (&D_8002F628);
extern unsigned int (&D_8002F62C);
extern HudSprite (&D_8002F804);
extern HudSprite (&D_8005B18C);
extern HudSprite (&D_8005B198);
extern HudSprite (&D_8005B1A4);
extern HudSprite (&D_8005B1B0);
extern signed char (&sR_START)[];
extern signed char (&sR_Finish)[];
extern signed char (&sR_Results)[];
extern signed char (&sR_Running)[];
extern signed char (&sR_Npts)[];
extern signed char (&sR_BRONZEPRIZE)[];
extern signed char (&sR_SILVERPRIZE)[];
extern signed char (&sR_GOLDPRIZE)[];
extern signed char (&sR_PLAYER1WINS)[];
extern signed char (&sR_YOUWIN)[];
extern signed char (&sR_TotalPoints)[];
extern signed char (&sR_Points)[];
void RaceSfx_PlayStereo(int sfx);
unsigned int PackedTime_ToMs(void *time, int index);
int sprintf(char *buf, const char *format, ...);
char *strcpy(char *dest, const char *src);
void Result_DrawTimeRow(void *ot, int t, int row, int car, int laps, unsigned int time, TextCtx *ctx, int versus);
void Result_DrawPointsRow(void *ot, int t, int row, int car, int points, TextCtx *ctx, unsigned int rgb);

// Cursor 0, blink 0.
void PauseMenu_Reset(void *view)
{
    g_PauseMenu_Cursor = '\0';
    g_PauseMenu_Blink = '\0';
}

// The cursor and the two items: 0 goes on, 1 ends the race. 1 while the menu stays.
int PauseMenu_Update(RaceViewLoop *view, RacePad *pad)
{
    PadState *state = &pad->raw;
    unsigned int pressed;

    if (++g_PauseMenu_Blink >= 0x1f) {
        g_PauseMenu_Blink = 0;
    }
    pressed = state->pressed;
    if ((pressed & 1) != 0) {
        if (--g_PauseMenu_Cursor < 0) {
            g_PauseMenu_Cursor = 0;
        }
        g_PauseMenu_Blink = 0;
    }
    if ((pressed & 2) != 0) {
        if (++g_PauseMenu_Cursor >= 2) {
            g_PauseMenu_Cursor = 1;
        }
        g_PauseMenu_Blink = 0;
    }
    if ((pressed & 0x10a00) != 0) {
        switch (g_PauseMenu_Cursor) {
        case 0:
            g_PauseMenu_Cursor = 0;
            g_PauseMenu_Blink = -1;
            return 0;
        case 1:
            g_PauseMenu_Blink = -1;
            g_PauseMenu_Cursor = 0;
            Race_RequestEnd(2);
            break;
        }
    }
    return 1;
}

// The two items in boxes, the selected one blinking.
void PauseMenu_Draw(RaceViewLoop *view)
{
    ResultBox box;
    TextCtx text;
    unsigned int *ot = *(game_pointer<unsigned int> *)((char *)view + 0x8c);

    if (g_PauseMenu_Blink >= 0) {
        int blink;
        int r;
        int g;
        int b;
        unsigned int selected;
        int i;

        TextCtx *ctx = &text;

        Text_Init(ctx, -1);
        text.ot = ot;
        Text_SetFont(ctx, &D_80093130);
        ctx->rgb = 0x2475b5b;
        *(unsigned int *)&text.clut = (*(unsigned int *)&text.clut & ~0x600000) | 0x200000;
        blink = g_PauseMenu_Blink;
        if (blink >= 0xf) {
            blink = 0xe;
        }
        box.w = 100;
        box.h = 0x14;
        r = 0xf2 - blink * 22 / 14;
        g = 0xf2 - blink * 82 / 14;
        b = 0xf2 - blink * 132 / 14;
        selected = (r | g << 8 | b << 16) | 0x2000000;
        for (i = 0; i < 2; i++) {
            short x = i * 4 + 0x9c;
            short y = i * 0x16 + 0x6d;

            box.x = x;
            box.y = y;
            Text_DrawCentered(&text, g_PauseMenu_Items[i], x, y + 7, 0);
            box.rgb = 0x2244290;
            if (i == g_PauseMenu_Cursor) {
                box.rgb = selected;
            }
            Prim_AddRoundedBox(ot, &box, 0x20);
            box.rgb = 0x2181818;
            Prim_AddRoundedBox(ot, &box, 0x20);
            Prim_AddDrawMode(ot, 0);
        }
    }
}

// cnt = intro + 240.
void Countdown_Init(short *cnt, short introFrames)
{
    *cnt = introFrames + 0xf0;
}

// Counts down: the beeps at 3, 2 and 1, the int one and the music at 0.
void Countdown_Update(short *cnt, void *view)
{
    int now = *cnt;

    if (now > 0) {
        int prev = now;

        now -= RaceSetup_Get()->frameStep;
        if (now < 0) {
            now = 0;
        }
        *cnt = now;
        if (now < 0xf0 && prev >= 0xf0) {
            RaceSfx_PlayStereo(1);
            RaceMusic_PlayRaceTrack(view);
            return;
        }
        if ((now < 300 && prev >= 300) || (now < 0x168 && prev >= 0x168) || (now < 0x1a4 && prev >= 0x1a4)) {
            RaceSfx_PlayStereo(0);
        }
    }
}

// The digits, then START, which fades out, or another text in a replay.
void Countdown_Draw(short *cnt, void *ot)
{
    TextCtx text;
    char buf[0x100];
    TextCtx *ctx = &text;
    signed char *start = sR_START;
    unsigned int rgb;
    int t;

    if (g_RaceWorld.isReplay != 0) {
        start += 0x17eb;
    }
    if (*cnt < 0) {
        return;
    }
    Text_Init(ctx, -1);
    ctx->ot = (uint *)ot;
    *(unsigned int *)&ctx->clut = (*(unsigned int *)&ctx->clut & ~0x600000) | 0x200000;
    Text_SetFont(ctx, &D_8009313C);
    t = *cnt;
    if (t >= 0xf1) {
        int digit = (t - 0xf0) / 60;

        if (digit < 3) {
            buf[0] = digit + '1';
            buf[1] = 0;
            Text_DrawCenteredRgb(ctx, (signed char *)buf, 0xa0, 0x6e, 0x6f6f6f);
        }
        return;
    }
    if (t >= 0x79) {
        rgb = 0xa376e;
    } else {
        int r;
        int g;
        int b;

        t = 0x78 - t;
        if (t >= 0x20) {
            return;
        }
        if (t < 0x10) {
            r = t * 34 / 16 + 0x6e;
            g = t * 89 / 16 + 0x37;
            b = t * 134 / 16 + 10;
            rgb = (r | g << 8 | b << 16) | 0x2000000;
        } else {
            t -= 0x10;
            r = 0x90 - t * 9;
            rgb = (r | r << 8 | r << 16) | 0x2000000;
        }
    }
    Text_DrawCenteredRgb(&text, start, 0xa0, 0x6e, rgb);
}

// anim = -1.
void ResultBanner_Hide(short *anim)
{
    *anim = -1;
}

// anim = 0.
void ResultBanner_Start(short *anim)
{
    *anim = 0;
}

// The banner's timeline, up to 0x100.
void ResultBanner_Step(short *anim)
{
    if (*anim >= 0) {
        if (++*anim >= 0x101) {
            *anim = 0x100;
        }
    }
}

// A sprite that opens up over 0x10 frames, stays, and closes from 0x40 on.
void Hud_DrawZoomSprite(uint *ot, int x, int y, HudSprite *sprite, int t)
{
    int active = 0;
    int skew = 0;
    int height = 0;
    unsigned int rgb = 0;
    int step = t;
    int half = (short)sprite->h / 2;

    if (step < 0) {
        return;
    }
    if ((unsigned int)step < 0x10) {
        height = D_8002F5E8[0x10 - step] * half >> 7;
        skew = D_8002F5E8[step] * half >> 8;
        active = 1;
        rgb = Rgb_Lerp(D_8002F604, D_8002F5FC, step, 0x10);
    } else if (step < 0x40) {
        height = half;
        active = 1;
        rgb = 0x6680;
    } else {
        step = t - 0x40;
        if ((unsigned int)step < 0x10) {
            height = D_8002F5E8[step] * half >> 7;
            skew = -(D_8002F5E8[0x10 - step] * half >> 8);
            active = 1;
            rgb = Rgb_Lerp(D_8002F5FC, D_8002F604, step, 0x10);
        }
    }
    if (active != 0) {
        int width = (short)sprite->w >> 1;
        PrimFT4 *prim = Prim_AddPolyFT4(ot, rgb);
        int left = x - width;
        int topLeft = left + skew;
        int right;
        int top;
        int bottom;

        prim->x2 = left - skew;
        right = x + width;
        prim->x1 = right - skew;
        right += skew;
        top = y - height;
        prim->y1 = top;
        prim->y0 = top;
        bottom = y + height;
        prim->x0 = topLeft;
        prim->x3 = right;
        prim->y3 = bottom;
        prim->y2 = bottom;
        prim->u0 = prim->u2 = sprite->u;
        prim->u1 = prim->u3 = sprite->u + (unsigned char)sprite->w - 1;
        prim->v0 = prim->v1 = sprite->v;
        prim->v2 = prim->v3 = sprite->v + (unsigned char)sprite->h - 1;
        prim->clut = sprite->clut;
        prim->tpage = sprite->tpage | 0x20;
    }
}

// The two sprites of the banner, the second one late.
void ResultBanner_Draw(short *anim, uint *ot, int x, int y)
{
    HudSprite *sprite = &D_8002F804;
    int t;

    Hud_DrawZoomSprite(ot, x, y, sprite, *anim);
    t = *anim - 0x50;
    if (t >= 0x11) {
        t = 0x10;
    }
    Hud_DrawZoomSprite(ot, x, y, sprite - 1, t);
}

// Timeline and banner off.
void Result_Init(short *res)
{
    *res = -1;
    ResultBanner_Hide(res + 1);
}

// Starts the result timeline.
void Result_Start(short *res)
{
    *res = 0;
}

// Plays the timeline of the race's results, at up to 3 frames a frame while a button is
// held. A press gets past the stops. 0 once it's over.
int Result_Update(short *res, PadState *pad)
{
    char *base = (char *)&g_GameState;
    RaceOutcome *outcome = Race_Outcome(base);
    ChampRaceList *champ = Race_Champ(base);
    unsigned int held;
    int steps;

    if (*res < 0) {
        return 1;
    }
    switch (Race_Setup(base)->raceType) {
    case 3:
        if (outcome->status == 1) {
            steps = 3;
            do {
                if (*res == 0x78) {
                    Sfx_PlayMenu(3);
                } else if (*res == 0x92) {
                    steps = 0;
                    if ((pad->pressed & 0xa00) != 0) {
                        Sfx_PlayMenu(1);
                        (*res)++;
                    }
                    continue;
                }
                held = pad->held;
                (*res)++;
                steps--;
                if ((held & 0xa00) == 0) {
                    steps = 0;
                }
            } while (steps > 0);
            if (*res >= 0x94) {
                *res = 0x93;
                return 0;
            }
        } else {
            if (*res == 0x1d) {
                if ((pad->pressed & 0xa00) != 0) {
                    Sfx_PlayMenu(1);
                    (*res)++;
                }
            } else {
                (*res)++;
            }
            if (*res >= 0x1f) {
                *res = 0x1e;
                return 0;
            }
        }
        return 1;
    case 1:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10: {
        int count = 3;

        do {
            held = pad->held;
            (*res)++;
            count--;
            if ((held & 0xa00) == 0) {
                count = 0;
            }
        } while (count > 0);
        if (*res >= 0xc7) {
            *res = 0xc6;
            return 0;
        }
        return 1;
    }
    case 11:
        steps = 3;
        do {
            if (*res == 0xe5) {
                steps = 0;
                if ((pad->pressed & 0xa00) != 0) {
                    Sfx_PlayMenu(1);
                    (*res)++;
                }
            } else {
                held = pad->held;
                (*res)++;
                steps--;
                if ((held & 0xa00) == 0) {
                    steps = 0;
                }
            }
        } while (steps > 0);
        if (*res >= 0xe7) {
            *res = 0xe6;
            return 0;
        }
        return 1;
    case 2: {
        int last;

        steps = 3;
        do {
            switch (*res) {
            case 0x13b:
            case 0x1b9:
            case 0x237:
                steps = 0;
                if ((pad->pressed & 0xa00) != 0) {
                    Sfx_PlayMenu(1);
                    (*res)++;
                }
                goto stepped;
            case 0x9e:
                ResultBanner_Start(res + 1);
                break;
            }
            held = pad->held;
            (*res)++;
            steps--;
            if ((held & 0xa00) == 0) {
                steps = 0;
            }
stepped:
            ResultBanner_Step(res + 1);
        } while (steps > 0);
        last = 0x13b;
        if (champ->raceCount >= 2) {
            last = 0x238;
        }
        if (*res > last) {
            *res = last;
            return 0;
        }
        return 1;
    }
    default:
        steps = 3;
        do {
            if (*res == 0x9e) {
                ResultBanner_Start(res + 1);
            } else if (*res == 0x13b) {
                steps = 0;
                if ((pad->pressed & 0xa00) != 0) {
                    Sfx_PlayMenu(1);
                    (*res)++;
                }
                goto again;
            }
            held = pad->held;
            (*res)++;
            steps--;
            if ((held & 0xa00) == 0) {
                steps = 0;
            }
again:
            ResultBanner_Step(res + 1);
        } while (steps > 0);
        if (*res >= 0x13d) {
            *res = 0x13c;
            return 0;
        }
        return 1;
    }
}

// Whether the results are still up: some race types end with them, the others at 0x96.
int FUN_OVL0__8002aaf0(short *res)
{
    int up = 1;

    switch (Race_Type()) {
    case 1:
    case 3:
    case 6:
    case 10:
        break;
    default:
        if (*res >= 0x96) {
            up = 0;
        }
        break;
    }
    return up;
}

// FINISH: in from a colour, split in three while it goes grey, then gone at 0x96.
void Result_DrawTitle(uint *ot, int t)
{
    TextCtx text;
    TextCtx *ctx;
    signed char *title;
    unsigned int rgb;
    int fade;

    if (t >= 0x96) {
        return;
    }
    title = sR_Finish;
    Text_Init(&text, -1);
    ctx = &text;
    ctx->ot = ot;
    Text_SetFont(ctx, &D_8009313C);
    *(unsigned int *)&ctx->clut = (*(unsigned int *)&ctx->clut & ~0x600000) | 0x200000;
    rgb = 0xa376e;
    fade = t - 0x50;
    if (fade >= 0) {
        if (fade < 0xc) {
            rgb = Rgb_Lerp(D_8002F608, D_8002F60C, fade, 0xc);
        } else {
            int grey;
            int offset;

            fade = t - 0x5c;
            if (fade < 0) {
                fade = 0;
            }
            if (fade >= 0xd) {
                fade = 0xc;
            }
            grey = 0x90 - fade * 12;
            rgb = (grey | grey << 8 | grey << 16) | 0x2000000;
            ctx->rgb = rgb;
            offset = fade * 8 / 12;
            Text_DrawCentered(ctx, title, 0xa0 - offset, 0x82, 0);
            Text_DrawCentered(ctx, title, offset + 0xa0, 0x82, 0);
        }
    }
    Text_DrawCenteredRgb(&text, title, 0xa0, 0x82, rgb);
}

// A car's row of times: place, name, then its time or how far behind it was.
void Result_DrawTimeRow(void *ot, int t, int row, int car, int laps, unsigned int time, TextCtx *ctx, int versus)
{
    char buf[0x40];
    ResultBox box;
    int steps = 8;
    int x;
    int y;
    int top;
    char *race;
    RaceVehicle *vehicle;
    RaceCar *entry;
    unsigned int carTime;

    if (t > steps) {
        t = steps;
    }
    t = steps - t;
    x = row * 4 + 0x12;
    y = row * 24;
    Text_SetFont(ctx, &D_80093130);
    buf[0] = row + '1';
    buf[1] = 0;
    *(unsigned int *)&ctx->clut = (*(unsigned int *)&ctx->clut & ~0x600000) | 0x200000;
    ctx->rgb = 0x2004670;
    Text_DrawString(ctx, (signed char *)buf, x, y + 0x51, 1);
    top = y + 0x4a;
    ctx->rgb = 0x606060;
    race = (char *)&g_Race_IsDemo + car * sizeof(RaceVehicle);
    entry = &g_RaceSetup.cars[car];
    vehicle = (RaceVehicle *)(race + 0x188);
    if (versus == 0 && car == 0) {
        ctx->rgb = 0x70543a;
    }
    Text_DrawString(ctx, (signed char *)entry->name, x + 0xd, top + 7, -1);
    carTime = vehicle->phys.raceTime;
    if (time == carTime) {
        Time_FormatMs(buf, carTime);
    } else {
        unsigned int behind;

        strcpy(buf, (char *)sR_Running);
        if (vehicle->phys.aiStarted != 0) {
            Time_FormatDiff(buf, time, carTime);
        }
        behind = laps - vehicle->phys.lap;
        if (behind >= 2) {
            sprintf(buf, (char *)sR_Running + 0x13, behind - 1);
            if (behind == 2) {
                sprintf(buf, (char *)sR_Running + 0xa);
            }
        }
    }
    y = row * 4;
    Text_DrawDigitsRight(ctx, (signed char *)buf, y + 0x118, top + 7, 7, 6, 1, 0);
    box.w = 0x130;
    box.h = 0x16;
    box.x = y + 0xac;
    box.y = top;
    box.rgb = Rgb_Lerp((unsigned char *)&D_8002F614, D_8002F618, t, steps);
    Prim_AddRoundedBox(ot, &box, 0x20);
    Prim_AddDrawMode(ot, 0);
}

// A car's row of championship points.
void Result_DrawPointsRow(void *ot, int t, int row, int car, int points, TextCtx *ctx, unsigned int rgb)
{
    char buf[0x40];
    ResultBox box;
    RaceSetup *setup = RaceSetup_Get();
    RaceCar *entry;
    int steps = 8;
    int x;
    int y;
    int top;

    if (t > steps) {
        t = steps;
    }
    t = steps - t;
    x = row * 4;
    y = row * 24;
    Text_SetFont(ctx, &D_80093130);
    buf[0] = row + '1';
    buf[1] = 0;
    *(unsigned int *)&ctx->clut = (*(unsigned int *)&ctx->clut & ~0x600000) | 0x200000;
    ctx->rgb = 0x2004670;
    Text_DrawString(ctx, (signed char *)buf, x + 0x12, y + 0x51, 1);
    top = y + 0x4a;
    y += 0x51;
    ctx->rgb = 0x606060;
    entry = &setup->cars[car];
    if (car == 0) {
        ctx->rgb = 0x70543a;
    }
    Text_DrawString(ctx, (signed char *)entry->name, x + 0x1f, y, -1);
    sprintf(buf, (char *)sR_Npts, points);
    Text_DrawStringRight(ctx, (signed char *)buf, x + 0x118, y, -1);
    box.w = 0x130;
    box.h = 0x16;
    box.x = x + 0xac;
    box.y = top;
    box.rgb = Rgb_Lerp((unsigned char *)&rgb, D_8002F618, t, steps);
    Prim_AddRoundedBox(ot, &box, 0x20);
    Prim_AddDrawMode(ot, 0);
}

// The results of a race: license prize, or the rows of the race type, and the title bar.
void Result_Draw(short *res, uint *ot)
{
    TextCtx text;
    char buf[0x40];
    signed char order[8];
    unsigned int color;
    unsigned int win;
    unsigned int winAlt;
    signed char *title = sR_Results;
    char *state = (char *)&g_GameState;
    ChampRaceList *champ = (ChampRaceList *)(state + 0xc514);
    RaceOutcome *outcome = (RaceOutcome *)(state + 0xc508);
    int t;
    int i;

    Text_Init(&text, -1);
    text.ot = ot;
    Text_SetFont(&text, &D_8009313C);
    *(unsigned int *)&text.clut = (*(unsigned int *)&text.clut & ~0x600000) | 0x200000;
    if (*res < 0) {
        return;
    }
    if (g_RaceSetup.raceType == 3) {
        if (outcome->status != 1) {
            text.rgb = 0x6e;
            Text_DrawCentered(&text, title + 0x3eb, 0xa0, 0xa0, 0);
            return;
        }
        Result_DrawTitle(ot, *res);
        t = *res - 0x78;
        if (t > 0) {
            HudSprite *sprite = 0;
            signed char *label = title + 0x3eb;
            char *event = (char *)GameData_FindEvent((GameData *)g_EventDataBuf, g_RaceSetup.eventCode);
            void *times = event + 0x44;
            unsigned int time;

            color = D_8002F62C;
            time = outcome->time;
            if (time < PackedTime_ToMs(times, 4)) {
                LicenseTestRecord *record =
                    (LicenseTestRecord *)(state + g_RaceSetup.licenseId * 0x668 + 0x1418) + g_RaceSetup.licenseTest;

                if (record->failed == 0 && record->prize + 1 == ((unsigned char *)times)[0x30]) {
                    sprite = &D_8005B1B0;
                    label = title + 0x17e8;
                    color = D_8002F628;
                }
            }
            if (time < PackedTime_ToMs(times, 3)) {
                sprite = &D_8005B1A4;
                label = sR_BRONZEPRIZE;
                color = D_8002F628;
            }
            if (time < PackedTime_ToMs(times, 2)) {
                sprite = &D_8005B198;
                label = sR_SILVERPRIZE;
                color = D_8002F624;
            }
            if (time < PackedTime_ToMs(times, 1)) {
                sprite = &D_8005B18C;
                label = sR_GOLDPRIZE;
                color = D_8002F620;
            }
            if (t == 1) {
                Sfx_PlayMenu(3);
            }
            if (t >= 0xd) {
                t = 0xc;
            }
            text.rgb = Rgb_Lerp(D_8002F61C, (unsigned char *)&color, t, 0xc);
            Text_DrawCentered(&text, label, 0xa0, 0xce, -1);
            Text_SetFont(&text, &D_80093130);
            Time_FormatMs(buf, outcome->time);
            Text_DrawDigitsCentered(&text, (signed char *)buf, 0xa0, 0xb2, 8, 7, 1, 0);
            if (sprite != 0) {
                int half = (short)sprite->h / 2;
                int height;
                int skew;
                int width;
                unsigned int rgb;
                PrimFT4 *prim;
                int left;
                int right;
                int top;

                t = t * 16 / 12;
                height = D_8002F5E8[0x10 - t] * half >> 7;
                skew = D_8002F5E8[t] * half >> 8;
                rgb = Rgb_Lerp(D_8002F604, D_8002F5FC, t, 0x10);
                if (t == 0x10) {
                    rgb = D_8002F600;
                }
                width = (short)sprite->w >> 1;
                prim = Prim_AddPolyFT4(ot, rgb);
                left = 0xa0 - width;
                right = width + 0xa0;
                top = 0x6e - height;
                prim->x2 = left - skew;
                prim->x1 = right - skew;
                prim->y1 = top;
                prim->y0 = top;
                prim->x0 = left + skew;
                prim->x3 = right + skew;
                top = height + 0x6e;
                prim->y3 = top;
                prim->y2 = top;
                prim->u2 = prim->u0 = sprite->u;
                prim->u3 = prim->u1 = sprite->u + (unsigned char)sprite->w - 1;
                prim->v1 = prim->v0 = sprite->v;
                prim->v3 = prim->v2 = sprite->v + (unsigned char)sprite->h - 1;
                prim->clut = sprite->clut;
                prim->tpage = sprite->tpage | 0x20;
            }
        }
        t = *res - 0x5a;
        if (t <= 0) {
            return;
        }
        if (t >= 0x11) {
            t = 0x10;
        }
        t *= 4;
    } else {
        int count;
        int laps = 0;
        unsigned int leader = -1;
        int type;

        Result_DrawTitle(ot, *res);
        ResultBanner_Draw(res + 1, ot, 0xa0, 0xd8);
        count = g_RaceSetup.carCount;
        for (i = 5; i >= 0; i--) {
            order[i] = -1;
        }
        for (i = 0; i < 6; i++) {
            int place = g_RaceWorld.vehicles[i].phys.racePos - 1;

            if ((unsigned int)place < 6) {
                order[place] = i;
            }
        }
        for (i = 0; i < 6; i++) {
            RaceVehicle *vehicle;
            int most;

            if (order[i] < 0) {
                continue;
            }
            vehicle = (RaceVehicle *)(&g_Race_IsDemo + order[i] * sizeof(RaceVehicle) + 0x188);
            most = g_RaceSetup.lapCount + 1;
            laps = vehicle->phys.lap;
            leader = vehicle->phys.raceTime;
            if (most < laps) {
                laps = most;
            }
            break;
        }
        switch (g_RaceSetup.raceType) {
        case 0: {
            signed char *won;
            int start = 0xbe;

            for (i = 0; i < count; i++) {
                t = *res - start;
                if (t < 0) {
                    break;
                }
                Result_DrawTimeRow(ot, t, i, order[i], laps, leader, &text, 1);
                start += 0x10;
            }
            t = *res - 0xee;
            if (t >= 0) {
                won = sR_PLAYER1WINS;
                win = 0x144080;
                if (order[0] != 0) {
                    won += 0x17;
                }
                if (t >= 9) {
                    t = 8;
                }
                text.rgb = Rgb_Lerp(D_8002F604, (unsigned char *)&win, t, 8);
                Text_SetFont(&text, &D_8009313C);
                Text_DrawCentered(&text, won, 0xa0, 0xb4, 0);
            }
            break;
        }
        case 11:
            t = *res - 0xbe;
            if (t >= 0) {
                signed char *won = sR_YOUWIN;

                winAlt = 0x144080;
                if (order[0] != 0) {
                    won += 0x12;
                }
                if (t >= 9) {
                    t = 8;
                }
                text.rgb = Rgb_Lerp(D_8002F604, (unsigned char *)&winAlt, t, 8);
                Text_SetFont(&text, &D_8009313C);
                Text_DrawCentered(&text, won, 0xa0, 0x8c, 0);
            }
            break;
        case 2:
            if (champ->raceCount >= 2) {
                if (*res >= 0x1ba) {
                    int start = 0x1ba;

                    title = sR_TotalPoints;
                    ChampRaceList_SortStandings(champ, order);
                    for (i = 0; i < count; i++) {
                        int car;

                        t = *res - start;
                        if (t < 0) {
                            break;
                        }
                        car = order[i];
                        Result_DrawPointsRow(ot, t, i, car, champ->points[car], &text, D_8002F610);
                        start += 0x10;
                    }
                    break;
                }
                if (*res >= 0x13c) {
                    int start = 0x13c;

                    title = sR_Points;
                    for (i = 0; i < count; i++) {
                        int car;

                        t = *res - start;
                        if (t < 0) {
                            break;
                        }
                        car = order[i];
                        Result_DrawPointsRow(ot, t, i, car, champ->racePoints[car], &text, D_8002F614);
                        start += 0x10;
                    }
                    break;
                }
            }
            goto rows;
        case 1:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
            break;
        default:
rows: {
            int start = 0xbe;

            for (i = 0; i < count; i++) {
                t = *res - start;
                if (t < 0) {
                    break;
                }
                Result_DrawTimeRow(ot, t, i, order[i], laps, leader, &text, 0);
                start += 0x10;
            }
            break;
        }
        }
        type = g_RaceSetup.raceType;
        if (type == 1 || (type != 0 && type < 12 && type >= 6)) {
            t = *res - 0x9e;
            if (t <= 0) {
                return;
            }
            if (t >= 0x11) {
                t = 0x10;
            }
            t *= 6;
        } else {
            RECT *rect;

            t = *res - 0x9e;
            if (t > 0) {
                if (t >= 0x11) {
                    t = 0x10;
                }
                t *= 4;
                rect = Prim_AddTile(ot, t | t << 8 | t << 16 | 0x2000000);
                rect->x = 0;
                rect->y = 0x32;
                rect->w = 0x140;
                rect->h = 0xbe;
                Prim_AddDrawMode(ot, 0x40);
            }
            if (*res >= 0x96) {
                rect = Prim_AddTile(ot, 0x80);
                rect->x = 0;
                rect->y = 0x30;
                rect->w = 0x140;
                rect->h = 2;
                Text_SetFont(&text, &D_8009313C);
                text.rgb = 0xa376f;
                Text_DrawCentered(&text, title, 0xa0, 0x2e, 0);
                rect = Prim_AddTile(ot, 0);
                rect->x = 0;
                rect->y = 0;
                rect->w = 0x140;
                rect->h = 0x30;
            }
            return;
        }
    }
    {
        RECT *rect = Prim_AddTile(ot, t | t << 8 | t << 16 | 0x2000000);

        rect->x = 0;
        rect->y = 0;
        rect->w = 0x140;
        rect->h = 0xf0;
        Prim_AddDrawMode(ot, 0x40);
    }
}
}
