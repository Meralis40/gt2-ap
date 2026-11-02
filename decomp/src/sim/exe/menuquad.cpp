// Resident code 8006b61c-8006b77c (menuquad): gradient quads. OVL2 builds it too.
#include "common.h"

extern "C" {
// Menu_AddGouraudQuad with grey levels (0-255) for colours.
void Menu_AddGreyBand(uint *ot, GouraudQuad *q)
{
    uint top = q->top;
    uint bottom = q->bottom;
    PrimG4 *p = Prim_AddPolyG4(ot);
    uint c;

    c = top | top << 8 | top << 16 | 0x3a000000;
    p->c2 = c;
    p->c0 = c;
    c = bottom | bottom << 8 | bottom << 16;
    p->c3 = c;
    p->c1 = c;
    p->x0 = p->x2 = q->rect.x;
    p->x1 = p->x3 = q->rect.x + q->rect.w;
    p->y0 = p->y1 = q->rect.y;
    p->y2 = p->y3 = q->rect.y + q->rect.h;
}

// Adds a semi-transparent (additive) gradient quad.
void Menu_AddGouraudQuad(uint *ot, GouraudQuad *q)
{
    PrimG4 *p = Prim_AddPolyG4(ot);

    p->c0 = p->c2 = q->top | 0x3a000000;
    p->c1 = p->c3 = q->bottom;
    p->x0 = p->x2 = q->rect.x;
    p->x1 = p->x3 = q->rect.x + q->rect.w;
    p->y0 = p->y1 = q->rect.y;
    p->y2 = p->y3 = q->rect.y + q->rect.h;
}
}
