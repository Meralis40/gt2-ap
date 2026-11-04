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
extern RaceSetup g_RaceSetup;
extern RaceMem g_Race_IsDemo;
extern unsigned char g_HudTexFile[];
void *memcpy(void *dst, const void *src, unsigned int n);
void *memset(void *p, int c, unsigned int n);

// Reads TEXT_DATA_RACE_TXD and copies the block of g_GameState.language to the race
// text. Called first by Race_Entry.
void RaceText_Load(void)
{
    RaceText texts[8];

    VolId_ReadFile(VOLID_TEXT_DATA_RACE_TXD, texts);
    sR_Lap = texts[g_GameState.language];
}

#ifdef NON_MATCHING
// Loads each car's model and textures, with the course's lighting blended into its
// palettes (slots 0-2; slot 3 is black), and its wheel.
void RaceCars_LoadModels(RaceWorld *world)
{
    CourseEntry *course = Course_Get(world->course);
    unsigned int rgb[4];
    int t[4];
    unsigned int *rgbs = rgb;
    int *ts = t;
    int i;

    rgb[0] = Rgb555ToRgb24(*(unsigned short *)((char *)course + 0xc));
    rgb[1] = Rgb555ToRgb24(*(unsigned short *)((char *)course + 0x14));
    rgb[2] = Rgb555ToRgb24(*(unsigned short *)((char *)course + 0x10));
    rgb[3] = 0;
    t[0] = *(unsigned short *)((char *)course + 0xe);
    t[1] = *(unsigned short *)((char *)course + 0x16);
    t[2] = *(unsigned short *)((char *)course + 0x12);
    t[3] = 0x1000;
    for (i = 0; i < g_RaceWorld.carCount; i++) {
        RaceCar *car = &g_RaceSetup.cars[i];
        int color = car->colorCode;
        CarAsset *asset = CarAsset_Find(car->carId);
        CarModelHeader *model = (CarModelHeader *)((char *)&g_Race_IsDemo + 0xb6394 + i * 0x5000);
        int slot = 0x1a - i;
        int file;
        unsigned short *clut = (unsigned short *)(g_HudTexFile + 0xb3a0);
        unsigned short *src;
        unsigned int *r = rgbs;
        int *u = ts;
        int j;

        file = asset->file + (*(unsigned short *)((char *)course + 8) & 1) * 2;
        world->carModels[i] = model;
        CarModel_Load(file, model, slot);
        CarModel_ReadTim(file, g_HudTexFile);
        Vram_UploadCarSlot(g_HudTexFile + 0x43a0, slot);
        src = (unsigned short *)(g_HudTexFile + CarTex_FindColorIndex((CarTex *)g_HudTexFile, color) * 0x240 + 0x20);
        if (Wheel_LoadTim(car->setup.wheel, clut + 0x100) != 0) {
            memcpy(src, (char *)clut + 0x214, 0x20);
            Tim_LoadToSlot((char *)(clut + 0x100), slot, 0);
        }
        for (j = 0; j < 4; j++) {
            if (j < 3) {
                Clut_Blend(clut, src, *r, *u);
            } else {
                memset(clut, 0, 0x200);
            }
            Clut_UploadToSlot(clut, slot, j);
            CarModel_SetColor(model, j, clut[0]);
            r++;
            u++;
        }
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/raceassets", RaceCars_LoadModels);
#endif

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
