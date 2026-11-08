// Resident code 800683fc-800686a0 (roundedbox): a box with round ends.
#include "common.h"

extern "C" {

// The box: its colour and code, its centre and its size.
struct RoundedBox {
    unsigned int rgb;
    short x, y;
    short w, h;
};

// Points around a half circle.
struct CirclePoint {
    short x, y;
};

extern CirclePoint (&D_80091A78)[];

// A tile, with a fan of POLY_F4s (three rim points and the end's centre) at each end.
// The game keeps box and ot on the stack, with nine saved registers for the rest, and
// recomputes the table addresses in the loop; GCC hoists them and keeps box in a register.
void Prim_AddRoundedBox(void *ot, void *box, int radius)
{
    RoundedBox *b = (RoundedBox *)box;
    int h = b->h;
    int rx = h * radius;
    int w = b->w;
    int half = w >> 1;
    short x = b->x;
    short cy = b->y;
    short right = x + half;
    short left = x - half;
    RECT *tile = Prim_AddTile(ot, b->rgb);
    int i;

    tile->x = left;
    tile->y = cy - (h >> 1);
    tile->w = b->w;
    tile->h = b->h;
    i = 0;
    do {
        short *p = (short *)Prim_AddPolyF4(ot, b->rgb);
        CirclePoint *c0 = &D_80091A78[i];
        CirclePoint *c1 = &D_80091A78[i + 1];
        CirclePoint *c2 = &D_80091A78[i + 2];

        p[2] = right + (c0->x * rx >> 17);
        p[3] = cy + (c0->y * h >> 12);
        p[4] = right + (c1->x * rx >> 17);
        p[5] = cy + (c1->y * h >> 12);
        p[8] = right + (c2->x * rx >> 17);
        p[6] = right;
        p[7] = cy;
        p[9] = cy + (c2->y * h >> 12);

        p = (short *)Prim_AddPolyF4(ot, b->rgb);
        p[8] = left - (c0->x * rx >> 17);
        p[9] = cy + (c0->y * h >> 12);
        p[4] = left - (c1->x * rx >> 17);
        p[5] = cy + (c1->y * h >> 12);
        p[2] = left - (c2->x * rx >> 17);
        p[6] = left;
        p[7] = cy;
        p[3] = cy + (c2->y * h >> 12);
        i += 2;
    } while (i < 12);
}

}
