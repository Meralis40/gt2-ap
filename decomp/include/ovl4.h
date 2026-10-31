// Types of the OVL4 code, exported from the Ghidra project (DataTypeWriter) and
// adapted for cc1plus as resident.h is: enums as sized integers, C++ keywords
// renamed, bool as a byte. Only the types that the decompiled OVL4 files use.
// See docs/decomp.md.
#ifndef OVL4_H
#define OVL4_H

#define bool unsigned char
typedef void *pointer;
typedef unsigned char   undefined;

typedef unsigned char    byte;
typedef unsigned int    dword;
typedef long long    longlong;
typedef signed char    schar;
typedef unsigned char    uchar;
typedef unsigned int    uint;
typedef unsigned long    ulong;
typedef unsigned char    undefined1;
typedef unsigned short    undefined2;
typedef unsigned int    undefined4;
typedef unsigned short    ushort;
typedef unsigned short    word;
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

typedef struct CarWork CarWork, *PCarWork;

typedef struct CarSetup CarSetup, *PCarSetup;

struct CarSetup {
    uint wheel; /* Wheel code from table 0x1d entry (see Wheel_ParseCode) | variant << 8. Rebuilt by CarSetup_SetWheels */
    short idx_t00; /* Part index into game-data table 0x00; tune slot 1, part 1 */
    short idx_t01; /* Part index into game-data table 0x01; tune slot 2, part 2 */
    short idx_08; /* From car entry +0x08; no tune part writes it */
    short idx_0a; /* From car entry +0x0a; no tune part writes it */
    short idx_0c; /* From car entry +0x10; no tune part writes it */
    short idx_t0d; /* Part index into game-data table 0x0d; no tune part writes it */
    short idx_t11; /* Part index into game-data table 0x11 (transmission); tune slot 6, parts 13-15 */
    short idx_t12; /* Part index into game-data table 0x12 (suspension); tune slot 0, parts 35-37 */
    short idx_t15; /* Part index into game-data table 0x15; tune slot 0x16, parts 21-25 (5 stages) */
    short idx_t16_front; /* Part index into game-data table 0x16; tune slot 3, parts 39-45 (7 stages): front tyres */
    short idx_t16_rear; /* Part index into game-data table 0x16; tune slot 4, parts 39-45: rear tyres */
    short idx_t04; /* Part index into game-data table 0x04; tune slot 0x12, parts 18-20: weight reduction stages 1-3 (each requires the previous) */
    short idx_t05; /* Part index into game-data table 0x05; tune slot 0x13, part 34: racing modification (requires part 20; also changes baseCarId) */
    short idx_t07; /* Part index into game-data table 0x07; tune slot 0xb, part 32 */
    short idx_t08; /* Part index into game-data table 0x08; tune slot 0xc, part 9 */
    short idx_t09; /* Part index into game-data table 0x09; tune slot 0xe, parts 7-8 */
    short idx_t0a; /* Part index into game-data table 0x0a; tune slot 10, part 6 */
    short idx_t0b; /* Part index into game-data table 0x0b; tune slot 0xd, parts 29-31 (3 stages) */
    short idx_t0c; /* Part index into game-data table 0x0c; tune slot 0xf, parts 46-49 (4 stages; likely turbo) */
    short idx_t0e; /* Part index into game-data table 0x0e; tune slot 8, parts 10-12 (3 stages) */
    short idx_t0f; /* Part index into game-data table 0x0f; tune slot 7, parts 3-5 (3 stages) */
    short idx_t10; /* Part index into game-data table 0x10; tune slot 9, part 33 */
    short idx_t14; /* Part index into game-data table 0x14; tune slot 0x11, parts 26-28 (3 stages) */
    short idx_t13; /* Part index into game-data table 0x13; tune slot 0x10, parts 16-17 (2 stages) */
    short idx_t1b; /* Part index into game-data table 0x1b; tune slot 0x14, part 0 */
    short idx_t1c; /* Part index into game-data table 0x1c; tune slot 0x15, part 38 */
    short idx_t1d; /* Part index into game-data table 0x1d: wheels (tune slot 5, set by the wheel shop) */
    ushort pad_3a;
    short t11_values[8]; /* Gear ratios x1000 from the transmission entry: [1..n] forward gears, [0] = (3*[1]+[2])/4. Recomputed by Gearbox_ComputeRatios for a full-custom transmission */
    short t11_4c; /* From table 0x11 entry +0x1a */
    byte t11_4e; /* From table 0x11 entry +0x21 */
    byte t0d_4f; /* From table 0x0d entry +0x07 */
    byte t01_50[2]; /* Both from table 0x01 entry +0x0c */
    byte t05_52; /* From table 0x05 entry +0x12 */
    byte t05_53; /* From table 0x05 entry +0x15 */
    byte turbo; /* From table 0x0c entry +0x0a. 0 without a turbo kit; non-zero (4, 5, 8 seen) with one. N/A kits leave it alone. */
    byte t0c_55[5]; /* From table 0x0c entry +0x0b..+0x0f */
    byte t12_5a[4]; /* From table 0x12 entry +0x0b/+0x0e/+0x15/+0x18 */
    byte setting_5e[2]; /* Both default to 0x80 (centred setting) */
    byte t12_60[14]; /* From table 0x12 entry +0x1f..+0x4a, every 3-4 bytes */
    byte t15_6e[6]; /* From table 0x15 entry +0x0d/+0x17/+0x10/+0x1a/+0x13/+0x1d (pairs) */
    byte t1b_74; /* From table 0x1b entry +0x0b */
    byte t1c_75; /* From table 0x1c entry +0x0c */
    ushort engineSound; /* Engine sound id (sorted list g_EngineSoundIds); Race_LoadEngineBanks loads file g_VolBootIds[0xbf] + EngineSound_FindIndex(id) * 9 */
    byte engineSoundVariant; /* Second engine bank: file + engineSoundVariant + 1 of the same 9-file group */
    byte manufacturerId; /* 0xff by default; from car entry +0x3a when DAT_80092878 == 2. Same IDs as g_MenuPageAttr dealers */
    byte flags_7a; /* bit 6 set when DAT_80092878 == 2 */
    byte t11_4e_copy; /* Copy of t11_4e, made when slot 6 changes */
    short t11_4c_copy; /* Copy of t11_4c, made when slot 6 changes */
    short work_17b6; /* Copy of CarWork +0x17b6 when slot 6 changes */
    short work_17b8; /* Copy of CarWork +0x17b8 when slot 6 changes */
    ushort unk_82;
};

