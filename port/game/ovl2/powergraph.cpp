// OVL2 800121e0-80013218 (powergraph): Power graph: another version of the resident
// PowerGraphView.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "copies.h"
#include "common.h"

extern "C" {
extern char (&D_800F85A3)[]; // hp
extern char (&D_800F85A6)[]; // lb-ft
extern char (&D_800F85AC)[]; // x1000 rpm


// Copies the template, and hides the graph.
void PowerGraphView_Init_OVL2(ArcPowerGraph *v, void *arg)
{
    ArcPowerGraphTmpl *tmpl = (ArcPowerGraphTmpl *)arg;
    int value;

    if (tmpl != 0) {
        value = tmpl->w;
        v->w = value;
        value = tmpl->h;
        v->h = value;
        v->font = tmpl->font;
        value = tmpl->labelGap;
        v->labelGap = value;
        value = tmpl->textMode;
        v->textMode = value;
        value = tmpl->charH;
        v->charH = value;
        value = tmpl->maxRpm;
        v->maxRpm = value;
        value = tmpl->maxPower;
        v->maxPower = value;
        value = tmpl->maxTorque;
        v->maxTorque = value;
        *(unsigned int *)v->powerRgb = tmpl->powerRgb;
        *(unsigned int *)v->torqueRgb = tmpl->torqueRgb;
        *(unsigned int *)v->rpmRgb = tmpl->rpmRgb;
    }
    v->abr = 1;
    v->stagger = 0x14;
    v->frame = -1;
    v->endFrame = (signed char)v->stagger + 0x4b;
}

// Computes the scales and starts the fade-in.
void PowerGraphView_Show_OVL2(ArcPowerGraph *v)
{
    PowerGraphView_ComputeScale_OVL2(v);
    v->frame = 0;
    v->endFrame = (signed char)v->stagger + 0x4b;
}

// Hidden at once (counter -1).
void PowerGraphView_Hide_OVL2(ArcPowerGraph *v)
{
    v->frame = -1;
}

// Copy of PowerGraphView_Close.
void PowerGraphView_Close_OVL2(ArcPowerGraph *v)
{
    if (0 < v->frame) {
        v->frame = -0x10;
    }
}

// Fades in, then loops back; a negative frame counts up to -1.
void PowerGraphView_Update_OVL2(ArcPowerGraph *v)
{
    short frame;

    if (v->frame < 0) {
        if (v->frame < -1) {
            v->frame = v->frame + 1;
            return;
        }
    } else {
        frame = v->frame + 1;
        v->frame = frame;
        if (v->endFrame < frame) {
            v->frame = (signed char)v->stagger + 0xf;
        }
    }
}

// Copy of PowerGraphView_ComputeScale.
void PowerGraphView_ComputeScale_OVL2(ArcPowerGraph *v)
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
void PowerGraphView_DrawAxis_OVL2(ArcPowerGraph *v, uint *ot, TextCtx *ctx, int torque)
{
    char buf[256];
    int h = v->h;
    char *unit;
    int r, g, b;
    int step;
    int ticks;
    int top;
    int x;
    int labelY;
    int tickH;
    int i;
    int value;
    int limit;

    if (torque != 0) {
        unit = D_800F85A6;
        r = v->torqueRgb[0];
        g = v->torqueRgb[1];
        b = v->torqueRgb[2];
        step = v->torqueStep;
        ticks = v->torqueTicks;
        x = v->x + v->w - 2;
        top = v->torqueTop;
    } else {
        unit = D_800F85A3;
        r = v->powerRgb[0];
        g = v->powerRgb[1];
        b = v->powerRgb[2];
        step = v->powerStep;
        ticks = v->powerTicks;
        top = v->powerTop;
        x = v->x;
    }
    labelY = v->y + (v->charH >> 1);
    ctx->ot = ot;
    Text_SetFont(ctx, v->font);
    tickH = h / ticks;
    *(uint *)&ctx->clut = (*(uint *)&ctx->clut & 0xff9fffff) | ((v->abr & 3) << 21);
    tickH -= 2;
    if (tickH <= 0) {
        tickH = 1;
    }
    limit = v->y + h;
    value = step;
    for (i = 1; i <= ticks; i++) {
        int k = v->frame - v->stagger * i / ticks;
        int ty;
        int r2, g2, b2;
        unsigned int full;
        unsigned int half;
        int ch;
        int ly;
        RECT *t;

        if (k < 0) {
            break;
        }
        k = 15 - k;
        if (k < 0) {
            k = 0;
        }
        ty = h * value / top;
        r2 = r + (255 - r) * k / 15;
        g2 = g + (255 - g) * k / 15;
        b2 = b + (255 - b) * k / 15;
        full = r2 | g2 << 8 | b2 << 16 | 0x2000000;
        half = r2 >> 1 | (g2 >> 1) << 8 | (b2 >> 1) << 16 | 0x2000000;
        ty = v->y + h - ty;
        t = Prim_AddTile(ot, full);
        t->x = x;
        t->y = ty;
        t->w = 2;
        t->h = tickH;
        ch = v->charH;
        ly = ty + (ch >> 1);
        if ((ly < limit) & (labelY + ch + (ch >> 2) < ly)) {
            int tx;
            int tickX;

            IntToStr_OVL2(buf, value);
            if (torque != 0) {
                tx = x + 4;
                tickX = x + 2;
            } else {
                tx = Text_MeasureStringMonoDigits(ctx, (signed char *)buf, 1, 0) + 4;
                tx = x - tx;
                tickX = x - 2;
            }
            ctx->rgb = half;
            Text_DrawStringMonoDigits(ctx, (signed char *)buf, tx, ly, 1, v->textMode, 0);
            t = Prim_AddTile(ot, full);
            t->x = tickX;
            t->y = ty;
            t->w = 2;
            t->h = 1;
            limit = ly - ch - v->labelGap;
        }
        if (i == ticks) {
            int ux = x + 3;

            if (torque == 0) {
                ux = Text_MeasureString(ctx, (signed char *)unit, 1) + 2;
                ux = x - ux;
            }
            ctx->rgb = half;
            Text_DrawString(ctx, (signed char *)unit, ux, labelY, 1);
        }
        value += step;
    }
}

// The rpm scale under the graph: a tick and a label per 1000 rpm, each with a line
// growing up the graph as it fades in.
void PowerGraphView_DrawRpmAxis_OVL2(ArcPowerGraph *v, uint *ot, TextCtx *ctx)
{
    char buf[256];
    int ticks = v->rpmTicks;
    short tickW;
    int r, g, b;
    int i;
    int limit;
    int labelY;

    ctx->ot = ot;
    Text_SetFont(ctx, v->font);
    *(uint *)&ctx->clut = (*(uint *)&ctx->clut & 0xff9fffff) | ((v->abr & 3) << 21);
    tickW = v->w / ticks;
    r = v->rpmRgb[0];
    g = v->rpmRgb[1];
    b = v->rpmRgb[2];
    limit = v->x;
    labelY = v->y + v->h + v->charH + 3;
    tickW -= 1;
    for (i = 0; i < ticks; i++) {
        int k = v->frame - v->stagger * i / ticks;
        int r2, g2, b2;
        unsigned int half;
        unsigned int quarter;
        int x;
        int w;
        int tx;
        RECT *t;

        if (k < 0) {
            break;
        }
        if (k > 15) {
            k = 15;
        }
        r2 = r * k / 15;
        g2 = g * k / 15;
        b2 = b * k / 15;
        x = v->w * i / ticks;
        half = r2 >> 1 | (g2 >> 1) << 8 | (b2 >> 1) << 16 | 0x2000000;
        quarter = r2 >> 2 | (g2 >> 2) << 8 | (b2 >> 2) << 16 | 0x2000000;
        x = v->x + x;
        t = Prim_AddTile(ot, r2 | g2 << 8 | b2 << 16);
        t->x = x;
        t->h = 2;
        t->w = tickW;
        t->y = v->y + v->h;
        IntToStr_OVL2(buf, i);
        w = Text_MeasureString(ctx, (signed char *)buf, v->textMode);
        tx = x - (w >> 1) - 1;
        if (limit < tx) {
            LineF2 *l;
            int d;

            ctx->rgb = half;
            Text_DrawString(ctx, (signed char *)buf, tx, labelY, v->textMode);
            d = 15 - k;
            limit = tx + w + 1;
            d = d * d / 15;
            l = Prim_AddLineF2(ot, quarter);
            l->x1 = x;
            l->x0 = x;
            l->y0 = v->y + v->h * d / 15;
            l->y1 = v->y + v->h;
        }
        if (i == ticks - 1) {
            ctx->rgb = half;
            w = Text_MeasureString(ctx, (signed char *)D_800F85AC, 1);
            Text_DrawString(ctx, (signed char *)D_800F85AC, v->x + v->w - w - 5, v->y + v->h - 2, 1);
        }
    }
}

// One power or torque curve (samples every 250 rpm, up to maxRpm) as line segments that
// rise as they fade in, the torque in tenths of lb-ft.
void PowerGraphView_DrawCurve_OVL2(ArcPowerGraph *v, uint *ot, TextCtx *ctx, int torque)
{
    int prevY = 0;
    int prevX = 0;
    int prevOk = 0;
    int h = v->h;
    int r, g, b;
    int top;
    short *samples;
    int i = 0;

    if (torque != 0) {
        r = v->torqueRgb[0];
        g = v->torqueRgb[1];
        b = v->torqueRgb[2];
        top = v->torqueTop * 10;
        samples = (short *)v->torqueCurve;
    } else {
        r = v->powerRgb[0];
        g = v->powerRgb[1];
        b = v->powerRgb[2];
        top = v->powerTop;
        samples = (short *)v->powerCurve;
    }
    if (v->samples > 0) {
        int rpm = 0;

        do {
            short n = v->samples;
            int k = v->frame - v->stagger * i / n;
            unsigned int c;

            if (k < 0) {
                break;
            }
            k = 15 - k;
            if (k < 0) {
                k = 0;
            }
            c = (r + (255 - r) * k / 15) | (g + (255 - g) * k / 15) << 8 | (b + (255 - b) * k / 15) << 16;
            if (rpm <= v->maxRpm) {
                int x = v->w * i / n;
                int s = *samples;
                int y = h * s / top;
                int f = 15 - k;
                int x0 = prevX;
                int y0 = prevY;
                int ok0 = prevOk;

                f = f * f / 15;
                y = y * f / 15;
                prevOk = s >= 0;
                prevX = v->x + x;
                prevY = v->y + h - y;
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
            rpm += 250;
        } while (i < v->samples);
    }
}

// The scales and curves, or a closing grey band.
void PowerGraphView_Draw_OVL2(ArcPowerGraph *v, uint *ot, TextCtx *ctx)
{
    int frame = v->frame;

    if (frame != -1) {
        if (frame < -1) {
            GouraudQuad q;
            int k = ~frame;
            int done = v->w * (15 - k) / 15;
            int mid = (v->h + v->charH) >> 1;
            int height = mid * k / 15;
            int grey;

            q.rect.x = v->x + (done >> 1);
            q.rect.w = v->w + done - (done >> 1);
            q.rect.h = height * 2;
            q.top = 0;
            q.rect.y = v->y + mid - height;
            grey = (k * 192) / 15;
            q.bottom = grey;
            Menu_AddGreyBand(ot, &q);
            q.top = grey;
            q.bottom = 0;
            q.rect.x = v->x - done;
            Menu_AddGreyBand(ot, &q);
            Prim_AddDrawMode(ot, 0x20);
        } else {
            PowerGraphView_DrawAxis_OVL2(v, ot, ctx, 0);
            PowerGraphView_DrawAxis_OVL2(v, ot, ctx, 1);
            PowerGraphView_DrawRpmAxis_OVL2(v, ot, ctx);
            PowerGraphView_DrawCurve_OVL2(v, ot, ctx, 0);
            PowerGraphView_DrawCurve_OVL2(v, ot, ctx, 1);
            Prim_AddDrawMode(ot, v->abr);
        }
    }
}
}
