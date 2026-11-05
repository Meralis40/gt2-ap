// Types of the OVL4 code, exported from the Ghidra project (DataTypeWriter) and
// adapted for cc1plus as resident.h is (enums as sized integers, C++ keywords
// renamed), then the types declared with the decompiled OVL4 files. The
// resident types are in resident.h. See docs/decomp.md.
#ifndef OVL4_H
#define OVL4_H

#include "resident.h"

typedef struct CarSpecStrings CarSpecStrings, *PCarSpecStrings;

struct CarSpecStrings { /* Per-language unit/format strings for car specs. 7 records of 0x88 bytes are stored gzip-compressed in OVL4 (g_CarSpecStringsGz); the one for the current language is copied to g_CarSpecStrings. */
    char emptyList[16]; /* 'Nothing' / 'N/A': empty car list */
    char owned[12]; /* 'Already' / 'Purchased' / 'Bought' */
    char powerApprox[6]; /* '~%dps' / '~%dhp' */
    char noLimit[11]; /* 'no limited' / 'free' */
    char powerAtRpm[13]; /* '%dps / %drpm' */
    char torque[14]; /* '%d.%dkgm / ' or '%d.%dlb-ft / ' */
    char speed[12]; /* '%d.%02dkm/h' / '%d.%02dmph' */
    char rpm[6]; /* '%drpm' */
    char dimension[5]; /* '%dmm' */
    char displacement[5]; /* '%dcc' */
    char drivetrainFR[5];
    char drivetrainFF[5];
    char drivetrain4WD[4];
    char drivetrainMR[4];
    char drivetrainRR[5];
    char power[5]; /* '%dps' / '%dhp' */
    char displacementRotary[8]; /* '%dx%dcc' */
};

typedef struct UsedCarEntry UsedCarEntry, *PUsedCarEntry;

struct UsedCarEntry {
    uint carId;
    uint price:24; /* credits */
    uint colorCode:8; /* colour letter, see CarInfoA_GetColorRgb */
};

typedef struct UsedCarPeriod UsedCarPeriod, *PUsedCarPeriod;

typedef struct UsedCarSlot UsedCarSlot, *PUsedCarSlot;

struct UsedCarSlot {
    ushort offset; /* from the start of the UsedCarPeriod */
    ushort count;
};

struct UsedCarPeriod { /* One 10-day period of the used car lot. Copied to g_OVL4_StorageUsedCar. */
    struct UsedCarSlot makers[39]; /* indexed by manufacturer ID */
    struct UsedCarEntry cars[272]; /* 272 = largest period in .usedcar_usa (2332 bytes) */
};

typedef struct LangMenuFiles LangMenuFiles, *PLangMenuFiles;


struct LangMenuFiles { /* /gtmenu/<lang>/ files. Rows: 0 JPN, 1 USA, 2 ENG, 3 FRA, 4 GER, 5 ITA, 6 SPA */
    VolBootId16 gtmenudatDat;
    VolBootId16 iconimg;
    VolBootId16 solodata;
    VolBootId16 gtmenudatIdx;
};


typedef struct BattleCarStat BattleCarStat, *PBattleCarStat;

struct BattleCarStat { /* One GT race-data car, for the free-race opponent pick. */
    uchar powerDiv10;
    uchar pad01;
    ushort score; /* weight*300000/power^2 (Battle_WeightPowerScore), max 0xfffe */
    uchar flags; /* bit0 racing mod in the default setup, bits1-3 drivetrain, bit4 no turbo */
    uchar pad05;
};

typedef struct BattleCarTable BattleCarTable, *PBattleCarTable;

struct BattleCarTable { /* Work table of GTBattleMenu (in g_RaceCarWorkAlt): stats of every car, then the indices sorted by score. */
    struct BattleCarStat cars[1500];
    short loaded; /* cars processed so far (one per frame) */
    short count; /* number of cars (GT race data +0x16) */
    short sorted[1500]; /* car indices by increasing score */
};

typedef struct BgPicStream BgPicStream, *PBgPicStream;

struct BgPicStream {
    void *sectorBuf;
    void *header;
    void *lineBuf;
    uint bytesDone;
    uint startOffset;
    uint size;
    uint lineFill;
    short vramX;
    short vramY;
    short vramW;
    short vramH;
    uchar state;
    uchar started;
    uchar pad26[2];
};