struct CarWork { /* Tuning workspace for one car (0x17e8 bytes). setup is the car's CarSetup; each tuning category keeps the game-data entry of every stage the car can use (stage 0 = stock, copied from the default setup) and its entry index (-1 = not available). partSlots[slot] = installed stage per tune slot (CarWork_SetPartSlot). Built by CarWork_Clear (memset -1), CarWork_LoadParts, CarWork_SetStagesFromSetup. Instances: g_CarWork (OVL4 tune shops), g_RaceCarWork / g_RaceCarWorkAlt (resident, read by OVL0). */
    struct CarSetup setup;
    uchar unk0084[1540]; /* not written by CarWork_LoadParts (stays 0xff) */
    int wheelIdx[192]; /* table 0x1d entry index per wheel choice (CarWork_SetPartSlot slot 5); filled outside CarWork_LoadParts, count unknown */
    uchar suspEntries[4][76]; /* table 0x12 (suspension): stock, sports, semi-racing, full */
    int suspIdx[4];
    uchar brakeEntries[2][12]; /* table 0x00 (brakes) */
    int brakeIdx[2];
    uchar brakeBalEntries[2][16]; /* table 0x01 (brake balance) */
    int brakeBalIdx[2];
    uchar frontTyreEntries[10][16]; /* table 0x16 front tyres: [0] stock, [1..7] parts 39-45, [8],[9] also stock */
    int frontTyreIdx[10];
    uchar frontTyre18[10][4]; /* table 0x18 entry of each front tyre stage */
    uchar frontTyre19[10][64]; /* table 0x19 entry of each front tyre stage */
    uchar frontTyre1a[10][8]; /* table 0x1a entry of each front tyre stage */
    uchar rearTyreEntries[10][12]; /* table 0x17 rear tyres (same stages as front) */
    int rearTyreIdx[10];
    uchar rearTyre18[10][4]; /* table 0x18 */
    uchar rearTyre19[10][64]; /* table 0x19 */
    uchar transEntries[4][36]; /* table 0x11 (transmission): stock, sports, semi, full. Entry +0x20 != 0 = ratios computed (CarWork_TransIsCustom) */
    int transIdx[4];
    uchar clutchEntries[4][16]; /* table 0x0f */
    int clutchIdx[4];
    uchar flywheelEntries[4][12]; /* table 0x0e */
    int flywheelIdx[4];
    uchar driveshaftEntries[2][12]; /* table 0x10 */
    int driveshaftIdx[2];
    uchar chipEntries[2][12]; /* table 0x0a */
    int chipIdx[2];
    uchar portPolishEntries[2][12]; /* table 0x07 */
    int portPolishIdx[2];
    uchar balanceEntries[2][12]; /* table 0x08 */
    int balanceIdx[2];
    uchar naTuneEntries[4][12]; /* table 0x0b */
    int naTuneIdx[4];
    uchar displEntries[2][12]; /* table 0x09 */
    int displIdx[2];
    uchar turboEntries[5][20]; /* table 0x0c */
    int turboIdx[5];
    uchar intercoolerEntries[3][12]; /* table 0x13 */
    int intercoolerIdx[3];
    uchar mufflerEntries[4][12]; /* table 0x14 */
    int mufflerIdx[4];
    uchar weightEntries[4][12]; /* table 0x04 */
    int weightIdx[4];
    uchar racingModEntries[5][28]; /* table 0x05; [1..4] = racing modification stages (Part_LookupRacingModStage) */
    int racingModIdx[5];
    uchar ascEntries[2][16]; /* table 0x1b */
    int ascIdx[2];
    uchar tcsEntries[2][16]; /* table 0x1c */
    int tcsIdx[2];
    uchar lsdEntries[6][32]; /* table 0x15 (LSD / yaw control) */
    int lsdIdx[6];
    uchar t02Entry[24]; /* table 0x02, from setup idx_08 (no tune part) */
    int t02Idx;
    uchar t03Entry[20]; /* table 0x03, from setup idx_0a */
    int t03Idx;
    uchar t06Entry[76]; /* table 0x06, from setup idx_0c */
    int t06Idx;
    uchar t0dEntry[16]; /* table 0x0d, from setup idx_t0d */
    int t0dIdx;
    short partSlots[27]; /* installed stage per tune slot = g_CarWork_PartSlots. 0 susp, 1 brakes, 2 brake balance, 3 front tyres, 4 rear tyres, 5 wheels (not set here), 6 transmission, 7 clutch, 8 flywheel, 9 driveshaft, 10 chip, 0xb port polish, 0xc balancing, 0xd NA tune, 0xe displacement, 0xf turbo, 0x10 intercooler, 0x11 muffler, 0x12 weight, 0x13 racing mod, 0x14 ASC, 0x15 TCS, 0x16 LSD, 0x17 table 2, 0x18/0x19 always 0, 0x1a table 0xd */
    short pad17e6;
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

enum { VOLID_CARCOLOR=0, VOLID_CARINFOA=1, VOLID_CARINFOE=2, VOLID_CARINFOJ=3, VOLID_CCJAPANESE=4, VOLID_CCLATAIN=5, VOLID_CRSINFO=6, VOLID_CRSTIMS_TSD=7, VOLID_TEXT_DATA_RACE_TXD=8, VOLID_USEDCAR=9, VOLID_USEDCAR_JPN=10, VOLID_USEDCAR_USA=11, VOLID_ARCADE_ARC_CARLOGO=12, VOLID_ARCADE_ARC_FONT_TIM=13, VOLID_ARCADE_ARC_FONTINFO=14, VOLID_ARCADE_ARC_GOODIES_FR_TIM=15, VOLID_ARCADE_ARC_GOODIES_GR_TIM=16, VOLID_ARCADE_ARC_GOODIES_IT_TIM=17, VOLID_ARCADE_ARC_GOODIES_JP_TIM=18, VOLID_ARCADE_ARC_GOODIES_SP_TIM=19, VOLID_ARCADE_ARC_GOODIES_UK_TIM=20, VOLID_ARCADE_ARC_GOODIES_US_TIM=21, VOLID_ARCADE_ARC_KEY_CONFIG_TIM=22, VOLID_ARCADE_ARC_MAKER_TIM=23, VOLID_ARCADE_ARC_OTHER_TIM=24, VOLID_ARCADE_ARC_PANELS_FR_TIM=25, VOLID_ARCADE_ARC_PANELS_GR_TIM=26, VOLID_ARCADE_ARC_PANELS_IT_TIM=27, VOLID_ARCADE_ARC_PANELS_JP_TIM=28, VOLID_ARCADE_ARC_PANELS_SP_TIM=29, VOLID_ARCADE_ARC_PANELS_UK_TIM=30, VOLID_ARCADE_ARC_PANELS_US_TIM=31, VOLID_ARCADE_CHAMPTIM_TIM=32, VOLID_ARCADE_COURSE_MAP=33, VOLID_ARCADE_COURSE_MAPINFO=34, VOLID_ARCADE_DEMOFILE_EU_GMR=35, VOLID_ARCADE_DEMOFILE_JP_GMR=36, VOLID_ARCADE_DEMOFILE_US_GMR=37, VOLID_ARCADE_GAME_STATUS_FILES=38, VOLID_ARCADE_GT_CUR_FR_TIM=39, VOLID_ARCADE_GT_CUR_GR_TIM=40, VOLID_ARCADE_GT_CUR_IT_TIM=41, VOLID_ARCADE_GT_CUR_SP_TIM=42, VOLID_ARCADE_GT_CUR_UK_TIM=43, VOLID_ARCADE_GT_CURSOR_TIM=44, VOLID_ARCADE_GT_ITEMS_TIM=45, VOLID_ARCADE_GTMODE_FONT_TIM=46, VOLID_ARCADE_L_SEL_TIM=47, VOLID_ARCADE_LANGSEL_TIM=48, VOLID_ARCADE_LICENSE_INFO_FR=49, VOLID_ARCADE_LICENSE_INFO_GR=50, VOLID_ARCADE_LICENSE_INFO_IT=51, VOLID_ARCADE_LICENSE_INFO_JP=52, VOLID_ARCADE_LICENSE_INFO_SP=53, VOLID_ARCADE_LICENSE_INFO_UK=54, VOLID_ARCADE_LICENSE_INFO_US=55, VOLID_ARCADE_LICENSE_TIM_TIM=56, VOLID_ARCADE_SETTING_TIM=57, VOLID_ARCADE_TITLE_ARCADE_JP_TIM=58, VOLID_ARCADE_TITLE_ARCADE_US_TIM=59, VOLID_ARCADE_TITLE_GTMODE_JP_TIM=60, VOLID_ARCADE_TITLE_GTMODE_US_TIM=61, VOLID_ARCADE_TITLE_ITEM_TIM=62, VOLID_ARCADE_TOPMENU_PANELS_FR_TIM=63, VOLID_ARCADE_TOPMENU_PANELS_GR_TIM=64, VOLID_ARCADE_TOPMENU_PANELS_IT_TIM=65, VOLID_ARCADE_TOPMENU_PANELS_JP_TIM=66, VOLID_ARCADE_TOPMENU_PANELS_SP_TIM=67, VOLID_ARCADE_TOPMENU_PANELS_UK_TIM=68, VOLID_ARCADE_TOPMENU_PANELS_US_TIM=69, VOLID_BGSOBJ=70, VOLID_CARPARAM_ARCADE_DATA_DAT=71, VOLID_CARPARAM_ENG_ARCADE_DATA_DAT=72, VOLID_CARPARAM_ENG_GTMODE_DATA_DAT=73, VOLID_CARPARAM_ENG_GTMODE_RACE_DAT=74, VOLID_CARPARAM_ENG_LICENSE_DATA_DAT=75, VOLID_CARPARAM_ENG_UNISTRDB_DAT=76, VOLID_CARPARAM_FRA_ARCADE_DATA_DAT=77, VOLID_CARPARAM_FRA_GTMODE_DATA_DAT=78, VOLID_CARPARAM_FRA_GTMODE_RACE_DAT=79, VOLID_CARPARAM_FRA_LICENSE_DATA_DAT=80, VOLID_CARPARAM_FRA_UNISTRDB_DAT=81, VOLID_CARPARAM_GER_ARCADE_DATA_DAT=82, VOLID_CARPARAM_GER_GTMODE_DATA_DAT=83, VOLID_CARPARAM_GER_GTMODE_RACE_DAT=84, VOLID_CARPARAM_GER_LICENSE_DATA_DAT=85, VOLID_CARPARAM_GER_UNISTRDB_DAT=86, VOLID_CARPARAM_GTMODE_DATA_DAT=87, VOLID_CARPARAM_GTMODE_DATA_DEV_DAT=88, VOLID_CARPARAM_GTMODE_RACE_DAT=89, VOLID_CARPARAM_ITA_ARCADE_DATA_DAT=90, VOLID_CARPARAM_ITA_GTMODE_DATA_DAT=91, VOLID_CARPARAM_ITA_GTMODE_RACE_DAT=92, VOLID_CARPARAM_ITA_LICENSE_DATA_DAT=93, VOLID_CARPARAM_ITA_UNISTRDB_DAT=94, VOLID_CARPARAM_JPN_UNISTRDB_DAT=95, VOLID_CARPARAM_LICENSE_DATA_DAT=96, VOLID_CARPARAM_SPA_ARCADE_DATA_DAT=97, VOLID_CARPARAM_SPA_GTMODE_DATA_DAT=98, VOLID_CARPARAM_SPA_GTMODE_RACE_DAT=99, VOLID_CARPARAM_SPA_LICENSE_DATA_DAT=100, VOLID_CARPARAM_SPA_UNISTRDB_DAT=101, VOLID_CARPARAM_USA_ARCADE_DATA_DAT=102, VOLID_CARPARAM_USA_GTMODE_DATA_DAT=103, VOLID_CARPARAM_USA_GTMODE_RACE_DAT=104, VOLID_CARPARAM_USA_LICENSE_DATA_DAT=105, VOLID_CARPARAM_USA_UNISTRDB_DAT=106, VOLID_CARWHEEL=107, VOLID_CRSOBJ=108, VOLID_CRSTIM_ARC=109, VOLID_DIRT_AA_NN_DIRT_2=110, VOLID_DIRT_AA_NN_DIRT_3=111, VOLID_DIRT_AA_NO_NAME_DIRT=112, VOLID_DIRT_AA_NO_NAME_DIRT_REV=113, VOLID_DIRT_AA_PIKES=114, VOLID_DIRT_AA_PIKES_REV=115, VOLID_DIRT_AA_TAHITI_D_NEW=116, VOLID_DIRT_AA_TAHITI_D_NEW_REV=117, VOLID_DIRT_AA_TAHITI_TEST7=118, VOLID_DIRT_AB_NN_DIRT_2=119, VOLID_DIRT_AB_NN_DIRT_3=120, VOLID_DIRT_AB_NO_NAME_DIRT=121, VOLID_DIRT_AB_NO_NAME_DIRT_REV=122, VOLID_DIRT_AB_PIKES=123, VOLID_DIRT_AB_PIKES_REV=124, VOLID_DIRT_AB_TAHITI_D_NEW=125, VOLID_DIRT_AB_TAHITI_D_NEW_REV=126, VOLID_DIRT_AB_TAHITI_TEST7=127, VOLID_DIRT_AC_NN_DIRT_2=128, VOLID_DIRT_AC_NN_DIRT_3=129, VOLID_DIRT_AC_NO_NAME_DIRT=130, VOLID_DIRT_AC_NO_NAME_DIRT_REV=131, VOLID_DIRT_AC_PIKES=132, VOLID_DIRT_AC_PIKES_REV=133, VOLID_DIRT_AC_TAHITI_D_NEW=134, VOLID_DIRT_AC_TAHITI_D_NEW_REV=135, VOLID_DIRT_AC_TAHITI_TEST7=136, VOLID_DIRT_EA_NN_DIRT_2=137, VOLID_DIRT_EA_NN_DIRT_3=138, VOLID_DIRT_EA_NO_NAME_DIRT=139, VOLID_DIRT_EA_NO_NAME_DIRT_REV=140, VOLID_DIRT_EA_PIKES=141, VOLID_DIRT_EA_PIKES_REV=142, VOLID_DIRT_EA_TAHITI_D_NEW=143, VOLID_DIRT_EA_TAHITI_D_NEW_REV=144, VOLID_DIRT_EA_TAHITI_TEST7=145, VOLID_DIRT_EB_NN_DIRT_2=146, VOLID_DIRT_EB_NN_DIRT_3=147, VOLID_DIRT_EB_NO_NAME_DIRT=148, VOLID_DIRT_EB_NO_NAME_DIRT_REV=149, VOLID_DIRT_EB_PIKES=150, VOLID_DIRT_EB_PIKES_REV=151, VOLID_DIRT_EB_TAHITI_D_NEW=152, VOLID_DIRT_EB_TAHITI_D_NEW_REV=153, VOLID_DIRT_EB_TAHITI_TEST7=154, VOLID_DIRT_EC_NN_DIRT_2=155, VOLID_DIRT_EC_NN_DIRT_3=156, VOLID_DIRT_EC_NO_NAME_DIRT=157, VOLID_DIRT_EC_NO_NAME_DIRT_REV=158, VOLID_DIRT_EC_PIKES=159, VOLID_DIRT_EC_PIKES_REV=160, VOLID_DIRT_EC_TAHITI_D_NEW=161, VOLID_DIRT_EC_TAHITI_D_NEW_REV=162, VOLID_DIRT_EC_TAHITI_TEST7=163, VOLID_DIRT_IA_NN_DIRT_2=164, VOLID_DIRT_IA_NN_DIRT_3=165, VOLID_DIRT_IA_NO_NAME_DIRT=166, VOLID_DIRT_IA_NO_NAME_DIRT_REV=167, VOLID_DIRT_IA_PIKES=168, VOLID_DIRT_IA_PIKES_REV=169, VOLID_DIRT_IA_TAHITI_D_NEW=170, VOLID_DIRT_IA_TAHITI_D_NEW_REV=171, VOLID_DIRT_IA_TAHITI_TEST7=172, VOLID_DIRT_IB_NN_DIRT_2=173, VOLID_DIRT_IB_NN_DIRT_3=174, VOLID_DIRT_IB_NO_NAME_DIRT=175, VOLID_DIRT_IB_NO_NAME_DIRT_REV=176, VOLID_DIRT_IB_PIKES=177, VOLID_DIRT_IB_PIKES_REV=178, VOLID_DIRT_IB_TAHITI_D_NEW=179, VOLID_DIRT_IB_TAHITI_D_NEW_REV=180, VOLID_DIRT_IB_TAHITI_TEST7=181, VOLID_DIRT_IC_NN_DIRT_2=182, VOLID_DIRT_IC_NN_DIRT_3=183, VOLID_DIRT_IC_NO_NAME_DIRT=184, VOLID_DIRT_IC_NO_NAME_DIRT_REV=185, VOLID_DIRT_IC_PIKES=186, VOLID_DIRT_IC_PIKES_REV=187, VOLID_DIRT_IC_TAHITI_D_NEW=188, VOLID_DIRT_IC_TAHITI_D_NEW_REV=189, VOLID_DIRT_IC_TAHITI_TEST7=190, VOLID_ENGINE=191, VOLID_ENGINE_ENE_N_ES=192, VOLID_ENGINE_ENE_T_ES=193, VOLID_FONT_RACEFONT_DAT=194, VOLID_GTMENU_COMMONPIC_DAT=195, VOLID_GTMENU_COMMONPIC_IDX=196, VOLID_GTMENU_ENG_GTMENUDAT_DAT=197, VOLID_GTMENU_ENG_GTMENUDAT_IDX=198, VOLID_GTMENU_ENG_ICONIMG_DAT=199, VOLID_GTMENU_ENG_SOLODATA_DAT=200, VOLID_GTMENU_FRA_GTMENUDAT_DAT=201, VOLID_GTMENU_FRA_GTMENUDAT_IDX=202, VOLID_GTMENU_FRA_ICONIMG_DAT=203, VOLID_GTMENU_FRA_SOLODATA_DAT=204, VOLID_GTMENU_GER_GTMENUDAT_DAT=205, VOLID_GTMENU_GER_GTMENUDAT_IDX=206, VOLID_GTMENU_GER_ICONIMG_DAT=207, VOLID_GTMENU_GER_SOLODATA_DAT=208, VOLID_GTMENU_ITA_GTMENUDAT_DAT=209, VOLID_GTMENU_ITA_GTMENUDAT_IDX=210, VOLID_GTMENU_ITA_ICONIMG_DAT=211, VOLID_GTMENU_ITA_SOLODATA_DAT=212, VOLID_GTMENU_JPN_GTMENUDAT_DAT=213, VOLID_GTMENU_JPN_GTMENUDAT_IDX=214, VOLID_GTMENU_JPN_ICONIMG_DAT=215, VOLID_GTMENU_JPN_SOLODATA_DAT=216, VOLID_GTMENU_SPA_GTMENUDAT_DAT=217, VOLID_GTMENU_SPA_GTMENUDAT_IDX=218, VOLID_GTMENU_SPA_ICONIMG_DAT=219, VOLID_GTMENU_SPA_SOLODATA_DAT=220, VOLID_GTMENU_USA_GTMENUDAT_DAT=221, VOLID_GTMENU_USA_GTMENUDAT_IDX=222, VOLID_GTMENU_USA_ICONIMG_DAT=223, VOLID_GTMENU_USA_SOLODATA_DAT=224, VOLID_LICENSE_A_LIA00_LGF=225, VOLID_LICENSE_E_LIA00_LGF=226, VOLID_LICENSE_I_LIA00_LGF=227, VOLID_REPLAY_SCEA_000=228, VOLID_REPLAY_SCEA_999=229, VOLID_REPLAY_SCEE_000=230, VOLID_REPLAY_SCEE_999=231, VOLID_REPLAY_SCEI_000=232, VOLID_REPLAY_SCEI_999=233, VOLID_SOUND_ARCADE_SEQ=234, VOLID_SOUND_ARCSEQ_INS=235, VOLID_SOUND_GTMSEQ_INS=236, VOLID_SOUND_SE01_INS=237, VOLID_SOUND_SPU_02_SEQ=238, VOLID_SOUND_SPU_03_SEQ=239, VOLID_SOUND_SPU_04_SEQ=240, VOLID_SOUND_SPU_05_SEQ=241, VOLID_SOUND_SPU_06_SEQ=242, VOLID_SOUND_SPU_07_SEQ=243, VOLID_SOUND_SPU_08_SEQ=244, VOLID_SOUND_SPU_09_SEQ=245, VOLID_SOUND_SPU_10_SEQ=246, VOLID_SOUND_SYS_INS=247, VOLID_COUNT=248 };
typedef unsigned short VolBootId16;

struct LangMenuFiles { /* /gtmenu/<lang>/ files. Rows: 0 JPN, 1 USA, 2 ENG, 3 FRA, 4 GER, 5 ITA, 6 SPA */
    VolBootId16 gtmenudatDat;
    VolBootId16 iconimg;
    VolBootId16 solodata;
    VolBootId16 gtmenudatIdx;
};

enum {  };
typedef unsigned int VolBootId;

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
    uchar colorDirty;
    uchar pad165;
    uchar carParams[448];
    ushort perf[55];
    struct CarSpecSheet spec;
    uchar pad3f0[96];
};

