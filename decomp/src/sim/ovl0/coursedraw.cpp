// OVL0 80018d1c-80026bb4 (coursedraw): Course, sky and course object drawing.
// The functions that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"
#include "gte.h"

extern "C" {
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/coursedraw", Sky_Draw);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/coursedraw", Sky_DrawHorizonFill);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/coursedraw", CourseObj_DrawMeshUnclipped);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/coursedraw", CourseObj_DrawMeshClipped);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/coursedraw", CourseObjects_Draw);

// Draws object groups 0-31 of the course whose bit is set in mask (the visible sections
// from Course_Draw), then group 32 always.
void CourseObjects_DrawGroups(Camera *cam, CourseData *course, unsigned int mask)
{
    int i;

    for (i = 0; i < 32; i++, mask >>= 1) {
        if (mask & 1) {
            CourseObjects_Draw(cam, course->objGroups[i]);
        }
    }
    CourseObjects_Draw(cam, course->objGroups[i]);
}

// Adds base to a pointer.
static inline void Ptr_Relocate(void **p, int base)
{
    *p = (char *)*p + base;
}

// Adds base to the 11 pointers of a mesh header (vertices, 8 primitive lists, sprites,
// flares).
void CourseObjModel_Relocate(CourseMesh *mesh, int base)
{
    int i;

    mesh->verts = (char *)mesh->verts + base;
    for (i = 0; i < 8; i++) {
        Ptr_Relocate(&mesh->prims[i], base);
    }
    mesh->sprites = (int *)((char *)mesh->sprites + base);
    mesh->flares = (int *)((char *)mesh->flares + base);
}

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/coursedraw", Course_Draw);

// Swaps columns 1 and 2 of each row of a rotation, for the course's (x, z, y) axis order.
void Matrix_SwapYZ(MatrixRot *m)
{
    int x, y, z;

    // Unrolled: each row is read whole, then written back (x unchanged).
    x = m->m[0][0];
    y = m->m[0][1];
    z = m->m[0][2];
    m->m[0][0] = x;
    m->m[0][1] = z;
    m->m[0][2] = y;
    x = m->m[1][0];
    y = m->m[1][1];
    z = m->m[1][2];
    m->m[1][0] = x;
    m->m[1][1] = z;
    m->m[1][2] = y;
    x = m->m[2][0];
    y = m->m[2][1];
    z = m->m[2][2];
    m->m[2][0] = x;
    m->m[2][1] = z;
    m->m[2][2] = y;
}

// Gte_LoadScaled(scr, 0, 0, 0) with a zero translation and a black far colour (the game
// writes $zero straight to the GTE).
void Gte_LoadRotationNoFog(ViewScratch *scr)
{
    register int zero asm("$0");

    Gte_LoadScaled(scr, 0, 0, 0);
    gte_SetCtrlReg(GTE_TRX, zero);
    gte_SetCtrlReg(GTE_TRY, zero);
    gte_SetCtrlReg(GTE_TRZ, zero);
    gte_SetCtrlReg(GTE_RFC, zero);
    gte_SetCtrlReg(GTE_BFC, zero);
    gte_SetCtrlReg(GTE_GFC, zero);
}

// Frustum test of a section's box (4 corners' x and z, then y min and max: 8 points),
// relative to the camera and scaled to 16 bits into BoxScratch.box. Returns 0 inside,
// 1 crossing a plane, 2 outside.
#ifdef NON_MATCHING
// Handwritten: the game waits two nops between the LZCS write and the LZCR read, and
// steps two pointers into the points.
int CourseSection_TestBox(ViewScratch *scr, int *box)
{
    ClipPoint *out = ((BoxScratch *)scr)->box;
    int nx = scr->negPos[0];
    int ny = scr->negPos[1];
    int nz = scr->negPos[2];
    int boxY = box[8];
    int pass;
    unsigned int flags;

    for (pass = 2; pass != 0; pass--) {
        int *p;

        for (p = box; p != box + 8; p += 2) {
            int x = p[0] + nx;
            int y = boxY + ny;
            int z = p[1] + nz;
            int shift;

            gte_SetDataReg(GTE_LZCS, (x < 0 ? -x : x) | (y < 0 ? -y : y) | (z < 0 ? -z : z));
            shift = 19 - gte_GetDataReg(GTE_LZCR);
            if (shift >= 0) {
                x >>= shift;
                y >>= shift;
                z >>= shift;
            }
            // The course's y and z are the GTE's z and y.
            out->x = x;
            out->y = z;
            out->z = y;
            out++;
        }
        boxY = box[9];
    }
    flags = Clip_TestPoints(scr, ((BoxScratch *)scr)->box, 8);
    if (flags & 0x1f) {
        return 2;
    }
    return ((flags >> 16) & 0x3f) != 0;
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/coursedraw", CourseSection_TestBox);
#endif

// Approximate distance from the camera to pos: the largest absolute axis difference plus
// half the middle one plus a quarter of the smallest.
#ifdef NON_MATCHING
// The game computes z in $v0 and copies it to $v1 before its abs; GCC computes it in $v1.
unsigned int Vec_ApproxDist(ViewScratch *scr, int *pos)
{
    unsigned int x = pos[0] + scr->negPos[0];
    unsigned int y = pos[1] + scr->negPos[1];
    unsigned int z = pos[2] + scr->negPos[2];
    unsigned int t;

    if ((int)x < 0) {
        x = -x;
    }
    if ((int)y < 0) {
        y = -y;
    }
    if ((int)z < 0) {
        z = -z;
    }
    if (x < y) {
        t = x;
        x = y;
        y = t;
    }
    if (x < z) {
        t = x;
        x = z;
        z = t;
    }
    if (y < z) {
        t = y;
        y = z;
        z = t;
    }
    return x + (y >> 1) + (z >> 2);
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/coursedraw", Vec_ApproxDist);
#endif

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/coursedraw", CourseMesh_DrawUnclipped);

INCLUDE_ASM("asm/sim/ovl0/nonmatchings/coursedraw", CourseMesh_DrawClipped);
}
