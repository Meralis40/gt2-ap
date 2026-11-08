// Resident code 8006c274-8006c460 (grounddisc): a disc drawn flat on the ground.
// OVL2 builds it too.
#include "gptr.h"
#include "common.h"

extern "C" {
extern short (&g_SinTable)[];

// Puts the rim points on a circle of radius 0x40000 and the centre at 0.
void GroundDisc_Init(GroundDisc *disc, int unused)
{
    int radius = 0x400;
    int x;
    int z;
    int angle;
    int i;

    for (i = 0; i < 25; i++) {
        angle = (i << 12) / 24 & 0xfff;
        x = radius * g_SinTable[angle + 0x400];
        z = radius * g_SinTable[angle];
        disc->rim[i].y = 0;
        disc->rim[i].x = (x >> 12) << 8;
        disc->rim[i].z = (z >> 12) << 8;
    }
    disc->centre.z = 0;
    disc->centre.y = 0;
    disc->centre.x = 0;
}

// Draws the disc as Gouraud triangles from the centre's rgb to black, semi-transparent
// if blend is set, each at the deepest of its three points.
void GroundDisc_Draw(GroundDisc *disc, uint *ot, uchar *camera, int blend, uint rgb)
{
    ScreenPoint *pts = (ScreenPoint *)rt::from_guest(0x1f800000);
    ScreenPoint *a = pts;
    ScreenPoint *centre = &pts[25];
    uint code = 0x30;
    int i;
    int z;
    uint *entry;
    PrimG3 *p;

    if (blend) {
        code = 0x32;
    }
    Points_Project(camera + 8, pts, disc, 26);
    for (i = 0; i < 24; i++) {
        z = centre->z;
        if (z < a[0].z) {
            z = a[0].z;
        }
        if (z < a[1].z) {
            z = a[1].z;
        }
        entry = ot + z;
        p = Prim_AddPolyG3(entry);
        p->c0 = code << 24 | rgb;
        p->x0 = centre->x;
        p->y0 = centre->y;
        p->c1 = 0;
        p->x1 = a->x;
        p->y1 = a->y;
        a++;
        p->c2 = 0;
        p->x2 = a->x;
        p->y2 = a->y;
        Prim_AddDrawMode(entry, 0x200);
    }
}
}