typedef struct EventPrizeInfo EventPrizeInfo, *PEventPrizeInfo;

typedef struct Car Car, *PCar;

struct Car {
    uint carId; /* Car model ID (looked up for prices, parts and specs) */
    uint color; /* Colour index chosen at purchase */
    struct CarSetup setup; /* Copy of CarWork's first 0x84 bytes (Car_StoreSetup); default from Car_GetDefaultSetup */
    uint baseCarId; /* Car ID at purchase; racing modification (part 34) replaces it with the racing model's ID */
    int value; /* Price paid plus the cost of every installed part (Car_InstallPart). Used-car buy deducts this. Selling does NOT use it: Garage_SellCar pays Car_GetNewPrice(carId) / 4 */
    ushort stat94; /* bits 0-12 weight (CarParams.weight), bits 13-15 drivetrain (0 FR, 1 FF, 2 AWD, 3 MR, 4 RR). Set by Car_StoreSetup / CarWork_ComputeStats */
    ushort stat96; /* Max torque (EnginePerf.maxTorque) */
    ushort flags98; /* bits 0-13 max power (EnginePerf.maxPower; compared to EventRules.maxPower); bit 14 Car_GetTransFlag14; bit 15 racing modification installed (CarSetup_Finalize / Car_StoreSetup) */
    byte partsOwned[7]; /* Bitset of installed tuning parts, bit partId (Car_InstallPart / Car_HasPart) */
    byte pad_a1;
    ushort dirt; /* tentative: non-zero allows the car wash (50 Cr), which clears it; shown on the preview car */
};

