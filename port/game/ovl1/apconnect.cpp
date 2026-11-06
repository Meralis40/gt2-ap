// SPDX-License-Identifier: GPL-2.0-or-later
// The port's own page, not the game's: the Archipelago server, slot and password. It comes up
// before the autoload, because the seed decides which memory card the game reads.
#include "common.h"
#include "gamestate.h"
#include "ap.h"

extern "C" {
extern Font g_OVL1_FontLarge;
extern BaseMenu *g_CurrentMenu;

enum { ROW_SERVER, ROW_SLOT, ROW_PASSWORD, ROW_CONNECT, ROW_OFFLINE, ROW_COUNT };

static const int kRowY = 0xa0;
static const int kRowStep = 0x22;
static const int kLabelX = 0x20;
static const int kValueX = 0x68;
static const unsigned int kTextRgb = 0x505050;
static const unsigned int kSelectedRgb = 0xa376e;
static const unsigned int kErrorRgb = 0x20a50f0;

static int g_ApPage_Row;

static void ApConnect_Select(int row)
{
    g_ApPage_Row = row;
    Ap_PageSelect(row <= ROW_PASSWORD ? row : -1);
}

static void ApConnect_Enter(int back)
{
    Ap_PageOpen();
    ApConnect_Select(ROW_SERVER);
}

static int ApConnect_Update(int active)
{
    PadState *pad = (PadState *)((char *)g_CurrentMenu + 0x1a4);
    int state = Ap_PageState();
    unsigned int pressed = pad->pressed;

    if (!active) {
        return 0;
    }
    if (state == AP_PAGE_CONNECTED) {
        Ap_PageClose();
        return 3;
    }
    if (state == AP_PAGE_CONNECTING) {
        return 0;
    }
    if (pressed & PAD_UP) {
        ApConnect_Select((g_ApPage_Row + ROW_COUNT - 1) % ROW_COUNT);
        Sfx_PlayMenu(2);
    } else if (pressed & PAD_DOWN) {
        ApConnect_Select((g_ApPage_Row + 1) % ROW_COUNT);
        Sfx_PlayMenu(2);
    } else if (pressed & (PAD_START | PAD_CROSS)) {
        Sfx_PlayMenu(1);
        switch (g_ApPage_Row) {
        case ROW_CONNECT:
            Ap_PageConnect();
            break;
        case ROW_OFFLINE:
            Ap_PageClose();
            return 3;
        default:
            ApConnect_Select(g_ApPage_Row + 1);
            break;
        }
    }
    return 0;
}

static void ApConnect_DrawRow(TextCtx *ctx, int row, char *label, char *value)
{
    int y = kRowY + row * kRowStep;

    ctx->rgb = row == g_ApPage_Row ? kSelectedRgb : kTextRgb;
    Text_DrawString(ctx, (signed char *)label, kLabelX, y, 1);
    if (value != 0) {
        Text_DrawString(ctx, (signed char *)value, kValueX, y, 1);
    }
}

static void ApConnect_Draw(DrawLayer *layer)
{
    TextCtx ctx;
    char server[64];
    char slot[64];
    char password[64];
    const char *text;
    int i;

    Text_Init(&ctx, 0x1e);
    ctx.ot = (uint *)layer->ot;
    Text_SetFont(&ctx, &g_OVL1_FontLarge);
    strncpy(server, Ap_PageField(ROW_SERVER), 39);
    server[39] = 0;
    strncpy(slot, Ap_PageField(ROW_SLOT), 39);
    slot[39] = 0;
    text = Ap_PageField(ROW_PASSWORD);
    for (i = 0; text[i] != 0 && i < 39; i++) {
        password[i] = '*';
    }
    password[i] = 0;
    // The cursor on the field being typed in.
    if (g_ApPage_Row == ROW_SERVER) {
        strcat(server, "_");
    } else if (g_ApPage_Row == ROW_SLOT) {
        strcat(slot, "_");
    } else if (g_ApPage_Row == ROW_PASSWORD) {
        strcat(password, "_");
    }
    ApConnect_DrawRow(&ctx, ROW_SERVER, "Server", server);
    ApConnect_DrawRow(&ctx, ROW_SLOT, "Slot", slot);
    ApConnect_DrawRow(&ctx, ROW_PASSWORD, "Password", password);
    ApConnect_DrawRow(&ctx, ROW_CONNECT, "Connect", 0);
    ApConnect_DrawRow(&ctx, ROW_OFFLINE, "Play offline", 0);
    if (Ap_PageState() != AP_PAGE_IDLE) {
        ctx.rgb = Ap_PageState() == AP_PAGE_FAILED ? kErrorRgb : kSelectedRgb;
        Text_DrawCentered(&ctx, (signed char *)Ap_PageStatus(), 0xb0, 0x170, 1);
    }
}

BaseMenuPage g_Page_ApConnect = {ApConnect_Enter, ApConnect_Update, ApConnect_Draw, 0x6e3c10, (char *)"ARCHIPELAGO"};
}
