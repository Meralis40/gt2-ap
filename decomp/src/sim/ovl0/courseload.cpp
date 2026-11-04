// OVL0 80026bb4-800279e8 (courseload): Course data relocation and loading.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"
#include "gamestate.h"
#include "gte.h"

// g_RaceSetup, as the game reaches it: g_GameState plus 0xbf7c, added at run time.
static inline RaceSetup *RaceSetup_Get(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;

    return (RaceSetup *)(base + offset);
}

// The camera a section is drawn from.
struct CourseView {
    short rot[16];
    int pos[3];
    char unk2c[0x4c];
    int trans[3];
    char unk84[0x16];
    short shift;
    short matrix[10];
};

// A section's ground grid.
struct CourseGrid {
    char unk0[8];
    unsigned short counts[16];
    int cells[16];
};

// A table of offsets.
struct PtrTable {
    unsigned short count;
    int entries[1];
};

// A place banners go on a course.
struct BannerSlot {
    short (*coords)[2];
    short clutX;
    short clutY;
    unsigned short size;
    unsigned char count;
};

// The seed kept at the end of the event name.
#define RACE_SEED(setup) (*(unsigned int *)&(setup)->eventName[0x10])

// The world's course index.
#define WORLD_COURSE(world) (*((unsigned char *)(world) + 0x5d2c))