struct EventPrizeInfo { /* Prizes of the event about to be run (Event_LoadPrizeInfo); read by OVL0 Race_AwardChampionshipPrize. */
    int championshipPrize; /* event entry +0x98 x100 */
    int prizes[6]; /* entry +0x78..+0x82 x100, by finishing position */
    uchar prizeCarCount;
    uchar pad1d[3];
    struct Car prizeCars[4]; /* from entry +0x84, random colour */
};

typedef struct MenuBgmBank MenuBgmBank, *PMenuBgmBank;

typedef struct SeqPlayer SeqPlayer, *PSeqPlayer;

typedef struct SeqFade SeqFade, *PSeqFade;

typedef struct SeqTrack SeqTrack, *PSeqTrack;

struct SeqFade {
    short count;
    short step;
    short extra;
    short rate;
    short reload;
    short acc;
};

struct SeqTrack {
    uchar *ptr;
    int delta;
    ushort trackVol;
    ushort effVol;
    short pitchBend;
    short pad0e;
    uchar pan; /* event 0x05 */
    uchar program; /* event 0x03 */
    uchar loopDepth; /* loop stack depth (events 0x01/0x20-0x3f push, 0x02 pops) */
    uchar pad13;
    uchar unk14[4];
    uchar *loopStart[3]; /* loop start pointers */
    uchar unk24;
    uchar loopCount[3]; /* remaining loop counts (0xff = forever) */
};

