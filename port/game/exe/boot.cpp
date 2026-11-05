// Boot code 80010000-80010a24 (boot): opens GT2.OVL and GT2.VOL's directory, sets up a new
// game, and starts the hardware. An overlay loaded at 0x80010000 overwrites all of this.
// common.h declares this symbol as the overlays' first function, at the same address.
#define Overlay_OpenFile Overlay_FirstFunction
#include "common.h"
#undef Overlay_OpenFile
#include "gamestate.h"
#include "pad.h"

// The default controls of a controller kind, 11 bytes in a 12-byte slot.
struct ButtonMap {
    unsigned char keys[11];
};

// A copy of 0xc000 bytes (the VOL's header).
struct VolHeaderBytes {
    unsigned char bytes[0xc000];
};

// A copy of 0x8c000 bytes (the VOL's directory and resident files).
struct VolDataBytes {
    unsigned char bytes[0x8c000];
};

// The game's flow between overlays.
struct FlowState {
    unsigned char introShown;   // 0x0
    unsigned char homeOverlay;  // 0x1
    unsigned char gameMode;     // 0x2
    unsigned char unk3[5];
    int gtMenuPage;             // 0x8
    short gtMenuBgm;            // 0xc
    short demoReplayIndex;      // 0xe
    unsigned char unk10;        // 0x10
};

