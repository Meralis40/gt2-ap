// OVL2 8001894c-80019064 (spectable): Spec table.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "common.h"
#include "syslib.h"

extern "C" {
extern ArcTextItemTmpl D_8004F698; // the labels' template
extern ArcTextItemTmpl D_8004F6B4; // the values' template
extern char D_800F866D[];          // the labels and formats
extern char D_800F8698[];
extern char D_800270F8[];
extern char D_80027100[];


// The labels are grey, blue and orange, and every text starts as its label.
void SpecTable_Init(ArcSpecTable *t)
{
    ArcTextItem *weightValue;
    ArcTextItem *powerLabel;
    ArcTextItem *powerValue;
    ArcTextItem *torqueLabel;
    ArcTextItem *torqueValue;
    ArcTextItemTmpl *tmpl;
    char *text;
    char *powerText;

    D_8004F698.rgbFrom = 0xc0c0c0;
    TextItem_Init_OVL2(&t->weightLabel, &D_8004F698);
    text = D_800F866D;
    weightValue = &t->weightValue;
    tmpl = &D_8004F6B4;
    t->weightLabel.text = text;
    TextItem_Init_OVL2(weightValue, tmpl);
    weightValue->text = text;
    D_8004F698.rgbFrom = 0x3e8ede;
    powerLabel = &t->powerLabel;
    TextItem_Init_OVL2(powerLabel, &D_8004F698);
    powerValue = &t->powerValue;
    powerText = text + 0xd;
    powerLabel->text = powerText;
    TextItem_Init_OVL2(powerValue, tmpl);
    powerValue->text = powerText;
    D_8004F698.rgbFrom = 0xf08e3c;
    torqueLabel = &t->torqueLabel;
    TextItem_Init_OVL2(torqueLabel, &D_8004F698);
    text += 0x2b;
    torqueValue = &t->torqueValue;
    torqueLabel->text = text;
    TextItem_Init_OVL2(torqueValue, tmpl);
    torqueValue->text = text;
    t->anim = -1;
}

// Weight in pounds, power in hp and torque in ft-lb, each with its rpm when it has one,
// then the widths of the two columns.
void SpecTable_Set(ArcSpecTable *t, ArcSpecValues *v, TextCtx *ctx)
{
    char *strings = D_800F866D;
    char *text;
    char *strings2;
    ArcTextItem *value;
    ArcTextItem *item;
    int rpm;
    int width;
    int w;

    t->weightLabel.text = strings;
    TextItem_Show_OVL2(&t->weightLabel, -1);
    text = t->weightText;
    sprintf(text, strings + 8, v->weight * 22046 / 10000);
    item = &t->weightValue;
    item->text = text;
    TextItem_Show_OVL2(item, -1);
    item = &t->powerLabel;
    item->text = strings + 0xd;
    TextItem_Show_OVL2(item, -1);
    rpm = v->powerRpm;
    if (rpm == 0) {
        sprintf(t->powerText, strings + 0x26, v->power * 1000 / 1014);
    } else {
        sprintf(t->powerText, strings + 0x1b, v->power * 1000 / 1014, rpm);
    }
    item = &t->powerValue;
    item->text = t->powerText;
    TextItem_Show_OVL2(item, -1);
    item = &t->torqueLabel;
    strings2 = D_800F8698;
    item->text = strings2;
    TextItem_Show_OVL2(item, -1);
    if (v->torque == 0) {
        sprintf(t->torqueText, D_800270F8);
    } else {
        int rpm2;

        w = v->torque * 72329 / 10000;
        rpm2 = v->torqueRpm;
        if (rpm2 < 0) {
            int frac = w - w / 10 * 10;

            sprintf(t->torqueText, strings2 + 0x23, (w - frac) / 10, frac, -rpm2);
        } else if (rpm2 == 0) {
            sprintf(t->torqueText, D_80027100, w / 10, w % 10, strings2 - 0xf2);
        } else {
            int frac = w - w / 10 * 10;

            sprintf(t->torqueText, strings2 + 0x12, (w - frac) / 10, frac, rpm2);
        }
    }
    value = &t->torqueValue;
    value->text = t->torqueText;
    TextItem_Show_OVL2(value, -1);
    width = TextItem_MeasureWidth_OVL2(&t->weightLabel, ctx);
    w = TextItem_MeasureWidth_OVL2(&t->powerLabel, ctx);
    if (width < w) {
        width = w;
    }
    w = TextItem_MeasureWidth_OVL2(&t->torqueLabel, ctx);
    if (width < w) {
        width = w;
    }
    {
        int valueWidth = TextItem_MeasureWidth_OVL2(&t->weightValue, ctx);

        w = TextItem_MeasureWidth_OVL2(&t->powerValue, ctx);
        if (valueWidth < w) {
            valueWidth = w;
        }
        w = TextItem_MeasureWidth_OVL2(value, ctx);
        if (valueWidth < w) {
            valueWidth = w;
        }
        t->labelWidth = width;
        t->valueWidth = valueWidth;
    }
    t->anim = 0;
}

// Hides all items immediately.
void SpecTable_Reset(ArcSpecTable *t)
{
    (t->weightLabel).anim = -1;
    (t->weightValue).anim = -1;
    (t->powerLabel).anim = -1;
    (t->powerValue).anim = -1;
    (t->torqueLabel).anim = -1;
    (t->torqueValue).anim = -1;
    t->anim = -1;
}

// Starts the close animation of the 6 items (if shown).
void SpecTable_Close(ArcSpecTable *t)
{
    if (-1 < t->anim) {
        TextItem_Hide_OVL2(&t->weightLabel);
        TextItem_Hide_OVL2(&t->weightValue);
        TextItem_Hide_OVL2(&t->powerLabel);
        TextItem_Hide_OVL2(&t->powerValue);
        TextItem_Hide_OVL2(&t->torqueLabel);
        TextItem_Hide_OVL2(&t->torqueValue);
        t->anim = -1;
    }
}

// Advances the 6 TextItems.
void SpecTable_Update(ArcSpecTable *t)
{
    TextItem_Update_OVL2(&t->weightLabel);
    TextItem_Update_OVL2(&t->weightValue);
    TextItem_Update_OVL2(&t->powerLabel);
    TextItem_Update_OVL2(&t->powerValue);
    TextItem_Update_OVL2(&t->torqueLabel);
    TextItem_Update_OVL2(&t->torqueValue);
}

// Three rows 16 apart, with a line above and below while shown.
void SpecTable_Draw(ArcSpecTable *t, void *ot, int x, int y, TextCtx *ctx)
{
    int labelX;
    int valueX;
    unsigned short right;
    int powerY;
    int torqueY;
    ArcTextItem *item;
    LineF2 *l;

    x -= t->valueWidth;
    labelX = x - 3;
    valueX = x + 2;
    t->weightLabel.y = y;
    t->weightLabel.x = labelX;
    TextItem_Draw_OVL2(&t->weightLabel, ot, ctx);
    item = &t->weightValue;
    item->x = valueX;
    item->y = y;
    TextItem_Draw_OVL2(item, ot, ctx);
    item = &t->powerLabel;
    powerY = y + 0x10;
    item->x = labelX;
    item->y = powerY;
    TextItem_Draw_OVL2(item, ot, ctx);
    item = &t->powerValue;
    item->x = valueX;
    item->y = powerY;
    TextItem_Draw_OVL2(item, ot, ctx);
    item = &t->torqueLabel;
    torqueY = y + 0x20;
    item->x = labelX;
    item->y = torqueY;
    TextItem_Draw_OVL2(item, ot, ctx);
    item = &t->torqueValue;
    item->x = valueX;
    item->y = torqueY;
    TextItem_Draw_OVL2(item, ot, ctx);
    if (t->anim >= 0) {
        l = Prim_AddLineF2((uint *)ot, 0x3e3e3e);
        l->x0 = x - t->labelWidth - 5;
        right = t->valueWidth + x + 4;
        l->y1 = y - 0xf;
        l->y0 = y - 0xf;
        l->x1 = right;
        l = Prim_AddLineF2((uint *)ot, 0x3e3e3e);
        l->x0 = x - t->labelWidth - 5;
        right = t->valueWidth + x + 4;
        l->y1 = y + 0x24;
        l->y0 = y + 0x24;
        l->x1 = right;
    }
}
}