struct SeqPlayer {
    struct SeqPlayer *next;
    ushort baseVol;
    ushort vol;
    int tempo;
    int tickAcc;
    int tickPeriod;
    int ticksThisFrame;
    void *instBank;
    struct SeqFade fade; /* volume fade (SeqFade_Step each tick); count 0 = idle */
    struct SeqTrack tracks[16];
};

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
    ushort type;
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

typedef struct GameState GameState, *PGameState;

typedef struct PortConfig PortConfig, *PPortConfig;

typedef struct PlayerStats PlayerStats, *PPlayerStats;

typedef struct CourseRecord CourseRecord, *PCourseRecord;

typedef struct LicenseTest LicenseTest, *PLicenseTest;

typedef struct MachineTestRecords MachineTestRecords, *PMachineTestRecords;

typedef struct Garage Garage, *PGarage;

typedef struct NeGconCalib NeGconCalib, *PNeGconCalib;

typedef struct TimeRecord TimeRecord, *PTimeRecord;

enum { MEDAL_NONE=0, MEDAL_KIDS=1, MEDAL_BRONZE=2, MEDAL_SILVER=3, MEDAL_GOLD=4 };
typedef unsigned char LicenseMedal;

typedef struct MachineTestEntry MachineTestEntry, *PMachineTestEntry;

typedef struct PadAxisRange PadAxisRange, *PPadAxisRange;

struct TimeRecord { /* Initialised by TimeRecord_Init. Also used by the 0x24-byte records at g_GameState+0x218. */
    int time; /* -1 = empty */
    int splits[3]; /* -1 on init (tentative: sector/lap splits) */
    short unk10; /* 0 on init */
    short unk12; /* -1 on init */
};

struct LicenseTest { /* Saved progress for one license test (0xa4 bytes). g_LicenseTests[LicenseId][test]. */
    byte unk0; /* 0 on new game */
    LicenseMedal medal;
    byte nearMisses; /* finishes inside the kids-prize time while medal is NONE; saturates at 0xff */
    byte pad3;
    struct TimeRecord bestTimes[5]; /* top 5; drawn on the license result screen */
    char bestNames[5][12]; /* string per bestTimes entry, drawn right-aligned (tentative: driver/car name) */
};

struct Garage {
    short carCount; /* Number of cars owned, max 100 */
    ushort pad_02;
    struct Car cars[100];
    int credits; /* Player money, 0..99,999,999 (10000 on a new game) */
    short currentCar; /* Slot of the car in use, -1 = none */
    byte unk_401a;
    char driverName[12]; /* Entered on the license/record name screen (max 11 chars); copied into LicenseTest.bestNames. Empty on a new game. */
    undefined field7_0x4027;
};

struct CourseRecord { /* Per-course best (0x24). Merged by FUN_OVL1__8001e014 keeping the lower time. */
    struct TimeRecord best;
    undefined field1_0x14;
    undefined field2_0x15;
    undefined field3_0x16;
    undefined field4_0x17;
    undefined field5_0x18;
    undefined field6_0x19;
    undefined field7_0x1a;
    undefined field8_0x1b;
    undefined field9_0x1c;
    undefined field10_0x1d;
    undefined field11_0x1e;
    undefined field12_0x1f;
    undefined field13_0x20;
    undefined field14_0x21;
    undefined field15_0x22;
    undefined field16_0x23;
};

struct MachineTestEntry { /* One machine test result (tentative). */
    int carId;
    int result; /* time, or speed for the top-speed list */
    undefined field2_0x8;
    undefined field3_0x9;
    undefined field4_0xa;
    undefined field5_0xb;
    undefined field6_0xc;
    undefined field7_0xd;
    undefined field8_0xe;
    undefined field9_0xf;
    undefined field10_0x10;
    undefined field11_0x11;
    undefined field12_0x12;
    undefined field13_0x13;
};

struct MachineTestRecords { /* Ranking for one machine test (race modes 7, 8, 9; mode 9 shows speeds). Tentatively the 0-400m, 0-1000m and top-speed tests. */
    char count;
    undefined field1_0x1;
    undefined field2_0x2;
    undefined field3_0x3;
    struct MachineTestEntry entries[8]; /* carId -1 = empty */
};

struct PadAxisRange {
    ushort min;
    ushort max;
};

