// OVL2 80019064-800194c0 (carspecpanel): Car spec panel: opening, closing and colour.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {

extern unsigned char D_8004F634[]; // name text template
extern unsigned char D_8004F650[]; // icon template
extern unsigned char D_8004F65C[]; // power graph template
extern char D_80027108[];
extern unsigned char D_800454E0[]; // icon sprite
extern unsigned char D_800F8634[]; // stat bar templates (at 0, 0x13 and 0x1e)

static inline void TextItem_Reset(ArcTextItem *item)
{
    item->anim = -1;
}

static inline void IconItem_Reset(ArcIconItem *it)
{
    it->anim = -1;
}

// Initialises the widgets of both slots, hidden.
// The game strength-reduces the loop into other induction variables (offsets
// added to the panel, one for both curves).
#ifdef NON_MATCHING
void CarSpecPanel_Init(ArcCarSpecPanel *panel)
{
    int i;

    for (i = 0; i < 2; i++) {
        ArcStatBar *b;

        panel->shown[i] = 0;
        TextItem_Init_OVL2(&panel->name[i], D_8004F634);
        panel->name[i].text = D_80027108;
        IconItem_Init(&panel->icon[i], (ArcIconItemTmpl *)D_8004F650);
        panel->icon[i].sprite = D_800454E0;
        PowerGraphView_Init_OVL2(&panel->graph[i], D_8004F65C);
        panel->graph[i].powerCurve = panel->powerCurve[i];
        panel->graph[i].torqueCurve = panel->torqueCurve[i];
        b = panel->bars[i];
        StatBar_Init(b, D_800F8634);
        StatBar_Init(b + 1, D_800F8634 + 0x13);
        StatBar_Init(b + 2, D_800F8634 + 0x1e);
        ColorChips_Init(&panel->chips[i]);
        SpecTable_Init(&panel->table[i]);
    }
    panel->slot = 0;
}
#else
INCLUDE_ASM("asm/sim/ovl2/nonmatchings/carspecpanel", CarSpecPanel_Init);
#endif

// Starts the close animation of the current slot's widgets, if shown.
void CarSpecPanel_Close(ArcCarSpecPanel *panel)
{
    int slot = panel->slot;

    if (panel->shown[slot]) {
        ArcStatBar *b;

        TextItem_Hide_OVL2(&panel->name[slot]);
        IconItem_Hide(&panel->icon[slot]);
        PowerGraphView_Close_OVL2(&panel->graph[slot]);
        b = panel->bars[slot];
        StatBar_Hide(b);
        StatBar_Hide(b + 1);
        StatBar_Hide(b + 2);
        ColorChips_Close(&panel->chips[slot]);
        SpecTable_Close(&panel->table[slot]);
    }
}

// Highlights a colour chip of the current slot.
void CarSpecPanel_SelectColor(ArcCarSpecPanel *panel, int index)
{
    ColorChips_Select(&panel->chips[panel->slot], index);
}

// Closes the current slot and switches to the other one, reset and hidden for the next
// CarSpecPanel_Show.
void CarSpecPanel_Next(ArcCarSpecPanel *panel)
{
    int slot = panel->slot;

    if (panel->shown[slot]) {
        ArcStatBar *b;

        TextItem_Hide_OVL2(&panel->name[slot]);
        IconItem_Hide(&panel->icon[slot]);
        PowerGraphView_Close_OVL2(&panel->graph[slot]);
        b = panel->bars[slot];
        StatBar_Hide(b);
        StatBar_Hide(b + 1);
        StatBar_Hide(b + 2);
        ColorChips_Close(&panel->chips[slot]);
        SpecTable_Close(&panel->table[slot]);
        panel->shown[slot] = 0;
    }
    slot++;
    if (slot > 1) {
        slot = 0;
    }
    panel->slot = slot;
    TextItem_Reset(&panel->name[slot]);
    IconItem_Reset(&panel->icon[slot]);
    SpecTable_Reset(&panel->table[slot]);
    panel->shown[slot] = 0;
}
}