typedef struct CarCamera CarCamera, *PCarCamera;

struct CarCamera {
    uchar pad00[156];
    int field9C;
    int fieldA0;
    int fieldA4;
    short distance;
    ushort yaw;
    short fieldAC;
    uchar padAE[2];
    int fieldB0;
    int fieldB4;
    int fieldB8;
    short screen[4]; /* copied to the camera's first 4 shorts by CarPreview_DrawModel */
    short frustum[4]; /* left, right, top, bottom for Camera_SetFrustum */
    short fieldCC;
    short fieldCE;
    signed char fieldD0;
    signed char fieldD1;
    uchar padD2[2];
    uint ambient;
};

typedef struct CarPreview CarPreview, *PCarPreview;

typedef struct CarSpecSheet CarSpecSheet, *PCarSpecSheet;

struct CarSpecSheet { /* Spec values of the preview car (CarPreview.spec), filled by FUN_OVL4__8001afa8 (dealer: car tables) or FUN_OVL4__8001b10c (owned car: setup and computed perf). Field meanings from MenuPage_DrawElements. */
    ushort modelYear; /* kind 0x9a: shown as '%d ('----' if 0); car entry byte +0x41 (was drivetrain) */
    ushort pad02;
    int length; /* mm, kind 0xb2 (was body08) */
    int width; /* mm, kind 0xb3 (was body0a) */
    int height; /* mm, kind 0xb4 (was t05_1a) */
    int weight; /* kg, kinds 0xb5 / 0x4c (shown in lb) */
    int displacement; /* cc, kind 0xb6; bits 13-15 = rotor count for rotary engines (was engine2c) */
    int drivetrain; /* index into g_DrivetrainNames / g_GTIcon_Drivetrain (kinds 0xb7, 0x97) (was t0d_08) */
    int engineStrA; /* game string id, kind 0xb8 first part (was engine04) */
    int engineStrB; /* game string id, not drawn by MenuPage_DrawElements (was engine08) */
    int engineStrC; /* game string id, kind 0xb8 second part (was engine06) */
    int price; /* new price; kind 0xd0 shows price / 4 (sale value) (was typeE44) */
    int power; /* ps, kinds 0xb9 / 0x4a (shown in hp: *1000/1014) */
    int powerRpm; /* rpm of max power, kind 0xb9 (was torque) */
    int torque; /* kgm x 10, kind 0xba (shown in lb-ft) (was perf2) */
    int torqueRpmStr; /* game string id of the torque rpm text, -1 = use rpmText (was engine34) */
    ushort rpmText[16]; /* torque rpm text built by CarPreview_SpecFromCar for owned cars */
};

struct CarPreview {
    short x;
    short y;
    int firstFile;
    int logoFile;
    int carId;
    uchar vramPage;
    signed char colorIdx;
    uchar pad12[2];
    int dirt;
    short logoW;
    short logoH;
    signed char loaded;
    signed char isCurrentCar;
    uchar pad1e[2];
    void *modelBuf;
    void *texBuf;
    void *wheelBuf;
    uchar model2c[312];
    signed char colorDirty;
    uchar pad165;
    uchar carParams[448];
    ushort perf[55];
    struct CarSpecSheet spec;
    uchar pad3f0[96];
};

typedef struct EventPrizeInfo EventPrizeInfo, *PEventPrizeInfo;

struct EventPrizeInfo { /* Prizes of the event about to be run (Event_LoadPrizeInfo); read by OVL0 Race_AwardChampionshipPrize. */
    int championshipPrize; /* event entry +0x98 x100 */
    int prizes[6]; /* entry +0x78..+0x82 x100, by finishing position */
    uchar prizeCarCount;
    uchar pad1d[3];
    struct Car prizeCars[4]; /* from entry +0x84, random colour */
};

typedef struct MenuBgmBank MenuBgmBank, *PMenuBgmBank;

struct MenuBgmBank {
    void *buffer;
    uchar pad04[8];
    struct SeqPlayer seq;
    void *seqData;
    uchar playing;
    uchar pad2b9[3];
};

typedef struct OpponentPool OpponentPool, *POpponentPool;

