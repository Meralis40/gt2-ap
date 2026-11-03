// OVL0 80028cc0-800292a0 (raceassets): Race text, car models and the course map.
// The functions that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"
#include "gamestate.h"

// One language's race text (TEXT_DATA_RACE_TXD holds one per language), copied as one
// block.
struct RaceText {
    char data[0x1915];
};

// A car's dot on the course map; x is -1 when the car isn't shown.
struct MapDot {
    int x;
    int y;
};

extern "C" {
extern RaceText sR_Lap;             // the race text of the current language (its first string is "Lap")
extern RaceWorld g_RaceWorld;

// Reads TEXT_DATA_RACE_TXD and copies the block of g_GameState.language to the race
// text. Called first by Race_Entry.
void RaceText_Load(void)
{
    RaceText texts[8];

    VolId_ReadFile(VOLID_TEXT_DATA_RACE_TXD, texts);
    sR_Lap = texts[g_GameState.language];
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceassets", RaceCars_LoadModels);

// When the map option is on: a dot per car (red for player, green for the others) with a
// black border, over the 96x96 map texture at x, y.
#ifdef NON_MATCHING
// The second loop's dot pointer and x swap s0 and s1, and the pointers are set before
// the loop invariants instead of after them (the original reduces dots[i]).
void Hud_DrawCourseMap(void *world, void *ot, int x, int y, int player)
{
    MapDot dots[6];
    MapDot *tile;
    MapDot *border;
    RaceVehicle *v;
    TexQuad *q;
    unsigned int rgb;
    int i;

    if (g_GameState.options2[3]) {
        i = 0;
        while (1) {
            if (i >= g_RaceWorld.carCount) {
                break;
            }
            v = &((RaceWorld *)world)->vehicles[i];
            dots[i].x = -1;
            dots[i].y = -1;
            if (v->present) {
                int mx = ((v->world.t[0] >> 16) * 0xa3 >> 12) + 0x30;
                dots[i].x = x + mx;
                int my = ((v->world.t[2] >> 16) * 0xa3 >> 12) + 0x30;
                dots[i].y = y + my;
            }
            i++;
        }
        i = 0;
        tile = dots;
        while (1) {
            int tx, ty;

            if (i >= g_RaceWorld.carCount) {
                break;
            }
            tx = tile->x;
            ty = tile->y;
            if (tx >= 0) {
                rgb = 0xff00;
                if (i == player) {
                    rgb = 0xff;
                }
                RECT *r = Prim_AddTile(ot, rgb);
                r->x = tx - 1;
                r->y = ty - 1;
                r->w = 2;
                r->h = 2;
            }
            tile++;
            i++;
        }
        border = dots;
        i = 0;
        while (1) {
            int bx, by;

            if (i >= g_RaceWorld.carCount) {
                break;
            }
            bx = border->x;
            by = border->y;
            if (bx >= 0) {
                RECT *r = Prim_AddTile(ot, 0);
                r->x = bx - 2;
                r->y = by - 2;
                r->w = 4;
                r->h = 4;
            }
            border++;
            i++;
        }
        q = (TexQuad *)Prim_AddTexQuad((OtTag *)ot, 0x808080);
        q->x = 9;
        q->v = 0x90;
        q->clut = 0x7f17;
        q->texX = x;
        q->y = y;
        q->u = 0;
        q->w = 0x60;
        q->h = 0x60;
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceassets", Hud_DrawCourseMap);
#endif
}