extern "C" {
extern char D_80011D30[];       // "gt2.ovl"
extern unsigned int g_VolLba;
extern int g_OvlLba;
extern int g_OvlSize;
extern OverlayPack g_OverlayFile;
extern VolDirEntry g_VolDir[];
extern char *g_VolBootNames[];
extern unsigned short g_VolBootIds[248];
extern VolHeader g_VolHeader;
extern int g_DemoRaceCount;
extern ButtonMap D_80091570;
extern ButtonMap D_8009157C;
extern ButtonMap D_80091588;
extern ButtonMap D_80091594;
extern NeGconCalib D_800A6ED8;
extern CourseInfo g_CourseInfo;
extern unsigned char g_RaceEventParams[];
extern short g_InitRange[2];
extern unsigned char g_Unk801c93c3;
extern FlowState g_Flow_IntroShown;
extern unsigned int g_RandomSeed;
extern volatile int g_BootVSyncCount;

void Overlay_OpenFile(OverlayPack *pack);

// Finds GT2.OVL, keeps its position, and reads the whole file to RAM if there is room for
// it (its table otherwise, and overlays are then read from the disc).
// The game loads pack->unk0 again after storing it, where GCC keeps the value, and that moves
// the registers. Its C++ otherwise has the game's shape.
void Overlay_OpenFile(OverlayPack *pack)
{
    IsoDirRecord *rec = Iso_FindBootPath(D_80011D30);
    int lba;
    int size;
    char *data;

    pack->unk0 = rec->lba - g_VolLba;
    size = rec->size;
    pack->unk4 = size;
    lba = g_VolLba + pack->unk0;
    g_OvlLba = lba;
    g_OvlSize = size;
    data = (char *)malloc(size);
    pack->data = data;
    if (data == 0) {
        Cd_ReadSync(pack->entries, lba, 0x30);
    } else {
        Cd_ReadSync(data, lba, pack->unk4);
    }
    if (pack->data != 0) {
        memcpy(pack->entries, pack->data, 0x30);
    }
}

void Overlay_Init()
{
    Overlay_OpenFile(&g_OverlayFile);
}

// Resolves a path in GT2.VOL's directory, from the root if it starts with '/', else from
// dir (0 for the root).
VolDirEntry *Vol_FindEntry(char *path, VolDirEntry *dir)
{
    char name[256];
    VolDirEntry *root = g_VolDir;
    int first = *(signed char *)path;
    int second = path[1];

    if (first == '/') {
        path++;
        if (second == 0) {
            return root;
        }
        dir = 0;
    }
    if (dir == 0) {
        dir = root;
    }
    while (1) {
        char *slash = strchr(path, '/');
        int n;

        if (slash == 0) {
            break;
        }
        n = slash - path;
        strncpy(name, path, n);
        name[n] = 0;
        dir = Vol_FindEntry(name, dir);
        if (dir == 0) {
            return 0;
        }
        path = slash + 1;
        dir = g_VolDir + dir->index;
    }
    while (1) {
        if (strcmp(dir->name, path) == 0) {
            return dir;
        }
        if (dir->flags & 0x80) {
            return 0;
        }
        dir++;
    }
}

// Counts the files of a directory, from its first entry.
int func_800101FC(VolDirEntry *entry)
{
    int count = 0;
    int last;

    do {
        unsigned int flags = entry->flags;

        if (!(flags & 1)) {
            count++;
        }
        last = flags & 0x80;
        entry++;
    } while (last == 0);
    return count;
}

// Sets g_VolBootIds: the file index of each path in g_VolBootNames, 0xffff if missing. For
// a directory, that of its first file.
void Vol_ResolveBootFiles(void)
{
    char **names = g_VolBootNames;
    unsigned short *ids = g_VolBootIds;

    while (*names != 0) {
        VolDirEntry *entry = Vol_FindEntry(*names, 0);

        *ids = 0xffff;
        if (entry != 0) {
            if (entry->flags & 1) {
                VolDirEntry *children = &g_VolDir[entry->index];

                entry = children + 1;
            }
            *ids = entry->index;
        }
        names++;
        ids++;
    }
}

// Reads the start of GT2.VOL, keeps the header and moves VOL file 1 (the directory, then
// resident data) down to g_VolDir.
void Vol_Init()
{
    VolDirEntry *dir = g_VolDir;

    Vol_ReadSectors(dir, 0, 0x8c000);
    *(VolHeaderBytes *)&g_VolHeader = *(VolHeaderBytes *)dir;
    {
        char *base = (char *)g_VolDir;

        *(VolDataBytes *)base = *(VolDataBytes *)((g_VolHeader.toc[1] & ~0x7ff) + base);
    }
    Vol_ResolveBootFiles();
}

// The number of demo races: the files between two boot ids.
void Game_InitDemoCount(void)
{
    g_DemoRaceCount = g_VolBootIds[0xe5] - g_VolBootIds[0xe4] - 1;
}

// The options, controls, garages and records of a new game.
// The first loop matches. The game keeps 1 in $a0 with a copy in $v1, gets the options'
// address by adding that 1 to g_GameState, and adds the base after the offset in the other loops.
void GameState_InitNewGame(void)
{
    GameState *gs = &g_GameState;
    int i;

    memset(gs, 0, 0xb6);
    for (i = 0; i < 2; i++) {
        PortConfig *port = &gs->ports[i];

        *(ButtonMap *)port->buttonMaps[0] = D_80091570;
        *(ButtonMap *)port->buttonMaps[1] = D_8009157C;
        *(ButtonMap *)port->buttonMaps[2] = D_80091588;
        *(ButtonMap *)port->buttonMaps[3] = D_80091594;
        port->negconCalib = D_800A6ED8;
    }
    gs->options2[5] = 0xf0;
    gs->options2[6] = 0xc0;
    gs->options2[3] = 1;
    gs->options2[4] = 1;
    gs->unkB5 = 1;
    gs->language = 1;
    gs->options2[0] = 0;
    gs->options2[1] = 0;
    gs->options[0] = 0;
    gs->options[1] = 2;
    gs->options[2] = 1;
    gs->options[3] = 0;
    gs->options[4] = 2;
    gs->options[6] = 1;
    for (i = 0; i < 2; i++) {
        Garage *garage = gs->garages + i;

        Garage_InitNew(garage);
    }
    for (i = 0; i < g_CourseInfo.count; i++) {
        CourseRecord *record = gs->courseRecords + i;

        TimeRecord_Init((TimeRecord *)record);
    }
    for (i = 0; i < 6; i++) {
        int j;
        LicenseTest *test = gs->licenses[i];

        for (j = 0; j < 10; j++) {
            LicenseTest *t = test;

            LicenseTest_Init(t);
            test++;
        }
    }
    MachineTestRecords_Init(&gs->machineTests[0]);
    MachineTestRecords_Init(&gs->machineTests[1]);
    MachineTestRecords_Init(&gs->machineTests[2]);
    PlayerStats_Init(&gs->stats);
}

void Garage_InitNew(Garage *garage)
{
    garage->currentCar = -1;
    garage->carCount = 0;
    garage->credits = 10000;
    garage->driverName[0] = 0;
}

void PlayerStats_Init(PlayerStats *stats)
{
    memset(stats, 0, 0x160);
    stats->day = 1;
}

void Game_InitRaceEventParams(void)
{
    memset(g_RaceEventParams, 0, 0x40);
    g_InitRange[0] = -0x40;
    g_InitRange[1] = 0x40;
}

void Game_InitUnk801c93c3(void)
{
    g_Unk801c93c3 = 0x60;
}

void Flow_Reset(void)
{
    g_Flow_IntroShown.introShown = 0;
    g_Flow_IntroShown.homeOverlay = 0;
    g_Flow_IntroShown.gameMode = 0;
    g_Flow_IntroShown.gtMenuPage = 0;
    g_Flow_IntroShown.gtMenuBgm = -1;
    g_Flow_IntroShown.demoReplayIndex = 0;
    g_Flow_IntroShown.unk10 = 0;
}

// Indexes the VOL's cars and courses, loads the common sounds and starts a new game.
void Game_InitData()
{
    Vol_IndexCarData();
    Vol_LoadCourseInfo();
    Sound_LoadCommonBank();
    GameState_InitNewGame();
    Game_InitRaceEventParams();
    Game_InitUnk801c93c3();
    Flow_Reset();
    Game_InitDemoCount();
}

// Starts root counter 2 (system clock / 8, free running) and seeds g_RandomSeed from the
// four counters.
void Boot_InitRandomSeed(void)
{
    volatile unsigned short *timer2 = (volatile unsigned short *)0x1f801120;
    volatile unsigned short *timer0 = (volatile unsigned short *)0x1f801100;
    volatile unsigned short *timer1 = (volatile unsigned short *)0x1f801110;
    volatile unsigned short *timer3 = (volatile unsigned short *)0x1f801130;

    timer2[2] = 0;
    timer2[4] = 0xffff;
    timer2[2] = 0x248;
    g_RandomSeed = *timer0 ^ *timer1 << 4 ^ *timer2 << 8 ^ *timer3 << 12;
}

void Boot_VSyncHandler(void)
{
    EnableDisplay(1);
    g_BootVSyncCount++;
}

// Enables the display from a VSync handler, and waits 4 vblanks.
void Boot_WaitVSyncs(void)
{
    Psyq_VSyncCallback(Boot_VSyncHandler);
    while (g_BootVSyncCount < 4) {
    }
    Psyq_VSyncCallback(0);
}

// Called from Sys_Init: clears the BSS and starts the interrupts, CD, sound, memory cards,
// pads, GPU, MDEC, GTE and the random seed.
void Boot_InitHardware(void)
{
    Overlay_ClearBss();
    ResetCallback();
    Boot_WaitVSyncs();
    CdInit();
    Spu_Reset();
    InitCARD(0);
    StartCARD();
    _bu_init();
    {
        PadSystem *sys = &g_PadSystem;

        PadInitDirect((void *)sys->raw, (void *)(sys->raw + 1));
    }
    Gpu_Reset();
    DecDCTReset(0);
    InitGeom();
    Boot_InitRandomSeed();
}

}