struct OpponentPool { /* Car indices (GT race data car table) the AI opponents are drawn from (RaceCar_InitFromPool). Filled by BattleTable_PickOpponents for free races. */
    int count;
    short carIndex[80];
};

typedef struct ShopTxn ShopTxn, *PShopTxn;

struct ShopTxn { /* Pending shop transaction (g_ShopTxn). type: 0 none, 1 buy car, 2 bought car (make current), 3 buy part, 4 install owned part, 5 wheels, 6 sell car, 7 select car. */
    short type;
    ushort pad02;
    int sellSlot;
    int selectSlot;
    int carCode;
    int color; /* colour code (buy car); also passed to Tune_InstallPart */
    int partSlot;
    int part;
    int powerNow;
    int powerWithPart;
    int partOwned;
    int wheelSlot;
    int wheelCode;
    int price;
};


enum { PART_ASC=0, PART_SPORTS_BRAKES=1, PART_BRAKE_BALANCE=2, PART_CLUTCH_SINGLE=3, PART_CLUTCH_TWIN=4, PART_CLUTCH_TRIPLE=5, PART_PERF_CHIP=6, PART_DISPLACEMENT=7, PART_DISPLACEMENT_2=8, PART_ENGINE_BALANCING=9, PART_FLYWHEEL_SPORTS=10, PART_FLYWHEEL_SEMI=11, PART_FLYWHEEL_RACING=12, PART_TRANS_SPORTS=13, PART_TRANS_SEMI=14, PART_TRANS_FULL=15, PART_INTERCOOLER_SPORTS=16, PART_INTERCOOLER_RACING=17, PART_WEIGHT_RED_1=18, PART_WEIGHT_RED_2=19, PART_WEIGHT_RED_3=20, PART_LSD_1WAY=21, PART_LSD_2WAY=22, PART_LSD_15WAY=23, PART_LSD_FULL=24, PART_YAW_CONTROL=25, PART_MUFFLER_SPORTS=26, PART_MUFFLER_SEMI=27, PART_MUFFLER_RACING=28, PART_NA_TUNE_1=29, PART_NA_TUNE_2=30, PART_NA_TUNE_3=31, PART_PORT_POLISH=32, PART_CARBON_DRIVESHAFT=33, PART_RACING_MOD=34, PART_SUSP_SPORTS=35, PART_SUSP_SEMI=36, PART_SUSP_FULL=37, PART_TCS=38, PART_TIRES_SPORTS=39, PART_TIRES_HARD=40, PART_TIRES_MEDIUM=41, PART_TIRES_SOFT=42, PART_TIRES_SUPER_SOFT=43, PART_TIRES_SIMULATION=44, PART_TIRES_DIRT=45, PART_TURBO_1=46, PART_TURBO_2=47, PART_TURBO_3=48, PART_TURBO_4=49 };
typedef unsigned int PartId;

enum { LICENSE_S=0, LICENSE_IA=1, LICENSE_IB=2, LICENSE_IC=3, LICENSE_A=4, LICENSE_B=5, LICENSE_NONE=6 };
typedef unsigned int LicenseId;

typedef struct MapCursor MapCursor, *PMapCursor;

typedef struct GTMenuView GTMenuView, *PGTMenuView;


typedef struct MenuElement MenuElement, *PMenuElement;

// The resident screen loops, as C structs. A file that includes viewloop.h first
// gets its classes instead.
struct MapCursor { /* GT mode map cursor (tentative); init FUN_OVL4__8001e22c */
    short drawX; /* smoothed draw position (spring towards x) */
    short drawY;
    signed char moveFrames; /* 8 when a move to a new hotspot starts */
    signed char lastDir; /* the move's direction, 1 after a bump */
    signed char dir; /* last direction pressed: 2 up, 3 down, 4 left, 5 right, 6 up-right, 7 up-left, 8 down-right, 9 down-left */
    undefined field5_0x7;
    short prevX;
    short prevY;
    short x; /* centre of the target hotspot */
    short y;
    int velX; /* 8.8 fixed point */
    int velY;
    int posX; /* 8.8 fixed point */
    int posY;
    signed char blink; /* counts 0..64 */
    undefined field15_0x21;
    undefined field16_0x22;
    undefined field17_0x23;
    struct MenuElement *hotspot; /* element under the cursor (target of the current move) */
};