struct NeGconCalib {
    ushort twist[4]; /* min, centre low, centre high, max */
    struct PadAxisRange axes[3]; /* I, II, L */
};

struct PortConfig { /* Saved controller settings for one pad port (GameState.ports). */
    uchar buttonMaps[4][11]; /* per controller kind: 0 digital, 1 analog/DualShock, 2 NeGcon (KeyConfig_Load); 8 bytes + 3 bytes each */
    uchar vibrationOff; /* 0 = vibration on (OVL0 800133f0); Options item 0xe stores value != 1 */
    uchar analogMaps[3][4]; /* per controller kind, read with buttonMaps */
    undefined field3_0x39;
    undefined field4_0x3a;
    undefined field5_0x3b;
    undefined field6_0x3c;
    undefined field7_0x3d;
    struct NeGconCalib negconCalib; /* copied to g_NeGconCalib[port] by Options_Apply */
};

struct PlayerStats { /* Player career statistics (0x160, cleared by PlayerStats_Init). Read by the OVL4 status page (FUN_OVL4__80019c60..80019e2c) and OVL2 80023610/8001d434. */
    undefined field0_0x0;
    undefined field1_0x1;
    undefined field2_0x2;
    undefined field3_0x3;
    undefined field4_0x4;
    undefined field5_0x5;
    undefined field6_0x6;
    undefined field7_0x7;
    undefined field8_0x8;
    undefined field9_0x9;
    undefined field10_0xa;
    undefined field11_0xb;
    undefined field12_0xc;
    undefined field13_0xd;
    undefined field14_0xe;
    undefined field15_0xf;
    undefined field16_0x10;
    undefined field17_0x11;
    undefined field18_0x12;
    undefined field19_0x13;
    undefined field20_0x14;
    undefined field21_0x15;
    undefined field22_0x16;
    undefined field23_0x17;
    undefined field24_0x18;
    undefined field25_0x19;
    undefined field26_0x1a;
    undefined field27_0x1b;
    undefined field28_0x1c;
    undefined field29_0x1d;
    undefined field30_0x1e;
    undefined field31_0x1f;
    undefined field32_0x20;
    undefined field33_0x21;
    undefined field34_0x22;
    undefined field35_0x23;
    undefined field36_0x24;
    undefined field37_0x25;
    undefined field38_0x26;
    undefined field39_0x27;
    undefined field40_0x28;
    undefined field41_0x29;
    undefined field42_0x2a;
    undefined field43_0x2b;
    undefined field44_0x2c;
    undefined field45_0x2d;
    undefined field46_0x2e;
    undefined field47_0x2f;
    undefined field48_0x30;
    undefined field49_0x31;
    undefined field50_0x32;
    undefined field51_0x33;
    undefined field52_0x34;
    undefined field53_0x35;
    undefined field54_0x36;
    undefined field55_0x37;
    undefined field56_0x38;
    undefined field57_0x39;
    undefined field58_0x3a;
    undefined field59_0x3b;
    undefined field60_0x3c;
    undefined field61_0x3d;
    undefined field62_0x3e;
    undefined field63_0x3f;
    int day; /* 1 on a new game (g_day) */
    undefined field65_0x44;
    undefined field66_0x45;
    undefined field67_0x46;
    undefined field68_0x47;
    uint racesEntered; /* incremented by the GT race loop when a GT race starts */
    uint racesWon; /* wins of GT prize races (PlayerStats_AddRaceResult) */
    uint finishPlaceSum; /* sum of finish places of GT prize races (PlayerStats_AddRaceResult); the status page shows finishPlaceSum*100/finishCount as the average finish */
    uint finishCount; /* GT prize races finished (PlayerStats_AddRaceResult) */
    int prizeMoneyHi; /* total prize money / 100,000,000 (tentative) */
    uint prizeMoneyLo; /* total prize money % 100,000,000 (tentative) */
    byte eventResults[124]; /* 4-bit best finish per event index (low nibble = even index), 0 = never finished; 1 = won */
    undefined field76_0xdc;
    undefined field77_0xdd;
    undefined field78_0xde;
    undefined field79_0xdf;
    undefined field80_0xe0;
    undefined field81_0xe1;
    undefined field82_0xe2;
    undefined field83_0xe3;
    undefined field84_0xe4;
    undefined field85_0xe5;
    undefined field86_0xe6;
    undefined field87_0xe7;
    undefined field88_0xe8;
    undefined field89_0xe9;
    undefined field90_0xea;
    undefined field91_0xeb;
    undefined field92_0xec;
    undefined field93_0xed;
    undefined field94_0xee;
    undefined field95_0xef;
    undefined field96_0xf0;
    undefined field97_0xf1;
    undefined field98_0xf2;
    undefined field99_0xf3;
    undefined field100_0xf4;
    undefined field101_0xf5;
    undefined field102_0xf6;
    undefined field103_0xf7;
    undefined field104_0xf8;
    undefined field105_0xf9;
    undefined field106_0xfa;
    undefined field107_0xfb;
    undefined field108_0xfc;
    undefined field109_0xfd;
    undefined field110_0xfe;
    undefined field111_0xff;
    undefined field112_0x100;
    undefined field113_0x101;
    undefined field114_0x102;
    undefined field115_0x103;
    undefined field116_0x104;
    undefined field117_0x105;
    undefined field118_0x106;
    undefined field119_0x107;
    undefined field120_0x108;
    undefined field121_0x109;
    undefined field122_0x10a;
    undefined field123_0x10b;
    undefined field124_0x10c;
    undefined field125_0x10d;
    undefined field126_0x10e;
    undefined field127_0x10f;
    undefined field128_0x110;
    undefined field129_0x111;
    undefined field130_0x112;
    undefined field131_0x113;
    undefined field132_0x114;
    undefined field133_0x115;
    undefined field134_0x116;
    undefined field135_0x117;
    undefined field136_0x118;
    undefined field137_0x119;
    undefined field138_0x11a;
    undefined field139_0x11b;
    undefined field140_0x11c;
    undefined field141_0x11d;
    undefined field142_0x11e;
    undefined field143_0x11f;
    undefined field144_0x120;
    undefined field145_0x121;
    undefined field146_0x122;
    undefined field147_0x123;
    undefined field148_0x124;
    undefined field149_0x125;
    undefined field150_0x126;
    undefined field151_0x127;
    undefined field152_0x128;
    undefined field153_0x129;
    undefined field154_0x12a;
    undefined field155_0x12b;
    undefined field156_0x12c;
    undefined field157_0x12d;
    undefined field158_0x12e;
    undefined field159_0x12f;
    undefined field160_0x130;
    undefined field161_0x131;
    undefined field162_0x132;
    undefined field163_0x133;
    undefined field164_0x134;
    undefined field165_0x135;
    undefined field166_0x136;
    undefined field167_0x137;
    undefined field168_0x138;
    undefined field169_0x139;
    undefined field170_0x13a;
    undefined field171_0x13b;
    undefined field172_0x13c;
    undefined field173_0x13d;
    undefined field174_0x13e;
    undefined field175_0x13f;
    undefined field176_0x140;
    undefined field177_0x141;
    undefined field178_0x142;
    undefined field179_0x143;
    undefined field180_0x144;
    undefined field181_0x145;
    undefined field182_0x146;
    undefined field183_0x147;
    undefined field184_0x148;
    undefined field185_0x149;
    undefined field186_0x14a;
    undefined field187_0x14b;
    undefined field188_0x14c;
    undefined field189_0x14d;
    undefined field190_0x14e;
    undefined field191_0x14f;
    undefined field192_0x150;
    undefined field193_0x151;
    undefined field194_0x152;
    undefined field195_0x153;
    undefined field196_0x154;
    undefined field197_0x155;
    undefined field198_0x156;
    undefined field199_0x157;
    undefined field200_0x158;
    undefined field201_0x159;
    undefined field202_0x15a;
    undefined field203_0x15b;
    undefined field204_0x15c;
    undefined field205_0x15d;
    undefined field206_0x15e;
    undefined field207_0x15f;
};

