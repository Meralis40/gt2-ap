// Resident code 80073ce4-80074844 (powergraph). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#include "common.h"

extern "C" {
extern char sB_Hp[];
extern char sB_LbFt[];
extern char sB_1000rpm[];
extern char D_8008FB38[];       // "%d"
extern unsigned char D_80092388[];
extern unsigned char D_8009238C[];

void PowerGraphView_Init(PowerGraphView *v)
{
    v->frame = -1;
    v->endFrame = v->stagger + 0x44;
}

void PowerGraphView_Show(PowerGraphView *v)
{
    PowerGraphView_ComputeScale(v);
    v->frame = 0;
    v->endFrame = v->stagger + 0x44;
}

void PowerGraphView_Close(PowerGraphView *v)
{
    if (-1 < v->frame) {
        v->frame = -0x10;
    }
}

void PowerGraphView_Update(PowerGraphView *v)
{
    short sVar1;

    if (v->frame < 0) {
        if (v->frame < -1) {
            v->frame = v->frame + 1;
            return;
        }
    }
    else {
        sVar1 = v->frame + 1;
        v->frame = sVar1;
        if (v->endFrame < sVar1) {
            v->frame = v->stagger + 8;
        }
    }
}

// The scales: up to 110% of the peaks (120% for the torque), in steps of 20 or 50 hp
// and 2 or 5 lb-ft (ten times that for its labels), and one tick per 1000 rpm.
void PowerGraphView_ComputeScale(PowerGraphView *v)
{
    int p = v->maxPower;
    int step = 20;
    int ticks;
    int q;

    if (p >= 301) {
        step = 50;
    }
    ticks = p * 11 / 10 / step + 1;
    q = v->maxTorque;
    v->powerStep = step;
    v->powerTicks = ticks;
    v->powerTop = ticks * step;
    step = 2;
    if (q >= 401) {
        step = 5;
    }
    ticks = q * 12 / 10 * 11 / 10 / (step * 10) + 1;
    v->torqueStep = step;
    v->torqueTicks = ticks;
    ticks = v->maxRpm * 11 / 10000 + 1;
    v->rpmTicks = ticks;
    v->samples = ticks * 4;
    v->torqueTop = (q * 12 / 10 * 11 / 10 / (step * 10) + 1) * step;
}

// A vertical scale with its labels: the power in hp on the left, or the torque in lb-ft
// on the right.
// The instructions are the game's but the registers differ (v, x and the loop's values
// go to other saved registers, and the game keeps the tiles' width 2 in $t0).
void PowerGraphView_DrawAxis(PowerGraphView *v, uint *ot, int torque)
{
    char buf[256];
    TextCtx ctx;
    int h = v->h;
    char *unit;
    int ticks;
    int step;
    int top;
    int x;
    unsigned int rgb;
    int labelY;
    int tickH;
    int i;
    int value;
    int limit;

    if (torque != 0) {
        unit = sB_LbFt;
        ticks = v->torqueTicks;
        step = v->torqueStep;
        top = v->torqueTop;
        x = v->x + v->w - 2;
        rgb = v->torqueRgb;
    } else {
        ticks = v->powerTicks;
        step = v->powerStep;
        top = v->powerTop;
        unit = sB_Hp;
        x = v->x;
        rgb = v->powerRgb;
    }
    labelY = v->y + (v->charH >> 1);
    Text_Init(&ctx, v->tpage);
    ctx.ot = ot;
    Text_SetFont(&ctx, v->font);
    tickH = h / ticks - 2;
    *(uint *)&ctx.clut = (*(uint *)&ctx.clut & 0xff9fffff) | 0x200000;
    if (tickH <= 0) {
        tickH = 1;
    }
    value = step;
    limit = v->y + h;
    for (i = 1; i <= ticks; i++) {
        int k = v->frame - v->stagger * i / ticks;
        unsigned int c8;
        unsigned int c16;
        int ty;
        int ch;
        int ly;
        RECT *t;

        if (k < 0) {
            break;
        }
        k = 8 - k;
        if (k < 0) {
            k = 0;
        }
        c8 = Rgb_Lerp((uchar *)&rgb, D_80092388, k, 8);
        c16 = Rgb_Lerp((uchar *)&rgb, D_80092388, k, 16);
        ty = v->y + h - h * value / top;
        t = Prim_AddTile(ot, c8);
        t->x = x;
        t->y = ty;
        t->w = 2;
        t->h = tickH;
        ch = v->charH;
        ly = ty + (ch >> 1);
        if ((ly < limit) & (labelY + ch + (ch >> 2) < ly)) { // both tests, then an and
            int tx;

            sprintf(buf, D_8008FB38, value);
            if (torque != 0) {
                tx = x + 4;
                limit = x + 2;
            } else {
                tx = Text_MeasureStringMonoDigits(&ctx, buf, 1, 0) + 4;
                tx = x - tx; // in one expression, GCC subtracts 4 from x instead
                limit = x - 2;
            }
            ctx.rgb = c16;
            Text_DrawStringMonoDigits(&ctx, buf, tx, ly, 0, v->textMode, 0);
            t = Prim_AddTile(ot, c8);
            t->x = limit; // the label's tick
            t->y = ty;
            t->w = 2;
            t->h = 1;
            limit = ly - ch - v->labelGap;
        }
        if (i == ticks) {
            int ux = x + 3;

            if (torque == 0) {
                ux = Text_MeasureString(&ctx, unit, 1) + 2;
                ux = x - ux;
            }
            ctx.rgb = c16;
            Text_DrawString(&ctx, unit, ux, labelY, 1);
        }
        value += step;
    }
}

// The rpm scale under the graph: a tick and a label per 1000 rpm, each with a line
// growing up the graph as it fades in.
// The game sets the text flags before it divides the width, where GCC moves the division
// up (a do { } while (0) around the flags stops it, which isn't real code), and the
// registers differ.
void PowerGraphView_DrawRpmAxis(PowerGraphView *v, uint *ot)
{
    char buf[256];
    TextCtx ctx;
    int ticks = v->rpmTicks;
    short tickW;
    int i;
    int limit;
    int labelY;

    Text_Init(&ctx, v->tpage);
    ctx.ot = ot;
    Text_SetFont(&ctx, v->font);
    *(uint *)&ctx.clut = (*(uint *)&ctx.clut & 0xff9fffff) | 0x200000;
    tickW = v->w / ticks;
    limit = v->x;
    labelY = v->y + v->h + v->charH + 3;
    tickW -= 1;
    for (i = 0; i < ticks; i++) {
        int k = v->frame - v->stagger * i / ticks;
        unsigned int c8;
        unsigned int c16;
        unsigned int c32;
        int x;
        int w;
        int tx;
        RECT *t;

        if (k < 0) {
            break;
        }
        if (k > 8) {
            k = 8;
        }
        c8 = Rgb_Lerp(D_8009238C, (uchar *)&v->rpmRgb, k, 8);
        c16 = Rgb_Lerp(D_8009238C, (uchar *)&v->rpmRgb, k, 16);
        c32 = Rgb_Lerp(D_8009238C, (uchar *)&v->rpmRgb, k, 32);
        x = v->x + v->w * i / ticks;
        t = Prim_AddTile(ot, c8);
        t->x = x;
        t->h = 2;
        t->w = tickW;
        t->y = v->y + v->h;
        sprintf(buf, D_8008FB38, i);
        w = Text_MeasureString(&ctx, buf, v->textMode);
        tx = x - (w >> 1) - 1;
        if (limit < tx) {
            LineF2 *l;
            int d;

            ctx.rgb = c16;
            Text_DrawString(&ctx, buf, tx, labelY, v->textMode);
            d = 8 - k;
            limit = tx + w + 1;
            d = d * d / 8;
            l = Prim_AddLineF2(ot, c32);
            l->x1 = x;
            l->x0 = x;
            l->y0 = v->y + v->h * d / 8;
            l->y1 = v->y + v->h;
        }
        if (i == ticks - 1) {
            ctx.rgb = c16;
            w = Text_MeasureString(&ctx, sB_1000rpm, 1);
            Text_DrawString(&ctx, sB_1000rpm, v->x + v->w - w - 5, v->y + v->h - 2, 1);
        }
    }
}

// One power or torque curve (samples every 250 rpm, up to maxRpm) as line segments,
// the torque in tenths of lb-ft.
// Only the registers differ: the game gives i, samples, prevOk and prevX other saved
// registers (s1, s5, s6, fp).
void PowerGraphView_DrawCurve(PowerGraphView *v, uint *ot, short *samples, int torque, int fade)
{
    int prevOk;
    int prevX;
    int prevY;
    int top; // before h, for their stack slots
    int h = v->h;
    unsigned int rgb;
    unsigned int c;

    prevOk = prevX = prevY = 0;
    if (v->frame >= 0) {
        short n;
        int i;

        if (torque != 0) {
            top = v->torqueTop * 10;
            rgb = v->torqueRgb;
        } else {
            top = v->powerTop;
            rgb = v->powerRgb;
        }
        n = v->samples;
        i = 0;
        if (n > 0) {
            int rpm = 0;

            do {
                int k = v->frame - v->stagger * i / n;

                if (k < 0) {
                    break;
                }
                k = 8 - k;
                if (k < 0) {
                    k = 0;
                }
                c = Rgb_Lerp((uchar *)&rgb, D_80092388, k, 8);
                c = Rgb_Lerp(D_8009238C, (uchar *)&c, fade, 0x80);
                if (rpm <= v->maxRpm) {
                    int x0 = prevX;
                    int y0 = prevY;
                    int ok0 = prevOk;
                    int s = *samples;

                    prevOk = s >= 0;
                    prevX = v->x + v->w * i / v->samples;
                    prevY = v->y + h - h * s / top;
                    if (i > 0 && (prevOk & ok0) != 0) {
                        LineF2 *l = Prim_AddLineF2(ot, c);

                        l->x0 = x0;
                        l->x1 = prevX;
                        l->y0 = y0;
                        l->y1 = prevY;
                    }
                }
                samples++;
                i++;
                n = v->samples;
                rpm += 250;
            } while (i < n);
        }
        Prim_AddDrawMode(ot, 0x20);
    }
}

// The scales, then the grid's draw mode; nothing while it is hidden or closing.
void PowerGraphView_DrawGrid(PowerGraphView *v, uint *ot)
{
    if (v->frame != -1) {
        if (v->frame >= -1) { // as one test, GCC folds both into frame >= 0
            PowerGraphView_DrawAxis(v, ot, 0);
            PowerGraphView_DrawAxis(v, ot, 1);
            PowerGraphView_DrawRpmAxis(v, ot);
            Prim_AddDrawMode(ot, 0x20);
        }
    }
}
}