struct GTMenuView { /* GT mode menu pages and widgets (tentative); init FUN_OVL4__8001d118. Sub-objects at +0x28 (FUN_OVL4__8001aaf4) and +0x478 (FUN_OVL4__8001d090) */
    signed char loaded; /* set by MenuPage_Enter */
    undefined field1_0x1;
    undefined field2_0x2;
    undefined field3_0x3;
    int page; /* initial value from 0x801ef5f8; passed to MenuPage_Enter */
    int backPage; /* page for square/triangle (MenuView_GetBackPage); from the page footer */
    int prevBackPage; /* previous backPage (MenuView_GetPrevBackPage) */
    int carBaseId; /* car shown by MenuView_ShowCar (model) */
    int carId; /* car shown by MenuView_ShowCar (specs; key of MenuView_GetCarInfoPage) */
    void *bgListTail; /* last packet of the background display list (MenuPage_BuildBackground), linked into the OT each frame */
    pointer assets0;
    pointer assets1;
    pointer assets2;
    struct CarPreview preview;
    struct CarCamera camera;
    short bgmTrack; /* menu music track of the current page (MenuBgm_Request), saved in 0x801ef5fc on exit */
    signed char carWash; /* car wash spin in progress (MenuView_StartCarWash) */
    undefined field17_0x553;
    short carWashTimer; /* frames left of the car wash spin (0x40) */
    undefined field19_0x556;
    undefined field20_0x557;
};

// struct GTMenu is the class in gtmenu.h.
struct MenuElement { /* Menu page element (0x4c bytes, gtmenudat page data): hotspot rectangle, flags, action data. */
    short x0;
    short y0;
    short x1;
    short y1;
    uint flags; /* low 16 bits kind/action; bit 18 license-gated (licenseReq), bits 19..23 hidden/inactive variants, bit 24 selectable, bit 25 default cursor element, bit 27 race entry (code at target), bit 28 wheel (code at target), bit 29 hide tile price, bit 30 race without entry check (exit 3), bit 31 alternate colour */
    uint target; /* page id to go to, or the start of an ASCII race/wheel code */
    int arg10; /* car code for kind 6 (dealer car tile) */
    uchar data14[54];
    uchar licenseReq; /* bit-18 elements: 0 = IA license, 1 = S license */
    uchar prizePlace; /* flag bit 22: 1-based place whose prize is drawn (MenuPage_DrawElements) */
};



typedef struct MenuCarList MenuCarList, *PMenuCarList;

struct MenuCarList { /* garage or used car list widget (g_GarageList, g_UsedCarList) */
    short count;
    short selected;
    struct MenuElement *element;
};

// Types declared with the decompiled OVL4 code, by the file that first
// needed them.

// ovl4/battletable.cpp

// The player's car (gtbattlemenu.cpp's g_Battle_Player).
struct BattlePlayer {
    short power;
    short weight;
    signed char racingMod;
    unsigned char pad05[3];
};

// ovl4/carpreview.cpp

// The buffers and position of the preview (CarPreview_Init).
struct PreviewBuffers {
    void *modelBuf;
    void *texBuf;
    void *wheelBuf;
};

struct CarPreviewConfig {
    PreviewBuffers bufs;
    unsigned char vramPage;
    unsigned char pad0d;
    short x;
    short y;
};

// A TIM's CLUT block (the logo, at CarModelHeader + 8) and the image block after it.
struct TimBlock {
    unsigned int len;
    short x;
    short y;
    short w;
    short h;
};

// ovl4/carwork2.cpp

// A row of the installed parts summary: the tune slot (-1 ends the list), the part
// of its stage 1, and the text offset.
struct TuneSummaryRow {
    short slot;
    short nameBase;
    short dx;
    short dy;
};

// ovl4/eventrules.cpp

// The entry rules of an event, read through copies.
struct EventFlags {
    unsigned short bits;

    int Dirt() { return bits & 1; }
    // 1-6 = license B, A, IC, IB, IA, S needed
    int License() { return (bits & 0xe) >> 1; }
    // 1 FF, 2 FR, 3 MR, 4 RR, 5 4WD needed (Car.stat94 >> 13)
    int Drivetrain() { return (bits & 0x70) >> 4; }
    // 1 = no turbo, 2 = turbo
    int Turbo() { return (bits & 0x180) >> 7; }
    // 1 = no racing modification, 2 = racing modification, 3 = stock
    int RacingMod() { return (bits & 0x600) >> 9; }
};