struct GameState { /* Persistent game state. The memory-card save is the first 0x7c9c bytes (through garages[0]); see FUN_8006a214 / FUN_8006a278. Initialised by GameState_InitNewGame. */
    uchar language; /* index into per-language tables (CarSpecStrings_Load); 1 on a new game */
    uchar unk01; /* bytes 1..8 are copied to g_RaceSetup+0 */
    uchar options[7]; /* Options menu items 0..6 (Option_Get/80017e68); new game 0,2,1,0,2,0,1 */
    undefined field3_0x9;
    struct PortConfig ports[2];
    uchar options2[7]; /* Options menu items 7..0xd; new game 0,0,1,1,0xf0,0xc0 for items 8..0xd */
    uchar unkB5; /* 1 on a new game */
    undefined field7_0xb6;
    undefined field8_0xb7;
    struct PlayerStats stats;
    struct CourseRecord courseRecords[128]; /* first DAT_801e18e6 entries used */
    struct LicenseTest licenses[6][10]; /* licenses[LicenseId][test] */
    struct MachineTestRecords machineTests[3];
    struct Garage garages[2]; /* only garages[0] is in the save */
};

enum { PART_ASC=0, PART_SPORTS_BRAKES=1, PART_BRAKE_BALANCE=2, PART_CLUTCH_SINGLE=3, PART_CLUTCH_TWIN=4, PART_CLUTCH_TRIPLE=5, PART_PERF_CHIP=6, PART_DISPLACEMENT=7, PART_DISPLACEMENT_2=8, PART_ENGINE_BALANCING=9, PART_FLYWHEEL_SPORTS=10, PART_FLYWHEEL_SEMI=11, PART_FLYWHEEL_RACING=12, PART_TRANS_SPORTS=13, PART_TRANS_SEMI=14, PART_TRANS_FULL=15, PART_INTERCOOLER_SPORTS=16, PART_INTERCOOLER_RACING=17, PART_WEIGHT_RED_1=18, PART_WEIGHT_RED_2=19, PART_WEIGHT_RED_3=20, PART_LSD_1WAY=21, PART_LSD_2WAY=22, PART_LSD_15WAY=23, PART_LSD_FULL=24, PART_YAW_CONTROL=25, PART_MUFFLER_SPORTS=26, PART_MUFFLER_SEMI=27, PART_MUFFLER_RACING=28, PART_NA_TUNE_1=29, PART_NA_TUNE_2=30, PART_NA_TUNE_3=31, PART_PORT_POLISH=32, PART_CARBON_DRIVESHAFT=33, PART_RACING_MOD=34, PART_SUSP_SPORTS=35, PART_SUSP_SEMI=36, PART_SUSP_FULL=37, PART_TCS=38, PART_TIRES_SPORTS=39, PART_TIRES_HARD=40, PART_TIRES_MEDIUM=41, PART_TIRES_SOFT=42, PART_TIRES_SUPER_SOFT=43, PART_TIRES_SIMULATION=44, PART_TIRES_DIRT=45, PART_TURBO_1=46, PART_TURBO_2=47, PART_TURBO_3=48, PART_TURBO_4=49 };
typedef unsigned int PartId;

enum { LICENSE_S=0, LICENSE_IA=1, LICENSE_IB=2, LICENSE_IC=3, LICENSE_A=4, LICENSE_B=5, LICENSE_NONE=6 };
typedef unsigned int LicenseId;

typedef struct GTMenu GTMenu, *PGTMenu;

typedef struct psxViewLoopDoubleBuffer psxViewLoopDoubleBuffer, *PpsxViewLoopDoubleBuffer;

typedef struct DrawLayer DrawLayer, *PDrawLayer;

typedef struct PadDevice PadDevice, *PPadDevice;

typedef struct PadState PadState, *PPadState;

typedef struct MapCursor MapCursor, *PMapCursor;

typedef struct GTMenuView GTMenuView, *PGTMenuView;

typedef struct psxViewLoop psxViewLoop, *PpsxViewLoop;

typedef struct LayerHeader LayerHeader, *PLayerHeader;

typedef struct PadHandler PadHandler, *PPadHandler;

typedef struct PadButtonState PadButtonState, *PPadButtonState;

enum { PAD_UP=1, PAD_DOWN=2, PAD_LEFT=4, PAD_RIGHT=8, PAD_L1=16, PAD_L2=32, PAD_L3=64, PAD_TRIANGLE=256, PAD_CROSS=512, PAD_SQUARE=1024, PAD_CIRCLE=2048, PAD_R1=4096, PAD_R2=8192, PAD_R3=16384, PAD_START=65536, PAD_SELECT=131072 };
typedef unsigned int PadButton;

typedef struct MenuElement MenuElement, *PMenuElement;

typedef struct psxViewLoop_vtbl psxViewLoop_vtbl, *PpsxViewLoop_vtbl;

typedef struct VSyncCallback VSyncCallback, *PVSyncCallback;

typedef struct ViewDrawContext ViewDrawContext, *PViewDrawContext;

typedef struct PacketBuffer PacketBuffer, *PPacketBuffer;

struct PadState { /* Button events since the previous PadButtons_Read. */
    PadButton held; /* held at any time since the last read */
    PadButton pressed;
    PadButton released;
    PadButton repeat;
};

// The resident screen loops, as C structs. A file that includes viewloop.h first
// gets its classes instead.
#ifndef VIEWLOOP_H
struct ViewDrawContext {
    struct DrawLayer *layers; /* sorted DrawLayer list */
    void *otBase; /* both OT halves */
    void *otHead; /* head of the frame being built (last entry of the reverse-cleared half). BeginDraw sends the previous one with Gpu_DrawOTag before swapping halves; 0 skips the send */
    void *otStart; /* first entry of the current half */
    void *otEnd; /* last entry of the current half */
    ushort otSize; /* entries in both halves */
    ushort otHalf; /* 0 or otSize/2 */
    int field_18;
    void *tailPackets; /* terminator packets EndDraw appends (draw mode, cache flush), allocated from g_PacketPtr */
};