extern "C" {
extern unsigned short g_VolBootIds[248];
extern RECT g_CourseMapClutRect;
extern RECT g_CourseMapImageRect;
extern char D_8002F1B8[];
extern unsigned char D_800A951C;
extern BannerSlot g_BannerSlots[];
void CourseObjModel_Relocate(void *model, int base);
void AiLines_Init(void *lines);
void Race_LoadBanners(void *race, int index);
void Race_LoadCourseTims(void *race);
void Race_LoadCourse(void *race, int course);
extern void *Tim_LoadNext(void *tim, RECT *clutRect, RECT *imageRect) __asm__("Tim_LoadClutAndImage");

// Places a banner's clut and image in its slot.
static inline void Banner_Place(char *tim, BannerSlot *slot, int index)
{
    short *clut = (short *)(tim + 8);
    short *image;

    clut[2] = slot->clutX;
    clut[3] = slot->clutY + index;
    image = (short *)((char *)clut + *(int *)(tim + 8));
    image[2] = slot->coords[index][0];
    image[3] = slot->coords[index][1];
}

#ifdef NON_MATCHING
// Loads the view's rotation for a section, shifted down as far as the section's
// distance allows, and its translation.
void CourseSection_LoadTransform(CourseSection *section, CourseView *view)
{
    int px = view->pos[0];
    int py = view->pos[1];
    int pz = view->pos[2];
    int cx = section->center[0];
    int cy = section->center[1];
    int cz = section->center[2];
    long long dist = (long long)(cx + px) * (cx + px) + (long long)(cy + py) * (cy + py) +
                     (long long)(cz + pz) * (cz + pz) + (long long)section->radius * section->radius;
    long long limit = (long long)0x40000 << 32;
    int shift = 0;

    if (dist < limit) {
        do {
            limit >>= 2;
            shift++;
        } while (dist < limit);
    }
    if (shift != 0) {
        shift--;
    }
    if (shift >= 2) {
        shift = 2;
    }
    view->shift = shift;
    Matrix_ShiftCopy(view->matrix, (short *)view, shift);
    {
        int *m = (int *)view->matrix;
        int x;
        int y;
        int z;

        gte_SetCtrlReg(0, m[0]);
        gte_SetCtrlReg(1, m[1]);
        gte_SetCtrlReg(2, m[2]);
        gte_SetCtrlReg(3, m[3]);
        gte_SetCtrlReg(4, m[4]);
        x = (px + (cx & 0xffc00000)) >> 10;
        y = (py + (cy & 0xffc00000)) >> 10;
        z = (pz + (cz & 0xffc00000)) >> 10;
        gte_SetDataReg(9, x);
        gte_SetDataReg(10, z);
        gte_SetDataReg(11, y);
        gte_mvmva(1, 0, 3, 3, 0);
        x = gte_GetDataReg(25);
        y = gte_GetDataReg(26);
        z = gte_GetDataReg(27);
        gte_SetCtrlReg(5, x);
        gte_SetCtrlReg(6, y);
        gte_SetCtrlReg(7, z);
        view->trans[0] = x;
        view->trans[1] = y;
        view->trans[2] = z;
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/courseload", CourseSection_LoadTransform);
#endif

#ifdef NON_MATCHING
void CourseMesh_Relocate(CourseMesh *mesh, int base)
{
    int i;

    mesh->verts = (char *)mesh->verts + base;
    i = 0;
next:
    if (i < 8) {
        int *prim = &((int *)mesh->prims)[i];

        *prim += base;
        i++;
        goto next;
    }
    mesh->sprites = (int *)((char *)mesh->sprites + base);
    mesh->flares = (int *)((char *)mesh->flares + base);
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/courseload", CourseMesh_Relocate);
#endif

// Empty.
void CourseWalls_Relocate(int *walls, int base)
{
}

#ifdef NON_MATCHING
// A section's ground grid: 16 lists of cells.
void CourseGrid_Relocate(CourseGrid *grid, int base)
{
    int i;

    for (i = 0; i < 16; i++) {
        int *entry = &grid->cells[i];
        int j = 0;
        int value = *entry;
        int count = grid->counts[i];

    next:
        *entry = value + base;
        if (j < count) {
            entry = (int *)((j << 2) + grid->cells[i]);
            value = *entry;
            j++;
            goto next;
        }
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/courseload", CourseGrid_Relocate);
#endif

// Empty.
void CourseVisList_Relocate(unsigned short *list, int base)
{
}

void CourseSection_Relocate(CourseSection *section, int base)
{
    section->mirrorMesh = (CourseMesh *)((char *)section->mirrorMesh + base);
    CourseMesh_Relocate(section->mirrorMesh, base);
    section->walls = (int *)((char *)section->walls + base);
    CourseWalls_Relocate(section->walls, base);
    section->groundGrid = (char *)section->groundGrid + base;
    CourseGrid_Relocate((CourseGrid *)section->groundGrid, base);
    section->visList = (unsigned short *)((char *)section->visList + base);
    CourseVisList_Relocate(section->visList, base);
    CourseMesh_Relocate(&section->mesh, base);
}

#ifdef NON_MATCHING
// Also links each section to its neighbours.
void CourseSections_Relocate(CourseSectionTable *table, int base)
{
    int i;

    table->spriteTex = (char *)table->spriteTex + base;
    i = 0;
next:
    if (i < table->count) {
        int *section = (int *)&table->sections[i];

        *section += base;
        i++;
        goto next;
    }
    for (i = 0; i < table->count; i++) {
        CourseSection *section = table->sections[i];

        section->next = table->sections[section->nextIdx];
        section = table->sections[i];
        section->prev = table->sections[section->prevIdx];
        CourseSection_Relocate(table->sections[i], base);
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/courseload", CourseSections_Relocate);
#endif

// Empty.
void CourseObjGroup_Relocate(unsigned int *group, int base)
{
}

#ifdef NON_MATCHING
void LodTable_Relocate(LodTable *table, int base)
{
    unsigned int i;

    LodTable_SquareDistances(table);
    i = 0;
next:
    if (i < *(unsigned int *)table) {
        int *mesh = (int *)&table->lods[i].mesh;

        *mesh += base;
        i++;
        goto next;
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/courseload", LodTable_Relocate);
#endif

#ifdef NON_MATCHING
void ModelLib_Relocate(unsigned int *lib, int base)
{
    unsigned int i = 0;
    unsigned int *entry = lib;

next:
    if (i < lib[0]) {
        unsigned int *table = &lib[i + 1];

        i++;
        *table += base;
        LodTable_Relocate((LodTable *)entry[1], base);
        entry++;
        goto next;
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/courseload", ModelLib_Relocate);
#endif

#ifdef NON_MATCHING
void CourseObjModels_Relocate(unsigned int *models, int base)
{
    unsigned int i = 0;
    unsigned int *entry = models;

next:
    if (i < models[0]) {
        unsigned int *model = &models[i + 1];

        i++;
        *model += base;
        CourseObjModel_Relocate((void *)entry[1], base);
        entry++;
        goto next;
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/courseload", CourseObjModels_Relocate);
#endif

#ifdef NON_MATCHING
void PtrTable_Relocate(unsigned short *table, int base)
{
    int i;

    i = 0;
next:
    if (i < table[0]) {
        int *entry = &((PtrTable *)table)->entries[i];

        *entry += base;
        i++;
        goto next;
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/courseload", PtrTable_Relocate);
#endif

#ifdef NON_MATCHING
// Makes the offsets in a loaded course absolute.
void Course_Relocate(CourseData *course)
{
    int base = (int)course;
    int i;

    course->sections = (CourseSectionTable *)((char *)course->sections + base);
    course->modelLib = (uint *)((char *)course->modelLib + base);
    course->objModels = (uint *)((char *)course->objModels + base);
    course->table1c = (ushort *)((char *)course->table1c + base);
    i = 0;
next:
    if (i < 33) {
        int *group = (int *)&course->objGroups[i];

        *group += base;
        i++;
        goto next;
    }
    course->data20 = (char *)course->data20 + base;
    CourseSections_Relocate(course->sections, base);
    ModelLib_Relocate(course->modelLib, base);
    CourseObjModels_Relocate(course->objModels, base);
    for (i = 0; i < 33; i++) {
        CourseObjGroup_Relocate(course->objGroups[i], base);
    }
    PtrTable_Relocate(course->table1c, base);
    AiLines_Init(course->data20);
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/courseload", Course_Relocate);
#endif

#ifdef NON_MATCHING
// A count, then that many TIMs, each aligned to 4 bytes.
void TimList_Upload(int *list)
{
    int *tim = list;
    int n = tim[0];

    tim++;
    int end = -1;
    int mask = -4;

next:
    n--;
    if (n != end) {
        tim = (int *)Tim_LoadNext(tim, 0, 0);
        tim = (int *)(((int)tim + 3) & mask);
        goto next;
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/courseload", TimList_Upload);
#endif

void Race_SetCourseIndex(RaceMem *race)
{
    RaceWorld *world = &race->world;
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;

    WORLD_COURSE(world) = Course_FindByHash(((RaceSetup *)(base + offset))->courseHash);
}

// Loads sky skyId: the TIM list file (g_VolBootIds[0x46] + skyId * 2 + 1) through the
// load buffer at race +0xe5700 to VRAM, then the sky mesh file to race +0x5d34.
void Race_LoadSky(void *race, int skyId)
{
    int file;

    file = (unsigned int)g_VolBootIds[0x46] + skyId *2;
    Vol_ReadFileAsync(file + 1, (int *)((int)race + 0xe5700));
    TimList_Upload((int *)((int)race + 0xe5700));
    Vol_ReadFileAsync(file, (void *)((int)race + 0x5d34));
}

// Loads the course map image (CourseMap_GetFileId of the course's map id, Course_Get
// +4) to the load buffer and uploads it to g_CourseMapClutRect / g_CourseMapImageRect.
void Race_LoadCourseMap(void *race, int course)
{
    CourseEntry *pCVar1;
    int file;

    pCVar1 = Course_Get(course);
    file = CourseMap_GetFileId(pCVar1->hash);
    Vol_ReadFileAsync(file, (void *)((int)race + 0xe5700));
    Tim_LoadClutAndImage((void *)((int)race + 0xe5700), &g_CourseMapClutRect, &g_CourseMapImageRect);
}

// Loads a course: the map image, the course textures (g_VolBootIds[0x6c] + course * 2
// + 1, uploaded), the course file (same id) to race +0xb534 (0x800b4a34),
// Course_Relocate, then the sky of the course (Course_Get +0xa).
void Race_LoadCourse(void *race, int course)
{
    CourseEntry *pCVar1;
    int file;
    CourseData *course_00;

    Race_LoadCourseMap(race, course);
    pCVar1 = Course_Get(course);
    course_00 = (CourseData *)((int)race + 0xb534);
    file = (unsigned int)g_VolBootIds[0x6c] + course *2;
    Vol_ReadFileAsync(file + 1, course_00);
    TimList_Upload((int *)course_00);
    Vol_ReadFileAsync(file, course_00);
    Course_Relocate(course_00);
    Race_LoadSky(race, (unsigned int)*(unsigned short *)pCVar1->unkA);
}

#ifdef NON_MATCHING
// Uploads the HUD's cluts and images, and the lights picked by the course's flags.
void Race_LoadCourseTims(void *race)
{
    char *buf = (char *)race + 0xe5700;
    RaceWorld *world = &((RaceMem *)race)->world;
    int clutX = 0x170;
    int kind;

    VolId_ReadFile((VolBootId)0x6d, buf);
    Hud_LoadCluts((char *)race + 0x450c, (int)buf);
    {
        char *tim = buf + *(int *)(buf + 0x10);
        short *clut = (short *)(tim + 8);
        short *image;

        clut[2] = clutX;
        clut[3] = 0x1fd;
        image = (short *)((char *)clut + *(int *)(tim + 8));
        image[2] = 0x260;
        image[3] = 0x40;
        Tim_LoadClutAndImage(tim, 0, 0);
    }
    {
        CourseEntry *course = Course_Get(WORLD_COURSE(world));
        unsigned short flags = course->flags;

        kind = 3;
        if (flags & 1) {
            kind = 5;
        }
        if (flags & 2) {
            kind = 4;
        }
    }
    {
        char *pack = buf + kind * 12;
        char *tim = buf + *(int *)(pack + 0x10);
        short *clut = (short *)(tim + 8);
        short *image;

        clut[2] = clutX;
        clut[3] = 0x1ff;
        image = (short *)((char *)clut + *(int *)(tim + 8));
        image[2] = 0x240;
        image[3] = 0;
        Tim_LoadClutAndImage(tim, 0, 0);
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/courseload", Race_LoadCourseTims);
#endif

#ifdef NON_MATCHING
// Uploads the event's sponsor banners, in random order, into the banner slots.
void Race_LoadBanners(void *race, int index)
{
    CourseEntry *course = Course_Get(index);
    signed char order[8];
    unsigned int seed;
    char *name;
    char *buf;
    char *entry;
    char *found;
    char *tim;
    char *head;
    int i;
    int group;
    BannerSlot *slot;

    if (course->flags & 0x40) {
        return;
    }
    if (RaceSetup_Get()->game != 0) {
        name = RaceSetup_Get()->eventName;
    } else {
        name = D_8002F1B8;
    }
    if (*name == 0) {
        return;
    }
    buf = (char *)race + 0xe5700;
    VolId_ReadFileAsync((VolBootId)7, buf);
    head = buf;
    entry = buf + 0x10;
    found = 0;
    if (((unsigned short *)buf)[1] != 0) {
        int stride = ((unsigned short *)buf)[0] * 4 + 0x10;

        i = 0;
        do {
            if (strcmp(entry, name) == 0) {
                found = entry;
            }
            i++;
            entry += stride;
        } while (i < ((unsigned short *)buf)[1]);
    }
    if (found == 0) {
        return;
    }
    if (D_800A951C == 0) {
        RACE_SEED(RaceSetup_Get()) = Irq_WaitVSyncs(-1);
    }
    tim = entry;
    {
        char *pic = tim + 0x1f60;
        int total = 0;

        seed = RACE_SEED(RaceSetup_Get());
        for (slot = g_BannerSlots; slot < g_BannerSlots + 5; slot++) {
            int stride = slot->size;
            int count = slot->count;
            int used = 0;
            int n;
            char *pix = pic + 0xe;
            unsigned int *chance = (unsigned int *)(found + total * 4);

            memset(order, -1, 8);
            for (i = 0; i < count; i++) {
                unsigned int r;

                do {
                    r = Rand_Next(&seed) % count;
                } while (order[r] != -1);
                order[r] = i;
            }
            n = *(unsigned short *)(head + 4);
            for (i = 0; i < n; i++) {
                if (used < count) {
                    unsigned int r = Rand_Next(&seed) & 0xfff;
                    unsigned short *c = (unsigned short *)((char *)chance + 0x10);

                    if (c[1] != 0x13 && c[0] >= (int)r) {
                        Banner_Place(pic, slot, order[used]);
                        used++;
                        Tim_LoadClutAndImage(pic, 0, 0);
                    }
                }
                pic += stride;
                pix += stride;
                total++;
                chance++;
            }
            while (used < count) {
                Banner_Place(tim, slot, order[used]);
                used++;
                Tim_LoadClutAndImage(tim, 0, 0);
            }
            tim += stride;
            head += 2;
        }
    }
}
#else
INCLUDE_ASM("asm/sim/ovl0/nonmatchings/courseload", Race_LoadBanners);
#endif

// Once per race context: loads the course, its TIMs and the banners.
void Race_LoadCourseOnce(RaceMem *race)
{
    if (*(char *)((int)race + 0x11d700) == 0) {
        RaceWorld *world = &((RaceMem *)race)->world;

        *(unsigned char *)((int)race + 0x11d700) = 1;
        Race_LoadCourse(race, WORLD_COURSE(world));
        Race_LoadCourseTims(race);
        Race_LoadBanners(race, WORLD_COURSE(world));
    }
}
}