// An event's prizes and entry rules, built from the race data by EventRules_BuildAll.
struct EventRules {
    int championshipPrize;
    int prizes[6];                  // 0x4, by place
    EventFlags flags;               // 0x1c
    short maxPower;                 // 0x1e
    unsigned short allowedCarsStart; // 0x20, in g_EventAllowedCars
    unsigned char allowedCarsCount; // 0x22, 0 = any car

    EventFlags Flags() { return flags; }
};

// A replay file of an event's ghost.
struct GhostEventFile {
    char *eventCode;
    VolBootId fileId;
};

// ovl4/gtfont.cpp

// A glyph of the GT mode fonts: its character code and where it sits.
struct GTFontGlyph {
    unsigned short code;    // 0x0, sorted, so that codes from 0x100 are searched
    short xoff;             // 0x2
    short yoff;             // 0x4
    short advance;          // 0x6
};

// A sprite of the GT mode menus (a font glyph, the map cursor): the SPRT's
// texture words and its texture page.
struct SpriteDesc {
    unsigned int uvClut;    // 0x0
    unsigned int wh;        // 0x4
    unsigned short tpage;   // 0x8
    short unkA;
};

// ovl4/gtmenu.cpp

// Car preview setup that Enter passes to MenuView_Init.
struct GTMenuPreviewConfig {
    void *buf;
    unsigned char *work;
    void *buf2;
    short unk0c;
    short unk0e;
    short unk10;
};

// Where Enter takes the preview's unk0e and unk10 from: the high half of the
// first word, and the halfword at +8.
struct GTMenuPreviewParams {
    unsigned int unk0;
    unsigned int unk4;
    unsigned short unk8;
};

// ovl4/gtracedata.cpp

// An entry of game-data table 0x1d (the wheels).
struct WheelEntry {
    unsigned int code;              // 0x0, 0 for the car's own wheels
    unsigned short unk4;
    unsigned char altVariant;       // 0x6, for a racing modified car's own wheels
    unsigned char variant;          // 0x7
};

// ovl4/menudata.cpp

// One language's car spec strings, as a block for the struct copy.
struct CarSpecRecord {
    char text[0x88];
};

// One CD sector, as a block for the struct copy.
struct SectorData {
    char data[0x800];
};

// gtmenu/solodata's car page table: carCount, then the pairs sorted by car id.
struct CarPage {
    unsigned int carId;
    int pageId;
};

// ovl4/menuview.cpp

// The three menu buffers (GTMenuView.assets0..2), copied as one.
struct ViewAssets {
    void *buf[3];
};

// The flow state at 0x801ef5f0: the menu page and music track to come back to.
struct FlowMenuState {
    signed char introShown;
    signed char homeOverlay;    // g_Flow_HomeOverlay
    signed char gameMode;       // g_Flow_GameMode
    signed char mainMenuChoice;
    signed char arcadeMenuExit;
    signed char gtMenuExit;     // how GTMenu left: 2 start the event, 3 without the player car, 4 to OVL1's entry
    unsigned char pad06[2];
    int page;
    short bgmTrack;
    short demoReplayIndex;      // g_Flow_DemoReplayIndex: the next demo race or replay
};

// A cursor direction's unit vector (4.12), g_MapCursor_DirVectors.
struct DirVector {
    short x;
    short y;
};

// ovl4/license.cpp

// A license test car (License_BuildTestCarTable): name, max power and drivetrain.
struct LicenseTestCar {
    char name[64];
    unsigned short maxPower;
    unsigned short drivetrain;
};

// ovl4/garagecars.cpp

// A car of the machine test list: a record car or the player car.
struct MachineTestCar {
    unsigned int carId;
    char name[0x44];
};

// The cars of a machine test (MachineTest_BuildCarList): the record cars, then
// the player car.
struct MachineTestCarList {
    short count;
    short pad;
    MachineTestCar cars[9];
};

// ovl4/usedcar.cpp

// The used car file: a header, then the offsets of the 60 periods and the end.
struct UsedCarFile {
    int header[2];
    int offsets[61];
};

#endif
