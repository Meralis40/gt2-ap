// OVL2 80019518-80019e60 (carspecpanel2): Car spec panel.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

static inline int Half(unsigned short v)
{
    return (short)v >> 1;
}

extern "C" {
extern GameState g_GameState;
extern char D_800F85A3[];
extern ArcSprite *D_8004F618[];
int Lerp_Clamped_OVL2(int x, int x0, int x1, int y0, int y1);
void ColorChips_Set(ArcColorChips *c, int count, unsigned short *colors);
void ColorChips_Draw(ArcColorChips *c, void *ot, int x, int y);
void SpecTable_Draw(ArcSpecTable *t, void *ot, int x, int y, TextCtx *ctx);
void StatBar_Draw(ArcStatBar *b, void *ot, int x, int y, TextCtx *ctx, int level);
char *strcat(char *dst, const char *src);

#ifdef NON_MATCHING
// Fills the current slot and shows it. Without perf the power graph stays hidden.
// Otherwise the engine curve is resampled into 80 steps of 250 rpm.
void CarSpecPanel_Show(ArcCarSpecPanel *panel, int icon, EnginePerf *perf, unsigned char *spec3,
                       ArcCarRating *rating, int colors, unsigned short *rgb, TextCtx *text)
{
    ArcSpecValues *values = (ArcSpecValues *)rating;
    int lang = *(unsigned char *)&g_GameState;
    int hasCurve = perf != 0;
    int slot = panel->slot;
    int rpm[64];
    int power[64];
    int torque[64];
    int count;
    int i;
    int k;
    int at;

    IntToStr_OVL2(panel->nameText[slot], values->power * 1000 / 1014);
    strcat(panel->nameText[slot], D_800F85A3);
    panel->name[slot].text = panel->nameText[slot];
    TextItem_Show_OVL2(&panel->name[slot], -1);
    panel->icon[slot].sprite = D_8004F618[lang] + icon;
    IconItem_Show(&panel->icon[slot]);
    panel->levels[slot][0] = spec3[0];
    panel->levels[slot][1] = spec3[1];
    panel->levels[slot][2] = spec3[2];
    StatBar_Show(&panel->bars[slot][0]);
    StatBar_Show(&panel->bars[slot][1]);
    StatBar_Show(&panel->bars[slot][2]);
    ColorChips_Set(&panel->chips[slot], colors, rgb);
    SpecTable_Set(&panel->table[slot], values, text);
    if (!hasCurve) {
        PowerGraphView_Hide_OVL2(&panel->graph[slot]);
    } else {
        panel->graph[slot].maxRpm = perf->rpm[perf->pointCount - 1];
        panel->graph[slot].maxPower = values->power * 1000 / 1014;
        panel->graph[slot].maxTorque = values->torque * 72329 / 10000;
        PowerGraphView_Show_OVL2(&panel->graph[slot]);
        count = perf->pointCount;
        rpm[0] = -1;
        power[0] = -1;
        torque[0] = -1;
        for (i = 0; i < count; i++) {
            rpm[i + 1] = perf->rpm[i];
            power[i + 1] = (perf->power[i] * values->power / perf->maxPower) * 1000 / 1014;
            torque[i + 1] = (perf->torque[i] * values->torque / (unsigned short)perf->maxTorque) * 72329 / 10000;
        }
        count++;
        rpm[i + 1] = -1;
        power[i + 1] = -1;
        torque[i + 1] = -1;
        i = 0;
        at = 0;
        k = 1;
        while (i < 0x4f) {
            if (rpm[k] < at) {
                k++;
                if (k >= count) {
                    break;
                }
            } else {
                ((short *)panel->powerCurve[slot])[i] =
                    Lerp_Clamped_OVL2(at, rpm[k - 1], rpm[k], power[k - 1], power[k]);
                ((short *)panel->torqueCurve[slot])[i] =
                    Lerp_Clamped_OVL2(at, rpm[k - 1], rpm[k], torque[k - 1], torque[k]);
                i++;
                at += 250;
            }
        }
        for (; i < 0x50; i++) {
            ((short *)panel->powerCurve[slot])[i] = -1;
            ((short *)panel->torqueCurve[slot])[i] = -1;
        }
    }
    panel->shown[slot] = 1;
}
#else
INCLUDE_ASM("asm/sim/ovl2/nonmatchings/carspecpanel2", CarSpecPanel_Show);
#endif

// Advances every widget of both slots.
void CarSpecPanel_Update(void *panel)
{
    ArcStatBar *b;
    ArcTextItem *item;
    int iVar1;
    int iVar2;
    int iVar3;
    int iVar4;
    int iVar5;
    int iVar6;

    iVar6 = 0;
    iVar5 = 0x250;
    iVar4 = 0x6f0;
    iVar3 = 0x118;
    iVar2 = 0x88;
    iVar1 = 0x58;
    item = panel;
    do {
        TextItem_Update_OVL2(item);
        IconItem_Update((ArcIconItem *)((int)panel + iVar1));
        PowerGraphView_Update_OVL2((ArcPowerGraph *)((int)panel + iVar2));
        b = (ArcStatBar *)((int)panel + iVar3);
        StatBar_Update(b);
        StatBar_Update(b + 1);
        StatBar_Update(b + 2);
        ColorChips_Update((ArcColorChips *)((int)panel + iVar4));
        SpecTable_Update((ArcSpecTable *)((int)panel + iVar5));
        iVar5 = iVar5 + 0x250;
        iVar4 = iVar4 + 0x86;
        iVar3 = iVar3 + 0x9c;
        iVar2 = iVar2 + 0x44;
        iVar1 = iVar1 + 0x18;
        iVar6 = iVar6 + 1;
        item = item + 1;
    } while (iVar6 < 2);
}

#ifdef NON_MATCHING
// Draws one part of the panel: 0 name and icon, 1 graph, 2 meters, 3 colour chips, 4 table.
// selected picks the current slot, else the one fading out.
void CarSpecPanel_Draw(ArcCarSpecPanel *panel, void *ot, int x, int y, TextCtx *text, int selected,
                       int part)
{
    int slot;

    for (slot = 0;; slot++) {
        int nameX = x + 1;
        int nameY = y + 12;
        ArcTextItem *item = &panel->name[slot];
        int show;

        if (slot >= 2) {
            break;
        }
        show = slot == panel->slot;
        if (selected == 0) {
            show ^= 1;
        }
        if (show != 0) {
            switch (part) {
            case 0: {
                ArcIconItem *icon;

                item->x = nameX;
                item->y = nameY;
                TextItem_Draw_OVL2(item, ot, text);
                icon = &panel->icon[slot];
                icon->y = y;
                icon->x = x - Half(((ArcSprite *)panel->icon[slot].sprite)->w) - 1;
                IconItem_Draw(&panel->icon[slot], ot);
                break;
            }
            case 1: {
                ArcPowerGraph *graph = &panel->graph[slot];

                graph->x = x;
                graph->y = y;
                PowerGraphView_Draw_OVL2(&panel->graph[slot], (uint *)ot, text);
                break;
            }
            case 2: {
                ArcStatBar *b = panel->bars[slot];

                StatBar_Draw(b, ot, x, y, text, panel->levels[slot][0]);
                StatBar_Draw(b + 1, ot, x, y + 0x10, text, panel->levels[slot][1]);
                StatBar_Draw(b + 2, ot, x, y + 0x20, text, panel->levels[slot][2]);
                break;
            }
            case 3:
                ColorChips_Draw(&panel->chips[slot], ot, x, y);
                break;
            case 4:
                SpecTable_Draw(&panel->table[slot], ot, x, y, text);
                break;
            }
        }
    }
}
#else
INCLUDE_ASM("asm/sim/ovl2/nonmatchings/carspecpanel2", CarSpecPanel_Draw);
#endif
}
