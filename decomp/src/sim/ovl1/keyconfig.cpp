// OVL1 80019218-8001aad4 (keyconfig): Controller key configuration.
// The functions that don't match yet are the original assembly, with their C++
// under NON_MATCHING.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"
#include "gamestate.h"

// A KeyConfig's byte arrays, as blocks for the struct copies.
struct KeyButtons {
    unsigned char b[11];
};

struct KeyAnalog {
    unsigned char b[4];
};

extern "C" {
extern BaseMenu *g_CurrentMenu;
extern Font g_OVL1_FontMedium;
extern Font g_OVL1_FontSmall;
extern KeyConfig g_KeyConfig;
extern KeyConfig D_800B13F0;
extern short g_KeyConfig_RowSlot[];
extern signed char *g_KeyConfig_RowNames[];
extern unsigned char g_KeyConfig_Presets[][4];
extern KeyButtons D_80091570;
extern KeyButtons D_8009157C;
extern KeyButtons D_80091588;
extern SpriteDesc D_8004C144[];
extern SpriteDesc D_8004C1D4[];
extern SpriteDesc D_8004C24C[];
extern SpriteDesc D_8004C2AC;
extern SpriteDesc D_8004C2B8;
extern SpriteDesc D_8004C2C4;
extern SpriteDesc D_8004C2D0;
extern short D_8004C2DC[];
extern short D_8004C2FC[];
extern unsigned char D_8004C0D0[];
extern unsigned char D_8004C0D4[];
extern unsigned char D_8004C0D8[];
extern signed char s_Exit[];
extern signed char s_Default[];
extern signed char s_HoldStartToAssignDPad[];
int KeyConfig_ToNeGconCode(int code);
int KeyConfig_FromNeGconCode(int code);
void KeyConfig_SetDefaults(KeyConfig *cfg);

// The digital pad's codes 9, 10 and 4 as NeGcon codes.
int KeyConfig_ToNeGconCode(int code)
{
    if (code == 9) {
        return 0x81;
    }
    if (code == 10) {
        return 0x82;
    }
    if (code == 4) {
        return 0x83;
    }
    return code;
}

// The other way.
int KeyConfig_FromNeGconCode(int code)
{
    if (code == 0x81) {
        return 9;
    }
    if (code == 0x82) {
        return 10;
    }
    if (code == 0x83) {
        return 4;
    }
    return code;
}

// Analog stick codes 0xa0..0xc3 to icon index.
int KeyConfig_AnalogCodeToIcon(int code)
{
    if (code == 0xc3) {
        return 3;
    }
    if (code == 0xa3) {
        return 2;
    }
    if (code == 0xc2) {
        return 1;
    }
    if (code == 0xa2) {
        return 0;
    }
    if (code == 0xc1) {
        return 7;
    }
    if (code == 0xa1) {
        return 6;
    }
    if (code != 0xc0) {
        return (unsigned int)(code == 0xa0) << 2;
    }
    return 5;
}

// The icon of a NeGcon code; 0 for the buttons that have none.
SpriteDesc *KeyConfig_GetNeGconIcon(int code)
{
    code = KeyConfig_FromNeGconCode(code);
    if (((unsigned int)(code - 9) < 2) | (code == 4)) {
        return &D_8004C1D4[D_8004C2FC[code]];
    }
    return 0;
}

// padType = 3 (no controller).
void KeyConfig_Clear(KeyConfig *cfg)
{
    cfg->padType = 3;
}

#ifdef NON_MATCHING
// Copies the saved maps of the pad's kind: 0 digital, 1 analog, 2 NeGcon, 3 none.
void KeyConfig_Load(KeyConfig *cfg, PadDevice *pad, int port)
{
    int padType = 3;
    int kind = 0;
    GameState *gs = &g_GameState;
    PortConfig *saved = &gs->ports[port];

    switch (pad->type) {
    case 4:
        padType = 0;
        break;
    case 2:
        padType = 2;
        kind = 2;
        break;
    case 5:
    case 7:
        padType = 1;
        kind = 1;
        break;
    }
    cfg->padType = padType;
    if (padType != 3) {
        *(KeyButtons *)cfg->buttons = *(KeyButtons *)saved->buttonMaps[kind];
        *(KeyAnalog *)cfg->analogMap = *(KeyAnalog *)saved->analogMaps[kind];
    }
}
#else
INCLUDE_ASM("asm/sim/ovl1/nonmatchings/keyconfig", KeyConfig_Load);
#endif

#ifdef NON_MATCHING
// Saves the maps of the pad's kind.
void KeyConfig_Store(KeyConfig *cfg, int port)
{
    int padType = 3;
    int kind = 0;
    GameState *gs = &g_GameState;
    PortConfig *saved = &gs->ports[port];

    switch (cfg->padType) {
    case 0:
        padType = 0;
        break;
    case 2:
        padType = 2;
        kind = 2;
        break;
    case 1:
        padType = 1;
        kind = 1;
        break;
    }
    if (padType != 3) {
        *(KeyButtons *)saved->buttonMaps[kind] = *(KeyButtons *)cfg->buttons;
        *(KeyAnalog *)saved->analogMaps[kind] = *(KeyAnalog *)cfg->analogMap;
    }
}
#else
INCLUDE_ASM("asm/sim/ovl1/nonmatchings/keyconfig", KeyConfig_Store);
#endif

#ifdef NON_MATCHING
// The slot a code is assigned to, or -1. Slots 2 and 3 also hold it through the
// NeGcon codes, and through analogMap when the analog sticks take them.
int KeyConfig_FindButton(KeyConfig *cfg, int code)
{
    int i = 4;
    unsigned char *b = cfg->buttons;

    for (;;) {
        if (i >= 10) {
            break;
        }
        if (b[i] == code) {
            return i;
        }
        i++;
    }
    switch (cfg->padType) {
    case 0:
        if (cfg->buttons[2] == code) {
            return 2;
        }
        break;
    case 2:
        if (cfg->buttons[2] == code) {
            return 2;
        }
        if (cfg->buttons[3] == code) {
            return 3;
        }
        code = KeyConfig_ToNeGconCode(code);
        if (cfg->buttons[2] == code) {
            return 2;
        }
        break;
    case 1:
        if (cfg->buttons[2] == code) {
            return 2;
        }
        if (cfg->buttons[3] == code) {
            return 3;
        }
        if ((signed char)cfg->buttons[2] < 0 && cfg->analogMap[2] == code) {
            return 2;
        }
        if ((signed char)cfg->buttons[3] < 0 && cfg->analogMap[3] == code) {
            return 3;
        }
        return -1;
    default:
        return -1;
    }
    if (cfg->buttons[3] == code) {
        return 3;
    }
    return -1;
}
#else
INCLUDE_ASM("asm/sim/ovl1/nonmatchings/keyconfig", KeyConfig_FindButton);
#endif

#ifdef NON_MATCHING
// Assigns a code to a slot, the slot that had it taking the old code.
void KeyConfig_Assign(KeyConfig *cfg, int slot, int code)
{
    int found = KeyConfig_FindButton(cfg, code);
    int old;
    unsigned char *b;

    switch (cfg->padType) {
    case 0:
        if (found >= 0) {
            cfg->buttons[found] = cfg->buttons[slot];
        }
        cfg->buttons[slot] = code;
        break;
    case 2:
        if (found >= 0) {
            if (found < 4 && found >= 2) {
                b = cfg->buttons;
                old = b[slot];
                b += found;
                *b = KeyConfig_ToNeGconCode(old);
            } else if (found >= 0) {
                b = cfg->buttons;
                old = b[slot];
                b += found;
                *b = KeyConfig_FromNeGconCode(old);
            }
        }
        if (slot < 4 && slot >= 2) {
            b = cfg->buttons;
            b += slot;
            *b = KeyConfig_ToNeGconCode(code);
        } else {
            cfg->buttons[slot] = code;
        }
        break;
    case 1:
        if (found >= 0) {
            old = cfg->buttons[slot];
            switch (slot) {
            case 2:
                if ((signed char)cfg->buttons[2] < 0) {
                    old = cfg->analogMap[2];
                }
                break;
            case 3:
                if ((signed char)cfg->buttons[3] < 0) {
                    old = cfg->analogMap[3];
                }
                break;
            }
            switch (found) {
            case 2:
                if ((signed char)cfg->buttons[2] < 0) {
                    cfg->analogMap[2] = old;
                } else {
                    cfg->buttons[2] = old;
                }
                break;
            case 3:
                if ((signed char)cfg->buttons[3] < 0) {
                    cfg->analogMap[3] = old;
                } else {
                    cfg->buttons[3] = old;
                }
                break;
            default:
                if (found >= 0) {
                    cfg->buttons[found] = old;
                }
                break;
            }
        }
        switch (slot) {
        case 2:
            if ((signed char)cfg->buttons[3] < 0) {
                cfg->buttons[3] = cfg->analogMap[3];
                cfg->analogMap[1] = 0;
            }
            cfg->buttons[2] = code;
            break;
        case 3:
            if ((signed char)cfg->buttons[2] < 0) {
                cfg->buttons[2] = cfg->analogMap[2];
                cfg->analogMap[1] = 0;
            }
            cfg->buttons[3] = code;
            break;
        default:
            cfg->buttons[slot] = code;
            break;
        }
        break;
    }
}
#else
INCLUDE_ASM("asm/sim/ovl1/nonmatchings/keyconfig", KeyConfig_Assign);
#endif

#ifdef NON_MATCHING
// Row 0: Left and Right change the layout. Other buttons buzz. 1 when nothing changed.
int KeyConfig_ChangeLayout(KeyConfig *cfg, PadState *pad)
{
    int result = 1;
    unsigned int pressed = pad->pressed;
    int layout;

    if ((pad->held & 0x10000) != 0) {
        if ((pressed & ~0x10000) != 0) {
            Sfx_PlayMenu(0);
            return 0;
        }
        return 0;
    }
    switch (cfg->padType) {
    case 0:
        if ((pressed & ~3) != 0) {
            Sfx_PlayMenu(0);
            result = 0;
        }
        return result;
    case 2:
        if ((pressed & ~0xf) != 0) {
            goto buzz;
        }
        layout = cfg->analogMap[0];
        if ((pressed & 4) != 0) {
            layout--;
            if (layout < 0) {
                layout = 1;
            }
        }
        if ((pressed & 8) != 0) {
            layout++;
            if (layout >= 2) {
                layout = 0;
            }
        }
        if (layout == cfg->analogMap[0]) {
            return result;
        }
        result = 0;
        Sfx_PlayMenu(5);
        cfg->analogMap[0] = layout;
        if (layout != 0) {
            cfg->buttons[0] = 2;
            cfg->buttons[1] = 3;
        } else {
            cfg->buttons[0] = 0x80;
            cfg->buttons[1] = 0x80;
        }
        return result;
    case 1:
        if ((pressed & ~0xf) != 0) {
        buzz:
            Sfx_PlayMenu(0);
            return 0;
        }
        layout = cfg->analogMap[0];
        if ((pressed & 4) != 0) {
            layout--;
            if (layout < 0) {
                layout = 2;
            }
        }
        if ((pressed & 8) != 0) {
            layout++;
            if (layout >= 3) {
                layout = 0;
            }
        }
        if (layout == cfg->analogMap[0]) {
            return result;
        }
        result = 0;
        Sfx_PlayMenu(5);
        cfg->analogMap[0] = layout;
        switch (layout) {
        case 0:
            cfg->buttons[0] = 0x82;
            cfg->buttons[1] = 0x82;
            break;
        case 1:
            cfg->buttons[0] = 0x80;
            cfg->buttons[1] = 0x80;
            break;
        default:
            cfg->buttons[0] = 2;
            cfg->buttons[1] = 3;
            break;
        }
        return result;
    }
    return result;
}
#else
INCLUDE_ASM("asm/sim/ovl1/nonmatchings/keyconfig", KeyConfig_ChangeLayout);
#endif

#ifdef NON_MATCHING
// Analog pads, rows of slots 2 and 3: Left and Right pick a preset of the two slots.
// Preset 0 is what they held before. 0 when it changed.
int KeyConfig_ChangePreset(KeyConfig *cfg, PadState *pad)
{
    int result = 1;
    unsigned int pressed = pad->pressed;
    int preset;
    unsigned char *map;

    if ((pad->held & 0x10000) != 0 || cfg->padType != 1 || (pressed & 0xc) == 0) {
        return result;
    }
    preset = cfg->analogMap[1];
    if ((pressed & 4) != 0) {
        preset--;
        if (preset < 0) {
            preset = 8;
        }
    }
    map = cfg->analogMap;
    if ((pressed & 8) != 0) {
        preset++;
        if (preset >= 9) {
            preset = 0;
        }
    }
    if (preset == map[1]) {
        return result;
    }
    Sfx_PlayMenu(5);
    result = 0;
    if (map[1] == 0) {
        map[2] = cfg->buttons[2];
        map[3] = cfg->buttons[3];
    }
    map[1] = preset;
    if (preset == 0) {
        cfg->buttons[2] = map[2];
        cfg->buttons[3] = map[3];
    } else {
        cfg->buttons[2] = g_KeyConfig_Presets[preset][0];
        cfg->buttons[3] = g_KeyConfig_Presets[preset][2];
    }
    return result;
}
#else
INCLUDE_ASM("asm/sim/ovl1/nonmatchings/keyconfig", KeyConfig_ChangePreset);
#endif

#ifdef NON_MATCHING
// The default maps of the pad's kind.
void KeyConfig_SetDefaults(KeyConfig *cfg)
{
    switch (cfg->padType) {
    case 0:
        *(KeyButtons *)cfg->buttons = D_80091570;
        break;
    case 2:
        *(KeyButtons *)cfg->buttons = D_80091588;
        cfg->analogMap[0] = 0;
        break;
    case 1:
        *(KeyButtons *)cfg->buttons = D_8009157C;
        cfg->analogMap[0] = 0;
        cfg->analogMap[1] = 0;
        break;
    }
}
#else
INCLUDE_ASM("asm/sim/ovl1/nonmatchings/keyconfig", KeyConfig_SetDefaults);
#endif

#ifdef NON_MATCHING
// A row's input: 2 on Exit, 1 when something changed, else 0. With the button of bit
// 16 held, the first four buttons pick codes 0 to 3.
int KeyConfig_HandleInput(KeyConfig *cfg, PadState *pad, int row)
{
    int code = -1;
    int slot = g_KeyConfig_RowSlot[row];
    unsigned int pressed = pad->pressed;
    int result = 0;

    switch (slot) {
    case -2:
        if ((pressed & 0xa00) != 0) {
            return 2;
        }
        goto done;
    case -1:
        if ((pressed & 0xa00) == 0) {
            return result;
        }
        KeyConfig_SetDefaults(cfg);
        Sfx_PlayMenu(1);
        return 1;
    case 0:
        if (KeyConfig_ChangeLayout(cfg, pad) != 0) {
            return result;
        }
        result = 1;
        goto done;
    }
    if (cfg->padType == 1 && slot < 4 && slot >= 2) {
        if (KeyConfig_ChangePreset(cfg, pad) == 0) {
            return 1;
        }
    }
    if ((pad->held & 0x10000) != 0) {
        if ((pressed & 1) != 0) {
            code = 0;
        }
        if ((pressed & 2) != 0) {
            code = 1;
        }
        if ((pressed & 4) != 0) {
            code = 2;
        }
        if ((pressed & 8) != 0) {
            code = 3;
        }
    } else {
        if ((pressed & 0x10) != 0) {
            code = 4;
        }
        if ((pressed & 0x20) != 0) {
            code = 5;
        }
        if ((pressed & 0x1000) != 0) {
            code = 0xc;
        }
        if ((pressed & 0x2000) != 0) {
            code = 0xd;
        }
        if ((pressed & 0x200) != 0) {
            code = 9;
        }
        if ((pressed & 0x100) != 0) {
            code = 8;
        }
        if ((pressed & 0x400) != 0) {
            code = 0xa;
        }
        if ((pressed & 0x800) != 0) {
            code = 0xb;
        }
    }
    if (code < 0) {
        return result;
    }
    Sfx_PlayMenu(1);
    KeyConfig_Assign(cfg, slot, code);
    result = 1;
done:
    return result;
}
#else
INCLUDE_ASM("asm/sim/ovl1/nonmatchings/keyconfig", KeyConfig_HandleInput);
#endif

#ifdef NON_MATCHING
static inline void KeyConfig_DrawIcon(void *ot, SpriteDesc *spr, int x, int y, int b)
{
    PrimSprite *p = Prim_AddSprite(ot, b | b << 8 | b << 16);

    *(int *)p = (x - (((short *)&spr->wh)[0] >> 1)) + ((y - (((short *)&spr->wh)[1] >> 1)) << 16);
    ((unsigned int *)p)[1] = spr->uvClut;
    ((unsigned int *)p)[2] = spr->wh;
}

// A row's cell for one pad: the layout, a button's icon, or Default and Exit, with
// side arrows on the cursor's row.
void KeyConfig_DrawCell(KeyConfig *cfg, void *ot, int x, int y, int brightness, int row, int cursor, int blink)
{
    TextCtx ctx;
    char arrow[8];
    TextCtx *c;
    SpriteDesc *spr = &D_8004C2AC;
    int scale = 0x30;
    int code;
    int ay;

    Text_Init(&ctx, 0x1e);
    c = &ctx;
    c->ot = (uint *)ot;
    *(uint *)&c->clut = (*(uint *)&c->clut & 0xff9fffff) | 0x200000;
    Text_SetFont(c, &g_OVL1_FontMedium);
    c->rgb = 0x142864;
    if (cursor == row) {
        scale = 0x80;
    }
    if (cursor < 0) {
        scale = 0x80;
    }
    brightness = (brightness * scale) >> 7;
    if (cfg->padType == 3) {
        return;
    }
    switch (row) {
    case 9:
        if (cursor < 0) {
            return;
        }
        Text_SetFont(c, &g_OVL1_FontSmall);
        c->rgb = Rgb_Lerp(D_8004C0D0, D_8004C0D8, brightness, 0x80);
        Text_DrawCentered(c, s_Default, x, y + 6, 1);
        return;
    case 10:
        if (cursor < 0) {
            return;
        }
        Text_SetFont(c, &g_OVL1_FontSmall);
        c->rgb = Rgb_Lerp(D_8004C0D0, D_8004C0D8, brightness, 0x80);
        Text_DrawCentered(c, s_Exit, x, y, 1);
        return;
    case 0:
        switch (cfg->padType) {
        case 0:
            KeyConfig_DrawIcon(ot, spr, x, y, brightness);
            Prim_AddDrawMode(ot, spr->tpage | 0x20);
            return;
        case 2:
            if (cfg->analogMap[0] == 0) {
                spr = &D_8004C2D0;
            }
            break;
        case 1:
            if (cfg->analogMap[0] == 0) {
                spr = &D_8004C2B8;
            } else if (cfg->analogMap[0] == 1) {
                spr = &D_8004C2C4;
            }
            break;
        default:
            return;
        }
        KeyConfig_DrawIcon(ot, spr, x, y, brightness);
        Prim_AddDrawMode(ot, spr->tpage | 0x20);
        if (cursor == 0) {
            Menu_DrawSideArrow(arrow, (uint *)ot, x + 0x10, y, 6, 10, blink);
            Menu_DrawSideArrow(arrow, (uint *)ot, x - 0x10, y, -6, 10, blink);
            Prim_AddDrawMode(ot, 0x20);
        }
        return;
    }
    code = 0;
    switch (row) {
    case 1:
        code = cfg->buttons[2];
        break;
    case 2:
        code = cfg->buttons[3];
        break;
    case 3:
        code = cfg->buttons[5];
        break;
    case 4:
        code = cfg->buttons[4];
        break;
    case 5:
        code = cfg->buttons[6];
        break;
    case 6:
        code = cfg->buttons[7];
        break;
    case 7:
        code = cfg->buttons[9];
        break;
    case 8:
        code = cfg->buttons[8];
        break;
    }
    switch (cfg->padType) {
    case 0:
        spr = &D_8004C144[D_8004C2DC[code]];
        break;
    case 1:
        if (code >= 0x80) {
            spr = &D_8004C24C[KeyConfig_AnalogCodeToIcon(code)];
        } else {
            spr = &D_8004C144[D_8004C2DC[code]];
        }
        if (((unsigned int)(cursor - 1) < 2) & (row == 1)) {
            ay = y + 0xd;
            Menu_DrawSideArrow(arrow, (uint *)ot, x + 0x10, ay, 6, 10, blink);
            Menu_DrawSideArrow(arrow, (uint *)ot, x - 0x10, ay, -6, 10, blink);
            Prim_AddDrawMode(ot, 0x20);
        }
        break;
    case 2:
        spr = &D_8004C1D4[D_8004C2FC[KeyConfig_FromNeGconCode(code)]];
        break;
    }
    KeyConfig_DrawIcon(ot, spr, x, y, brightness);
    Prim_AddDrawMode(ot, spr->tpage | 0x20);
}
#else
INCLUDE_ASM("asm/sim/ovl1/nonmatchings/keyconfig", KeyConfig_DrawCell);
#endif

#ifdef NON_MATCHING
// The 11 rows: each name with both pads' cells; the hint under them while a cursor is
// on the table.
void KeyConfig_DrawTable(unsigned int *ot, int x, int y, int brightness, int row1, int row2, int blink)
{
    TextCtx ctx;
    TextCtx *c;
    int rowY = y;
    int i;
    int cellY;
    signed char **name;

    Text_Init(&ctx, 0x1e);
    c = &ctx;
    *(uint *)&c->clut = (*(uint *)&c->clut & 0xff9fffff) | 0x200000;
    c->ot = ot;
    Text_SetFont(c, &g_OVL1_FontMedium);
    name = g_KeyConfig_RowNames;
    for (i = 0; i < 11; i++) {
        c->rgb = Rgb_Lerp(D_8004C0D0, D_8004C0D4, brightness, 0x80);
        cellY = rowY - 10;
        Text_DrawCentered(c, *name++, x, rowY, 1);
        rowY += 0x1a;
        KeyConfig_DrawCell(&g_KeyConfig, ot, x - 0x50, cellY, brightness, i, row1, blink);
        KeyConfig_DrawCell(&D_800B13F0, ot, x + 0x50, cellY, brightness, i, row2, blink);
    }
    if (row1 >= 0) {
        Text_SetFont(&ctx, &g_OVL1_FontSmall);
        ctx.rgb = 0xb3060;
        Text_DrawCentered(&ctx, s_HoldStartToAssignDPad, x, y - 0x18, 1);
    }
}
#else
INCLUDE_ASM("asm/sim/ovl1/nonmatchings/keyconfig", KeyConfig_DrawTable);
#endif

// No cursors, no pads.
void KeyConfigPage_Init(KeyConfigPage *page)
{
    int none = -1;

    page->row[0] = none;
    page->row[1] = none;
    page->blink = 0;
    KeyConfig_Clear(&g_KeyConfig);
    KeyConfig_Clear(&D_800B13F0);
    page->delay = none;
}

// Both cursors on Exit (row 10), 4-frame input delay.
void KeyConfigPage_Open(KeyConfigPage *page)
{
    page->row[1] = 10;
    page->row[0] = 10;
    page->delay = 4;
}

void KeyConfigPage_Close(KeyConfigPage *page)
{
    page->row[1] = -1;
    page->row[0] = -1;
    page->delay = -1;
}

#ifdef NON_MATCHING
// Reads both pads' maps, handles each pad's row, and moves the cursors (Up and Down):
// 0 when a pad chose Exit, else -1.
int KeyConfigPage_Update(KeyConfigPage *page, PadState *pad)
{
    BaseMenu *menu = g_CurrentMenu;
    KeyConfig *cfg0;
    KeyConfig *cfg1;
    int r1 = 1;
    int r2 = 1;
    unsigned int buttons;
    int row;

    if (page->delay > 0) {
        pad = 0;
        page->delay--;
    }
    if (++page->blink >= 0x2d) {
        page->blink = 0;
    }
    cfg0 = &g_KeyConfig;
    KeyConfig_Load(cfg0, &menu->pads[0], 0);
    cfg1 = &D_800B13F0;
    KeyConfig_Load(cfg1, &menu->pads[1], 1);
    if (pad != 0) {
        r1 = KeyConfig_HandleInput(cfg0, &menu->padState[0], page->row[0]);
        r2 = KeyConfig_HandleInput(cfg1, &menu->padState[1], page->row[1]);
    }
    KeyConfig_Store(cfg0, 0);
    KeyConfig_Store(cfg1, 1);
    if ((r1 == 2) | (r2 == 2)) {
        return 0;
    }
    if ((pad != 0) & (r1 == 0)) {
        buttons = pad->pressed | pad->repeat;
        row = page->row[0];
        if ((buttons & 1) != 0) {
            row--;
            if (row < 0) {
                row = 10;
            }
        }
        if ((buttons & 2) != 0) {
            row++;
            if (row >= 11) {
                row = 0;
            }
        }
        if (row != page->row[0]) {
            page->row[0] = row;
            Sfx_PlayMenu(6);
        }
    }
    if ((pad != 0) & (r2 == 0)) {
        buttons = menu->padState[1].pressed | menu->padState[1].repeat;
        row = page->row[1];
        if ((buttons & 1) != 0) {
            row--;
            if (row < 0) {
                row = 10;
            }
        }
        if ((buttons & 2) != 0) {
            row++;
            if (row >= 11) {
                row = 0;
            }
        }
        if (row != page->row[1]) {
            page->row[1] = row;
            Sfx_PlayMenu(6);
        }
    }
    return -1;
}
#else
INCLUDE_ASM("asm/sim/ovl1/nonmatchings/keyconfig", KeyConfigPage_Update);
#endif

// KeyConfig_DrawTable.
void KeyConfigPage_Draw(KeyConfigPage *page, unsigned int *ot, int x, int y, int brightness)
{
    KeyConfig_DrawTable(ot, x, y, brightness, (int)page->row[0], (int)page->row[1], (int)page->blink);
}
}
