// OVL0 80048bd8-80049a14 (resultwidgets): Result screen widgets.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"
#include "gamestate.h"
#include "prim.h"

// A text context's draw mode word (clut and tpage).
#define TEXT_MODE(ctx) (*(unsigned int *)&(ctx)->clut)

// How InfoBar_Init sets up a bar.
struct InfoBarDesc {
    uint rgbTop;
    uint rgbBottom;
    char *label;
    uint labelRgb;
    uint value;
    uint valueRgb;
    ushort kind;
    ushort pad1a;
    ushort width;
    ushort height;
};

// A time shown with its lap (-1 for none).
struct LapTime {
    uint time;
    short lap;
};

// The look of a time: colours, digit widths and font.
struct TimeStyle {
    uint rgb;
    uint lapRgb;
    short width;
    short narrow;
    short lapGap;
    short digitShift;
    Font *font;
};

// The colours of a record row.
struct RecordRgb {
    uint rank;
    uint times[5];
};

// A row of a records table.
struct RecordRow {
    short rank;
    TimeRecord *record;
    RecordRgb rgb;
};

extern "C" {
extern TextStyle D_8005ABC8;
extern WipeBar D_8005ABAC;
extern WipeBar D_8005ABE4;
extern TextStyle D_8005AC00;
extern unsigned char D_8005AC1C[];
extern unsigned char D_8005AB58[];
extern char D_8005A864[];
extern char sR_Lap[];
extern char sR_Mph[];
extern Font g_OVL0_FontB;
extern Font g_OVL0_FontD;
extern KanjiFont *g_OVL0_KanjiFont;
void TitleBanner_Init(TitleBanner *b, char *text, TextCtx *ctx);
void InfoBar_Init(InfoBar *b, InfoBarDesc *desc);

void TitleBanner_Init(TitleBanner *b, char *text, TextCtx *ctx)
{
    TextItem_Init(&b->text, &D_8005ABC8, text);
    b->bar = D_8005ABAC;
    b->bar.width = 0x100;
    b->bar.t = -1;
}

// Title text centred at x, wipe bar from x - 0x80.
void TitleBanner_Draw(TitleBanner *b, unsigned int *ot, int x, int y, TextCtx *ctx)
{
    int iVar1;

    iVar1 = TextItem_MeasureWidth(&b->text, ctx);
    TextItem_Draw(&b->text, ot, x - (iVar1 >> 1), y + 8, ctx);
    WipeBar_Draw(&b->bar, ot + 1, x + -0x80, y - ((int)((unsigned int)(unsigned short)(b->bar).height << 0x10) >> 0x11));
    Prim_AddDrawMode((OtTag *)(ot + 1), 0x220);
}

// Builds the bar from desc, whose bar size also goes to the two templates.
void InfoBar_Init(InfoBar *b, InfoBarDesc *desc)
{
    TextItem *label = &b->label;

    b->kind = desc->kind;
    D_8005ABE4.duration = desc->height;
    D_8005AC00.revealSteps = desc->height;
    TextItem_Init(label, &D_8005AC00, desc->label);
    *(uint *)label->rgbFrom = desc->labelRgb;
    b->bar = D_8005ABE4;
    b->bar.rgbTop = desc->rgbTop;
    b->bar.rgbBottom = desc->rgbBottom;
    b->bar.width = desc->width;
    b->bar.t = -1;
    b->value = desc->value;
    b->valueRgb = desc->valueRgb;
    b->anim = -1;
}

// Updates the label and bar, and fades the value in over 16 frames (or out when anim <
// -1).
void InfoBar_Update(InfoBar *b)
{
    short sVar1;

    TextItem_Update(&b->label);
    WipeBar_Update(&b->bar);
    if (b->anim < 0) {
        if (b->anim < -1) {
            b->anim = b->anim + 1;
        }
    }
    else {
        sVar1 = b->anim + 1;
        b->anim = sVar1;
        if (0x10 < sVar1) {
            b->anim = 0x10;
        }
    }
}

#ifdef NON_MATCHING
// Label, wipe bar and the fading value; kind & 3: 0 text, 1 time, 2 digits.
void InfoBar_Draw(InfoBar *b, uint *ot, int x, int y, TextCtx *ctx)
{
    char buf[0x40];
    int anim = b->anim;

    if (anim < -1) {
        anim = ~anim;
    }
    if (anim > 0) {
        int one = 1;
        int width;

        ctx->ot = ot;
        TEXT_MODE(ctx) = (TEXT_MODE(ctx) & ~0x600000) | 0x200000;
        ctx->rgb = Rgb_Lerp((unsigned char *)&b->valueRgb, D_8005AC1C, 0x10 - anim, 0x10);
        switch (b->kind & 3) {
        case 1:
            Text_SetFont(ctx, &g_OVL0_FontB);
            Time_FormatMs(buf, b->value);
            width = Text_MeasureDigits(ctx, (signed char *)buf, 6, 5, 0);
            Text_DrawDigits(ctx, (signed char *)buf, x + b->bar.width - (width + 6), y + 9, 6, 5, 0, 0);
            break;
        case 0:
            Text_SetFont(ctx, &g_OVL0_FontD);
            width = Text_MeasureString(ctx, (signed char *)b->value, 1);
            Text_DrawString(ctx, (signed char *)b->value, x + b->bar.width - (width + 6), y + 0xd, one);
            break;
        default:
            Text_SetFont(ctx, &g_OVL0_FontB);
            Text_DrawStringMonoDigitsRight(ctx, (signed char *)b->value, x + b->bar.width - 6, y + 9, one, -2, 0);
            break;
        }
    }
    TextItem_Draw(&b->label, ot, x + 6, y + 9, ctx);
    {
        uint *ot2 = ot + 1;

        WipeBar_Draw(&b->bar, ot2, x, y - ((int)((unsigned int)(unsigned short)b->bar.height << 0x10) >> 0x11));
        Prim_AddDrawMode((OtTag *)ot2, 0x220);
        if (b->kind & 4) {
            uint top = b->bar.rgbTop;
            uint bottom = b->bar.rgbBottom;
            uint out = b->bar.rgbOut;

            b->bar.rgbTop = 0x2c0c0c0;
            b->bar.rgbBottom = 0;
            b->bar.rgbOut = 0;
            WipeBar_Draw(&b->bar, ot2, x, y - ((int)((unsigned int)(unsigned short)b->bar.height << 0x10) >> 0x11));
            Prim_AddDrawMode((OtTag *)ot2, 0x240);
            b->bar.rgbTop = top;
            b->bar.rgbBottom = bottom;
            b->bar.rgbOut = out;
        }
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/resultwidgets", InfoBar_Draw);
#endif

// A time, right-aligned at x, and its lap number when it has one.
void LapTime_Draw(LapTime *t, uint *ot, TextCtx *ctx, int x, int y, TimeStyle *style, int mode)
{
    char buf[0x10];
    int width;

    Text_SetFont(ctx, style->font);
    ctx->ot = ot;
    TEXT_MODE(ctx) = (TEXT_MODE(ctx) & ~0x600000) | ((mode & 3) << 21);
    Time_FormatMs(buf, t->time);
    {
        width = Text_MeasureDigits(ctx, (signed char *)buf, style->width, style->narrow, 0);

        ctx->rgb = style->rgb;
        Text_DrawDigits(ctx, (signed char *)buf, x - width, y, style->width, style->narrow, 0, 0);
    }
    if (t->lap >= 0) {
        char *lap = sR_Lap;

        x -= style->lapGap;
        strcpy(buf, lap);
        ctx->rgb = style->lapRgb;
        {
            int one = 1;

            width = Text_DrawString(ctx, (signed char *)lap, x, y, one);
            sprintf(buf, D_8005A864, t->lap);
            Text_DrawStringMonoDigits(ctx, (signed char *)buf, x + width + 6, y, one, style->digitShift, 0);
        }
    }
}

// c[0] = max, c[1] = counter: counts up to max, or from a negative (closing) value up
// to -1. Copy of the resident FadeCounter.
void FadeCounter_Update(short *c)
{
    short sVar1;

    if (c[1] < 0) {
        if (c[1] < -1) {
            c[1] = c[1] + 1;
            return;
        }
    }
    else {
        sVar1 = c[1] + 1;
        c[1] = sVar1;
        if (*c <= sVar1) {
            c[1] = *c;
        }
    }
}

// A time or speed that zooms in: fade[0] frames long, fade[1] the frame, ~frame
// while closing and -1 hidden.
void AnimatedTime_Draw(short *fade, uint *ot, TextCtx *ctx, int value, int x, int y, int mode, TimeStyle *style,
                       int isSpeed, int centre)
{
    char buf[0x10];
    int zoom = 1;
    int phase;

    Text_SetFont(ctx, style->font);
    ctx->ot = ot;
    if (fade[1] == -1) {
        return;
    }
    phase = fade[1];
    if (phase < 0) {
        phase = ~phase;
        zoom = 0;
    }
    if (isSpeed) {
        Speed_Format(buf, value);
        strcat(buf, sR_Mph);
        if (centre) {
            x += Text_MeasureStringMonoDigits(ctx, (signed char *)buf, style->digitShift, 0) >> 1;
        }
    } else {
        Time_FormatMs(buf, value);
        if (centre) {
            x += Text_MeasureDigits(ctx, (signed char *)buf, style->width, style->narrow, 0) >> 1;
        }
    }
    {
        int length = fade[0];

        phase = 0x80 - (phase << 7) / length;
        if (fade[1] < length && (zoom & 1)) {
            ctx->rgb = (phase | phase << 8 | phase << 16) | 0x2000000;
            TEXT_MODE(ctx) = (TEXT_MODE(ctx) & ~0x600000) | 0x200000;
            if (isSpeed) {
                int half = phase >> 1;

                Text_DrawStringMonoDigitsRight(ctx, (signed char *)buf, x - half, y, style->digitShift, -2, 0);
                Text_DrawStringMonoDigitsRight(ctx, (signed char *)buf, x + half, y, style->digitShift, -2, 0);
            } else {
                int half = phase >> 2;

                Text_DrawDigitsRight(ctx, (signed char *)buf, x - half, y, style->width, style->narrow, 0, 0);
                Text_DrawDigitsRight(ctx, (signed char *)buf, x + half, y, style->width, style->narrow, 0, 0);
            }
        }
    }
    ctx->rgb = Rgb_Lerp((unsigned char *)style, D_8005AB58, phase, 0x80);
    TEXT_MODE(ctx) = (TEXT_MODE(ctx) & ~0x600000) | ((mode & 3) << 21);
    if (isSpeed) {
        Text_DrawStringMonoDigitsRight(ctx, (signed char *)buf, x, y, style->digitShift, -3, 0);
    } else {
        Text_DrawDigitsRight(ctx, (signed char *)buf, x, y, style->width, style->narrow, 0, 0);
    }
}

#ifdef NON_MATCHING
void RecordRow_Init(void *rowPtr, int rank, void *record, uint *rgb)
{
    RecordRow *row = (RecordRow *)rowPtr;

    row->rank = rank;
    row->record = (TimeRecord *)record;
    row->rgb = *(RecordRgb *)rgb;
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/resultwidgets", RecordRow_Init);
#endif

#ifdef NON_MATCHING
// The rank, then the four sectors and the total, fading in from D_8005AB58.
void RecordRow_Draw(void *row, uint *ot, TextCtx *ctx, int x, int y, int fade)
{
    char buf[0x100];
    int six;
    int five;
    int i;

    ctx->ot = ot;
    TEXT_MODE(ctx) = (TEXT_MODE(ctx) & ~0x600000) | 0x200000;
    Text_SetFont(ctx, &g_OVL0_FontB);
    if (((RecordRow *)row)->rank > 0) {
        sprintf(buf, D_8005A864, ((RecordRow *)row)->rank);
        ctx->rgb = Rgb_Lerp(D_8005AB58, (unsigned char *)&((RecordRow *)row)->rgb.rank, fade, 0x80);
        Text_DrawStringMonoDigitsRight(ctx, (signed char *)buf, x, y, 1, -2, 0);
    }
    x += 0x3c;
    i = 0;
    six = 6;
    five = 5;
    for (; i < 5; i++) {
        unsigned int time = *(unsigned int *)((RecordRow *)row)->record;

        if (i < 4) {
            time = Records_GetSectorTime(((RecordRow *)row)->record, i);
        }
        Time_FormatMs(buf, time);
        ctx->rgb = Rgb_Lerp(D_8005AB58, (unsigned char *)&((RecordRow *)row)->rgb.times[i], fade, 0x80);
        Text_DrawDigitsRight(ctx, (signed char *)buf, x, y, six, five, 0, 0);
        x += 0x3c;
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/resultwidgets", RecordRow_Draw);
#endif

#ifdef NON_MATCHING
// The stage of a w x h car view.
void ResultCarView_Init(ResultCarView *view, int w, int h)
{
    int rgb = 0xa2a2a2;

    view->drawDisc = 1;
    view->discSemi = 1;
    view->rotX = 0x5e;
    view->rotY = 0x1500;
    view->viewport[2] = w;
    view->viewport[3] = h;
    h = h * 62 / 100;
    w >>= 1;
    view->discRgb = rgb;
    view->rotZ = 0;
    view->unk_b4 = 0;
    view->unk_b8 = 0;
    view->unk_bc = 0;
    view->pos[1] = 0;
    view->pos[0] = 0;
    view->viewport[0] = 0;
    view->viewport[1] = 0;
    view->frustum[1] = w;
    view->unk00 = 0;
    view->pos[2] = 0xb0000;
    view->frustum[0] = -w;
    view->frustum[4] = 0x190;
    view->frustum[5] = 0x7fff;
    view->frustum[2] = h * 80 / 100;
    view->frustum[3] = -(h / 5);
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/resultwidgets", ResultCarView_Init);
#endif

// Turns the car 0x10 per frame; after frame 0xfb90 also pulls the camera back.
void ResultCarView_Spin(ResultCarView *view, int frame)
{
    view->rotY = view->rotY + 0x10U & 0x3fff;
    if (0xfb90 < frame) {
        view->pos[2] = view->pos[2] + 0x8000;
    }
}

// Japanese only: a line of kanji fading from white to rgb.
void Kanji_DrawFaded(uint *ot, unsigned char *text, int x, int y, int fade, uint rgb, int centre)
{
    uint white = 0x2000000;
    KanjiFont *font = g_OVL0_KanjiFont;
    unsigned char *p;
    int width = 0;
    int bottom;

    if (g_GameState.language != 0) {
        return;
    }
    if (centre) {
        unsigned char *q;

        for (q = text; *q != 0; q += 2) {
            width += 0xc;
        }
        x -= width >> 1;
    }
    p = text;
    bottom = y + 0x1f;
    while (*p != 0) {
        KanjiGlyph *glyph = KanjiFont_FindGlyph(font, p[0] << 8 | p[1]);

        if (glyph != 0) {
            KanjiQuad *quad = KanjiFont_AddGlyphQuad(glyph, ot, Rgb_Lerp((unsigned char *)&white, (unsigned char *)&rgb, fade, 0x80));
            int right = x + 0xf;

            quad->x0 = x;
            quad->y0 = y;
            quad->x1 = right;
            quad->y1 = y;
            quad->x2 = x;
            quad->y2 = bottom;
            quad->x3 = right;
            quad->y3 = bottom;
            quad->tpage = (quad->tpage & 0xff9f) | 0x20;
        }
        x += 0xc;
        p += 2;
    }
    Prim_AddDrawMode((OtTag *)ot, 0x240);
}
}
