// OVL0 800279e8-80028cc0 (ground): Ground and course sections.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"
#include "gte.h"

// The vertices of the course meshes.
struct GroundVert {
    short x;
    short z;
    short y;
    short pad;
};

// What World_GetGroundInfo fills.
struct GroundInfo {
    unsigned char surface;
    unsigned char side;
    unsigned char kind;
    unsigned char pad3;
    unsigned short section;
    unsigned short pad6;
    int pos[3];
    int height;
};

struct Vec3i {
    int v[3];
};

// A corner's move in a wall test: world from and to, then the nearest hit.
struct WallRay {
    int from[2];
    int pad8;
    int to[2];
    int pad14;
    int t;
    short normal[2];
    short start[2];
    short end[2];
    short side[2];
};

struct WallRays {
    WallRay rays[4];
};

// The approximate distance of two points: longest + middle / 2 + shortest / 4.
static inline unsigned int Vec_ApproxDist(int *a, int *b)
{
    int x = a[0] - b[0];
    int y = a[1] - b[1];
    int z = a[2] - b[2];
    unsigned int c;
    unsigned int swap;

    if (x < 0) {
        x = -x;
    }
    if (y < 0) {
        y = -y;
    }
    c = z;
    if ((int)c < 0) {
        c = -c;
    }
    if ((unsigned int)x < (unsigned int)y) {
        swap = x;
        x = y;
        y = swap;
    }
    if ((unsigned int)x < c) {
        swap = x;
        x = c;
        c = swap;
    }
    if ((unsigned int)y < c) {
        swap = y;
        y = c;
        c = swap;
    }
    return x + ((unsigned int)y >> 1) + (c >> 2);
}

// The course, as the game reaches it: the race plus 0xb534, added at run time.
static inline CourseData *World_Course(void *world)
{
    int offset = 0xb534;

    return (CourseData *)((char *)world + offset);
}

// A fraction: value * scale >> 12.
static inline long long Fix12_Mul(long long value, long long scale)
{
    return value * scale >> 12;
}