struct VSyncCallback {
    struct VSyncCallback *prev;
    struct VSyncCallback *next;
    void (*func)(void);
    uint priority; /* list is sorted by ascending priority */
};

struct PacketBuffer { /* Double-buffered primitive packet pool; g_PacketPtr allocates from the current half */
    void *base;
    uint size; /* words, both halves */
    uint half; /* 0 or size/2 */
};

struct psxViewLoop {
    struct psxViewLoop_vtbl *vptr;
    struct psxViewLoop *prevLoop; /* loop below this one on the loop stack (g_CurrentViewLoop) */
    struct VSyncCallback vsync; /* registered in g_VSyncCallbacks; calls UpdateDisplay */
    int vsyncInterval; /* vsyncs per frame (1) */
    uchar videoModeDelay; /* frames until ApplyVideoMode (3) */
    uchar bufferIndex;
    uchar bufferCount; /* 1, or 2 for double buffering */
    uchar videoModePending;
    uchar swapPending; /* vsync count at which to SwapDisplay; 0 = none */
    uchar noDrawSync;
    uchar interlaced; /* videoMode bit 2 (480 lines): halves the vertical range, Flip waits for the field */
    uchar videoMode; /* GP1(08) mode: bits 0-1 hres (256/320/512/640), 2 480 lines, 3 PAL, 4 24-bit, 5 interlace, 6 hres 368 */
    uchar videoArg;
    undefined field13_0x25;
    short drawX;
    short drawY;
    short drawW;
    short drawH;
    short dispX; /* display rect x; Gpu_SetHRange start */
    short dispY;
    short dispW; /* display width; SetVideoMode copies the draw rect here */
    short dispH;
    undefined field22_0x36;
    undefined field23_0x37;
    struct ViewDrawContext draw;
    struct PacketBuffer packets;
    int frameStartTime;
    int frameEndTime;
};

struct psxViewLoopDoubleBuffer {
    struct psxViewLoop base;
    short bufferY[2]; /* VRAM Y of each buffer */
};
#endif

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
    uchar loaded; /* set by MenuPage_Enter */
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

struct PadButtonState {
    PadButton releaseMask; /* ANDed into released; 0 after PadDevice_FlushInput */
    PadButton held; /* current frame */
    PadButton heldAccum; /* events below accumulate until PadButtons_Read */
    PadButton pressed;
    PadButton released;
    PadButton repeat; /* fires at holdFrames == repeatStart, then every repeatEnd - repeatStart frames */
    uchar repeatStart; /* 30 */
    uchar repeatEnd; /* 37 */
    uchar holdFrames[32]; /* per button bit */
};

struct PadDevice {
    short port;
    uchar type; /* controller type from the raw buffer, 0 = none */
    uchar connected;
    struct PadHandler *handlers; /* PadHandler table, e.g. 0x800a6f5c */
    struct PadDevice *prevDevice; /* device this one replaced on its port; restored on close */
    struct PadButtonState buttons;
    ushort analogMask; /* bit n set = analog[n] valid; 0xf for NeGcon and analog pads */
    ushort analog[4]; /* 0..0xff, centre 0x80 (NeGcon twist / sticks) */
    undefined field8_0x52;
    undefined field9_0x53;
    undefined field10_0x54;
    undefined field11_0x55;
    undefined field12_0x56;
    undefined field13_0x57;
    undefined field14_0x58;
    undefined field15_0x59;
    undefined field16_0x5a;
    undefined field17_0x5b;
    short vibration1; /* 12.4 fixed point, sent as actuator byte 1 */
    short vibration0; /* 12.4 fixed point, sent as actuator byte 0 */
    short vibrationFrames; /* vibration fields are cleared when this counts down past 0 */
    undefined field21_0x62;
    undefined field22_0x63;
};

struct DrawLayer {
    ushort priority; /* ViewDrawContext.head list is sorted by it */
    ushort otSize; /* OT entries for this layer (its slice of the view's OT) */
    struct DrawLayer *next;
    void *ot; /* this layer's OT slice; primitives are added here */
    struct LayerHeader *header; /* draw area and offset packet sent before the layer's primitives */
};

// struct GTMenu is the class in gtmenu.h.
struct psxViewLoop_vtbl {
    int offset; /* always 0 */
    void * (*tf)(void *); /* __tf type_info function */
    void (*dtor)(void *, int);
    void (*Run)(void *);
    void (*Enter)(void *);
    int (*Step)(void *);
    void (*Flip)(void *);
    void (*Leave)(void *);
    void (*BeginFrame)(void *);
    int (*Update)(void *);
    void (*BeginDraw)(void *);
    void (*Draw)(void *);
    void (*EndDraw)(void *);
    void (*DrawFrame)(void *);
    void (*SwapDisplay)(void *);
    void (*ApplyVideoMode)(void *);
    void (*UpdateDisplay)(void *);
};

struct PadHandler { /* Entry of a PadDevice.handlers table, searched by type. */
    int type; /* controller type (typeLen>>4, 0 = none) */
    void (*update)(struct PadDevice *, ushort, ushort *, int);
    void (*post)(struct PadDevice *); /* also called when the device is closed */
};

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

struct LayerHeader { /* 16-byte packet at the head of each layer, allocated by ViewDrawContext_Begin; DrawLayer_SetArea fills it */
    uint tag; /* packet link and length */
    uint drawAreaTopLeft; /* GP0(E3) */
    uint drawAreaBottomRight; /* GP0(E4) */
    uint drawOffset; /* GP0(E5) */
};

typedef struct MenuList MenuList, *PMenuList;

enum { MLE_INIT=0, MLE_REVEAL=1, MLE_CLOSE=2, MLE_UPDATE=3, MLE_DRAW=4, MLE_LEAVE=5, MLE_ENTER=6, MLE_OPEN=7, MLE_IS_ENABLED=8 };
typedef unsigned int MenuListEventId;

typedef struct MenuListEvent MenuListEvent, *PMenuListEvent;

typedef int (MenuListFn)(MenuListEventId, struct MenuListEvent *, int);

struct MenuList {
    short count;
    ushort flags;
    short visibleRows;
    short selected;
    short cursorWidth;
    short itemHeight;
    short itemGap;
    char arrowWidth;
    char arrowHeight;
    short x;
    short y;
    short fadeMax;
    ushort skipEvents;
    short revealIndex;
    short revealTimer;
    short revealDelay;
    uchar hadInput;
    uchar pad1f;
    short scrollAnim;
    short blinkTimer;
    short fade;
    short state;
    MenuListFn *callback;
    void *userData;
};

struct MenuListEvent {
    struct MenuList *list;
    void *ot;
    struct PadState *pad;
    short x;
    short y;
    short brightness;
    short row;
    uchar enabled;
    uchar pad15[3];
};

typedef struct RECT RECT, *PRECT;

#ifndef VIEWLOOP_H
struct RECT {
    short x;
    short y;
    short w;
    short h;
};
#endif

typedef struct MenuCarList MenuCarList, *PMenuCarList;

struct MenuCarList { /* garage or used car list widget (g_GarageList, g_UsedCarList) */
    short count;
    short selected;
    struct MenuElement *element;
};

#endif