extern "C" {
bool CourseSect_IsInFront(CourseSection *section, int *pos);
int Ground_GetHeight(CourseSection *section, unsigned int *triangle, int *pos);
void CourseSection_CollideWalls(CourseSection *section, WallRay *rays);
int CourseSect_Locate(CourseSectionTable *table, int index, int *pos);
void CourseSect_GetSurface(CourseSectionTable *table, int index, GroundInfo *info);
int CourseSect_GetDistance(CourseSectionTable *table, int index, int *pos);
void World_GetGroundInfo(void *world, GroundInfo *info);
#ifdef NON_MATCHING
// Hand-written: walks the grid cell's triangles, testing the point with nclip, and
// returns the one it falls in. Each round tests the triangle loaded a round before.
unsigned int *Ground_FindTriangle(short *grid, void *verts, int x, int z)
{
    unsigned int cellX = (x - grid[0]) >> grid[2];
    unsigned int cellZ = (z - grid[1]) >> grid[3];
    int cell;
    int count;
    unsigned int **list;
    unsigned int **end;
    unsigned int **next;
    unsigned int *found;
    unsigned int *tri;
    unsigned int *following;
    unsigned int word;
    unsigned int flags;
    int v0;
    int v1;
    int v2;
    int v3;
    int last;

    if (cellX >= 4 || cellZ >= 4) {
        return 0;
    }
    cell = cellZ * 4 + cellX;
    count = ((unsigned short *)grid)[cell + 4];
    list = (unsigned int **)((int *)grid)[cell + 10];
    if (count == 0) {
        return 0;
    }
    end = list + count;
    tri = list[0];
    next = list + 1;
    if (next == end) {
        next = end - 1;
    }
#define VERT(index) (*(int *)((char *)verts + (index)))
    word = tri[0];
    gte_SetDataReg(GTE_SXY0, (x & 0xffff) | (z << 16));
    v0 = VERT((word << 3) & 0xff8);
    v1 = VERT((word >> 6) & 0xff8);
    v2 = VERT((word >> 15) & 0xff8);
    v3 = VERT((tri[1] << 3) & 0xff8);
    gte_SetDataReg(GTE_SXY1, v0);
    gte_SetDataReg(GTE_SXY2, v1);
    gte_nclip();
    following = *next;
    flags = word;
    do {
        int a;
        int b;
        int c;
        int d;

        word = following[0];
        flags <<= 1;
        found = tri;
        last = v0;
        if ((int)flags >= 0) {
            v3 = v2;
        }
        gte_SetDataReg(GTE_SXY1, v2);
        a = -gte_GetDataReg(GTE_MAC0);
        gte_nclip();
        v0 = VERT((word << 3) & 0xff8);
        gte_SetDataReg(GTE_SXY2, v3);
        b = gte_GetDataReg(GTE_MAC0);
        gte_nclip();
        v1 = VERT((word >> 6) & 0xff8);
        v2 = VERT((word >> 15) & 0xff8);
        gte_SetDataReg(GTE_SXY1, last);
        c = -gte_GetDataReg(GTE_MAC0);
        gte_nclip();
        v3 = VERT((following[1] << 3) & 0xff8);
        next++;
        if (next == end) {
            next = end - 1;
        }
        tri = following;
        following = *next;
        gte_SetDataReg(GTE_SXY1, v0);
        gte_SetDataReg(GTE_SXY2, v1);
        d = gte_GetDataReg(GTE_MAC0);
        gte_nclip();
        list++;
        if ((a | b | c | d) >= 0) {
            return found;
        }
        flags = word;
    } while (list != end);
#undef VERT
    return 0;
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ground", Ground_FindTriangle);
#endif

// Is pos on the front of the section's plane?
bool CourseSect_IsInFront(CourseSection *section, int *pos)
{
    int diff[3];
    int *v;
    int *origin;

    diff[0] = pos[0] - section->planeOrigin[0];
    origin = section->planeOrigin;
    v = diff;
    v[1] = pos[1] - origin[1];
    v[2] = pos[2] - origin[2];
    return Vec_DotFix12(v, section->planeNormal) >= 0;
}

// Ground_FindTriangle in the section's grid for pos (relative to the section's 4 MB
// block).
unsigned int *CourseSection_FindGround(CourseSection *sect, int *pos)
{
    unsigned int *puVar1;

    puVar1 = Ground_FindTriangle(sect->groundGrid, (sect->mesh).verts,
                                 (int)(*pos - (sect->center[0] & 0xffc00000U)) >> 10,
                                 (int)(pos[2] - (sect->center[2] & 0xffc00000U)) >> 10);
    return puVar1;
}

#ifdef NON_MATCHING
// The height at pos of the triangle, or of the part of the quad pos is in.
int Ground_GetHeight(CourseSection *section, unsigned int *triangle, int *pos)
{
    GroundVert *verts = (GroundVert *)section->mesh.verts;
    int blockX = pos[0] - (section->center[0] & 0xffc00000);
    int blockZ = pos[2] - (section->center[2] & 0xffc00000);
    int x = blockX >> 10;
    int z = blockZ >> 10;
    unsigned int word = triangle[0];
    GroundVert *a = &verts[word & 0x1ff];
    GroundVert *b = (GroundVert *)((char *)verts + ((word >> 6) & 0xff8));
    GroundVert *c = (GroundVert *)((char *)verts + ((word >> 15) & 0xff8));
    int blockY = section->center[1] & 0xffc00000;
    GroundVert middle;
    int height;
    int bx;
    int bz;
    int ax;
    int az;
    int ay;
    int by;
    int det;

    if (word & 0x40000000) {
        GroundVert *d = &verts[triangle[1] & 0x1ff];
        GroundVert *first = a;
        GroundVert *third = c;
        int mx = (a->x + b->x + c->x + d->x) >> 2;
        int mz = (a->z + b->z + c->z + d->z) >> 2;
        int my = (a->y + b->y + c->y + d->y) >> 2;
        int u;
        int w;
        int side;

        middle.x = mx;
        middle.z = mz;
        middle.y = my;
        u = x - mx;
        w = z - mz;
        side = (unsigned int)(u * (a->z - mz) - w * (a->x - mx)) >> 31;
        if (u * (b->z - mz) - w * (b->x - mx) < 0) {
            side |= 2;
        }
        if (u * (c->z - mz) - w * (c->x - mx) < 0) {
            side |= 4;
        }
        if (u * (d->z - mz) - w * (d->x - mx) < 0) {
            side |= 8;
        }
        c = &middle;
        switch (side) {
        case 1:
        case 3:
        case 7:
            a = d;
            b = first;
            break;
        case 4:
        case 12:
        case 13:
            a = b;
            b = third;
            break;
        case 8:
        case 9:
        case 11:
            a = third;
            b = d;
            break;
        default:
            a = first;
            break;
        }
    }
    ax = a->x - c->x;
    bz = b->z - c->z;
    az = a->z - c->z;
    bx = b->x - c->x;
    ay = a->y - c->y;
    by = b->y - c->y;
    blockX -= c->x << 10;
    blockZ -= c->z << 10;
    height = c->y << 10;
    det = ax * bz - az * bx;
    if (det != 0) {
        height += ((long long)blockX * (ay * bz - az * by) + (long long)blockZ * (ax * by - ay * bx)) / (long long)det;
    }
    return blockY + height;
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ground", Ground_GetHeight);
#endif

#ifdef NON_MATCHING
// Hand-written: clips the four corners' moves to the section's walls, in the section's
// 4 MB block, with the GTE's matrices doing the cross products.
void CourseSection_CollideWalls(CourseSection *section, WallRay *rays)
{
    int *walls = section->walls;
    int count = walls[0];
    int originX;
    int originZ;
    int *verts;
    int *wall;
    int i;

    if (count == 0) {
        return;
    }
    originX = section->center[0] & 0xffc00000;
    originZ = section->center[2] & 0xffc00000;
    for (i = 0; i < 4; i++) {
        WallRay *ray = &rays[i];
        int x0 = (ray->from[0] - originX) >> 10;
        int z0 = (ray->from[1] - originZ) >> 10;
        int x1 = (ray->to[0] - originX) >> 10;
        int z1 = (ray->to[1] - originZ) >> 10;
        int dx = x1 - x0;
        int dz = z1 - z0;

        ray->start[0] = x0;
        ray->start[1] = z0;
        ray->end[0] = z1;
        ray->end[1] = x1;
        if ((unsigned int)(x0 + 0x7fff) > 0xfffe || x1 < -0x7fff || x1 > 0x7fff || z0 < -0x7fff || z0 > 0x7fff ||
            z1 < -0x7fff || z1 > 0x7fff) {
            dz = 0;
            dx = 0;
        }
        ray->side[0] = -dz;
        ray->side[1] = dx;
    }
    verts = (int *)section->mesh.verts;
    gte_SetDataReg(GTE_VZ0, 0);
    gte_SetDataReg(GTE_VZ1, 0);
    gte_SetCtrlReg(GTE_R33, 0);
    gte_SetCtrlReg(GTE_L33, 0);
    wall = walls + 1;
    do {
        unsigned int ends = wall[0];
        int a = verts[(ends & 0xffff) * 2];
        int b = verts[(ends >> 16) * 2];

        count--;
        gte_SetCtrlReg(GTE_R31R32, a);
        gte_SetCtrlReg(GTE_L11L12, a);
        gte_SetCtrlReg(GTE_L31L32, b);
        gte_SetDataReg(GTE_VXY0, (((b >> 16) - (a >> 16)) & 0xffff) | ((a << 16) - (b << 16)));
        for (i = 0; i < 4; i++) {
            WallRay *ray = &rays[i];
            int end = *(int *)ray->end;
            int m1;
            int m2;
            int m3;
            int across;
            int along;
            int hit;

            gte_SetCtrlReg(GTE_R11R12, *(int *)ray->start);
            gte_SetCtrlReg(GTE_R13R21, end);
            gte_SetCtrlReg(GTE_R22R23, end);
            gte_mvmva(0, 0, 0, 3, 0);
            gte_SetDataReg(GTE_VXY1, *(int *)ray->side);
            gte_SetCtrlReg(GTE_L13L21, end);
            gte_SetCtrlReg(GTE_L22L23, end);
            m3 = gte_GetDataReg(GTE_MAC3);
            m1 = gte_GetDataReg(GTE_MAC1);
            m2 = gte_GetDataReg(GTE_MAC2);
            gte_mvmva(0, 1, 1, 3, 0);
            across = m1 - m3;
            along = across - (m2 - m3);
            if ((across ^ (m2 - m3)) >= 0 || (across | along) < 0) {
                continue;
            }
            m2 = gte_GetDataReg(GTE_MAC2);
            m1 = gte_GetDataReg(GTE_MAC1);
            m3 = gte_GetDataReg(GTE_MAC3);
            if (((m1 - m2) ^ (m3 - m2)) >= 0) {
                continue;
            }
            hit = ((long long)across << 12) / along;
            if (hit < ray->t) {
                ray->t = hit;
                ray->normal[0] = ((unsigned short *)wall)[3];
                ray->normal[1] = -((unsigned short *)wall)[2];
            }
        }
        wall += 2;
    } while (count != 0);
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ground", CourseSection_CollideWalls);
#endif

#ifdef NON_MATCHING
// The section nearest pos, by centre, then refined by its planes.
int CourseSect_FindNearest(CourseSectionTable *table, int *pos)
{
    CourseSection **sections = table->sections;
    CourseSection **end = sections + table->count;
    unsigned int best = -1;
    int found = -1;
    int i = 0;

    for (; sections != end; i++, sections++) {
        unsigned int length = Vec_ApproxDist((*sections)->center, pos);

        if (length < best) {
            best = length;
            found = i;
        }
    }
    if (found > 0) {
        found = CourseSect_Locate(table, found, pos);
    }
    return found;
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ground", CourseSect_FindNearest);
#endif

// From a section, forward to the first whose plane pos is in front of, then back while
// it stays in front of the previous ones.
int CourseSect_Locate(CourseSectionTable *table, int index, int *pos)
{
    CourseSection *section = table->sections[index];
    CourseSection *first;
    CourseSection *prev;
    int start;

    if (section != 0) {
        do {
            if (CourseSect_IsInFront(section, pos)) {
                break;
            }
            index = section->nextIdx;
            section = section->next;
        } while (section != 0);
        start = index;
        if (section != 0) {
            prev = section->prev;
            if (prev != 0) {
                first = section;
                do {
                    if (!CourseSect_IsInFront(prev, pos)) {
                        break;
                    }
                    if (first == prev) {
                        index = start;
                        break;
                    }
                    index = section->prevIdx;
                    section = prev;
                    prev = prev->prev;
                } while (prev != 0);
            }
        }
    }
    return index;
}

#ifdef NON_MATCHING
// The ground at the info's position, searching eight sections each way.
void CourseSect_GetSurface(CourseSectionTable *table, int index, GroundInfo *info)
{
    CourseSection *section = table->sections[index];
    int *pos = info->pos;
    unsigned int *triangle = CourseSection_FindGround(section, pos);
    int tries = 8;
    CourseSection *forward;
    CourseSection *back;

    if (triangle != 0) {
    found:
        {
            unsigned int word = triangle[1];

            *(int *)info = 0;
            info->surface = word >> 28;
            info->side = (word >> 23) & 3;
            info->kind = (word >> 25) & 3;
            info->height = Ground_GetHeight(section, triangle, pos);
        }
        return;
    }
    forward = section->next;
    back = section->prev;
    section = forward;
    do {
        triangle = CourseSection_FindGround(forward, pos);
        if (triangle != 0) {
            goto found;
        }
        section = back;
        triangle = CourseSection_FindGround(back, pos);
        if (triangle != 0) {
            goto found;
        }
        tries--;
        forward = forward->next;
        back = back->prev;
        section = forward;
    } while (tries != 0);
    *(int *)info = 0;
    info->surface = 7;
    info->side = 4;
    info->height = 0x7fffffff;
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ground", CourseSect_GetSurface);
#endif

#ifdef NON_MATCHING
// How far along the course pos is: between its section's start and the previous one's,
// in proportion to the distances from their planes.
int CourseSect_GetDistance(CourseSectionTable *table, int index, int *pos)
{
    CourseSection *section = table->sections[CourseSect_Locate(table, index, pos)];
    CourseSection *prev = section->prev;
    int diff[3];
    int *v = diff;
    long long fromStart;
    long long fromEnd;
    int start32;
    int sum;
    int fraction;
    int start;
    int span;
    int length;
    int distance;

    {
        int *origin = section->planeOrigin;

        v[0] = pos[0] - section->planeOrigin[0];
        v[1] = pos[1] - origin[1];
        v[2] = pos[2] - origin[2];
    }
    fromStart = Fix12_Mul(Vec_DotFix12(v, section->planeNormal), *(short *)(section->unk10 + 6));
    start32 = fromStart;
    {
        int *origin = prev->planeOrigin;

        v[0] = prev->planeOrigin[0] - pos[0];
        v[1] = origin[1] - pos[1];
        v[2] = origin[2] - pos[2];
    }
    fromEnd = Fix12_Mul(Vec_DotFix12(v, prev->planeNormal), *(short *)(section->unk10 + 4));
    sum = (int)fromEnd + start32;
    if (sum != 0) {
        fraction = ((long long)start32 << 12) / sum;
    } else {
        fraction = 0;
    }
    start = *(int *)section->unk10;
    length = table->unk00;
    span = *(int *)prev->unk10 - start;
    if (length < span) {
        span -= length;
    } else if (span < 0) {
        span += length;
    }
    distance = start + ((long long)span * fraction >> 12);
    if (length < distance) {
        distance -= length;
    } else if (distance < 0) {
        distance += length;
    }
    return distance;
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ground", CourseSect_GetDistance);
#endif

int World_LocateSection(void *world, int hint, int *pos)
{
    Vec3i local = *(Vec3i *)pos;

    local.v[2] = -local.v[2];
    return CourseSect_Locate(World_Course(world)->sections, hint, local.v);
}

void World_GetGroundInfo(void *world, GroundInfo *info)
{
    GroundInfo local = *info;
    CourseData *course;

    local.pos[2] = -local.pos[2];
    course = World_Course(world);
    CourseSect_GetSurface(course->sections, local.section = CourseSect_Locate(course->sections, local.section, local.pos),
                          &local);
    local.pos[2] = -local.pos[2];
    *info = local;
}

int World_GetGroundHeight(void *world, int x, int z, int y, int *section)
{
    GroundInfo info;

    int height;

    x <<= 4;
    y <<= 4;
    info.section = *section;
    z <<= 4;
    info.pos[0] = x;
    info.pos[1] = y;
    info.pos[2] = z;
    World_GetGroundInfo(world, &info);
    *section = info.section;
    height = info.height;
    if (height != 0x7fffffff) {
        height >>= 4;
    }
    return height;
}

#ifdef NON_MATCHING
// Clips four corners' moves to the walls of a section and the ones around it, until
// they're a long way off.
void World_CollideWalls(void *world, int sect, int *corners)
{
    WallRays local = *(WallRays *)corners;
    int i;
    int forwardDist = 0;
    int backDist = 0;
    int steps = 0;
    int done;
    CourseSection *forward;
    CourseSection *back;

    for (i = 0; i < 4; i++) {
        local.rays[i].from[1] = -local.rays[i].from[1];
        local.rays[i].to[1] = -local.rays[i].to[1];
        local.rays[i].t = 0x1000;
    }
    {
        CourseSection *section = World_Course(world)->sections->sections[sect];

        CourseSection_CollideWalls(section, local.rays);
        forward = section->next;
        back = section->prev;
    }
    while (steps < 8) {
        done = 1;
        if (forwardDist <= 0xfffff) {
            CourseSection *from = forward;

            CourseSection_CollideWalls(forward, local.rays);
            forward = from->next;
            forwardDist += Vec_ApproxDist(forward->planeOrigin, from->planeOrigin);
            done = 0;
        }
        if (backDist <= 0xfffff) {
            CourseSection *to;

            CourseSection_CollideWalls(back, local.rays);
            to = back->prev;
            backDist += Vec_ApproxDist(back->planeOrigin, to->planeOrigin);
            back = to;
            done = 0;
        }
        steps++;
        if (done) {
            break;
        }
    }
    for (i = 0; i < 4; i++) {
        local.rays[i].from[1] = -local.rays[i].from[1];
        local.rays[i].normal[1] = -local.rays[i].normal[1];
        local.rays[i].to[1] = -local.rays[i].to[1];
    }
    *(WallRays *)corners = local;
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/ground", World_CollideWalls);
#endif

int World_GetCourseDistance(void *world, int hint, int *pos)
{
    Vec3i local = *(Vec3i *)pos;

    local.v[2] = -local.v[2];
    return CourseSect_GetDistance(World_Course(world)->sections, hint, local.v);
}
}
