// The types of the game, shared by all the decompiled sources: those of the
// resident code exported from the Ghidra project (DataTypeWriter) and adapted
// for cc1plus (enums as sized integers, C++ keywords renamed), the screen loops
// (viewloop.h), the GPU primitives (prim.h), and at the end the types declared
// with the decompiled files, and the subsystem headers. Overlay types are in
// ovl4.h and movie.h. See docs/decomp.md.
#ifndef RESIDENT_H
#define RESIDENT_H

typedef int code(...);
typedef unsigned char   undefined;
typedef unsigned char    byte;

typedef unsigned char    uchar;
typedef unsigned int    uint;
typedef unsigned long    ulong;
typedef unsigned char    undefined1;
typedef unsigned short    undefined2;
typedef unsigned int    undefined4;
typedef unsigned short    ushort;
typedef unsigned short    word;
typedef void *pointer;
typedef unsigned int    dword;
typedef long long    longlong;
typedef signed char    schar;

#include "viewloop.h"
#include "prim.h"

typedef struct Camera Camera, *PCamera;
typedef struct MATRIX MATRIX, *PMATRIX;

struct MATRIX {
    short m[3][3];
    long t[3];
};

struct Camera {
    short vpX;
    short vpY;
    short width;
    short height;
    struct MATRIX view;
    int negPos[3];
    short clipX[9];
    short pad46;
    short clipY[9];
    short pad5a;
    int ofx;
    int ofy;
    short projH;
    short screenW;
    short screenH;
    short nearZ;
    void *ot;
    void *ot2;
    void *otEnd;
    int depthScale;
    short invRot[9];
    short pad8e;
    short centerX;
    short centerY;
    short extentX;
    short extentY;
    short frustumH;
    short pad9a;
};

typedef struct CardDialog CardDialog, *PCardDialog;
typedef struct TextItem TextItem, *PTextItem;
typedef struct Font Font, *PFont;
typedef struct FontGlyphTable FontGlyphTable, *PFontGlyphTable;
typedef struct FontKerning FontKerning, *PFontKerning;
typedef struct FontGlyph FontGlyph, *PFontGlyph;
typedef struct FontSprite FontSprite, *PFontSprite;

struct TextItem {
    uchar charsPerStep;
    uchar unk01;
    short revealSteps;
    short zoomWidth;
    short closeSteps;
    short fadeSteps;
    signed char spacing;
    signed char height;
    ushort flags;
    ushort pad0e;
    struct Font *font;
    uchar rgbFrom[4];
    uchar rgbTo[4];
    char *text;
    uchar brightness;
    uchar pad21;
    short anim;
    short animEnd;
    short length;
};

struct CardDialog {
    short x;
    short y;
    struct TextItem message;
    struct TextItem option0;
    struct TextItem option1;
    signed char sfx;            /* menu sound on a move, < 0 for none */
    signed char choice;         /* 0 or 1 */
    ushort boxWidth;
    ushort boxHeight;
    short anim;
    short flash;
    ushort flags;
    uchar pad88[4];
    uint color;
    uint color2;
    short result;
    short pad96;
};

struct FontSprite { /* Glyph image rectangle in the font texture (one GPU sprite). */

    uint u:8; /* texture x within page */

    uint v:8;
    uint halfWidth:5; /* width/2 */

    uint height:6;
    uint clutOffset:2; /* added to TextCtx.clut */

    uint tpageOffset:3; /* added to TextCtx.tpage */

};

struct FontGlyph {
    uint advance:6; /* pen advance before kerning */

    uint sprite:8; /* index in FontGlyphTable.sprites, 0 = nothing drawn */

    uint accent:3; /* 1-7 = FontGlyphTable.accents[accent-1], 0 = none */

    uint monoOffsetX:6; /* x offset when drawn in a fixed-width digit cell */

    uint kernLeft:8; /* kerning row when this glyph is on the left; if kernLeftFixed, low 6 bits are a fixed kerning */

    uint kernLeftFixed:1;
    uint offsetX:6;
    uint ascent:6; /* sprite top = y - ascent */

    uint accentX:6;
    uint accentY:5; /* accent top = sprite top - accentY */

    uint kernRight:8; /* kerning column when this glyph is on the right; if kernRightFixed, low 6 bits are a fixed kerning */

    uint kernRightFixed:1;
};

struct FontGlyphTable { /* Glyph data of one font. Kerning is in a separate FontKerning block. */

    struct FontGlyph glyphs[256]; /* indexed by character byte */

    struct FontSprite accents[6]; /* FontGlyph.accent 1-6 -> accents[accent-1]; accent 7 reads sprites[0] */

    struct FontSprite sprites[256]; /* indexed by FontGlyph.sprite (real count varies); [0] doubles as accent 7 */

};

struct FontKerning { /* Kerning pairs, subtracted from the left glyph's advance. */

    uchar rowBytes;
    uchar rowCount;
    ushort bitsPerEntry; /* 4-7 */

    uchar rows[1]; /* rowCount rows of rowBytes bytes, entries packed LSB first, indexed [kernLeft][kernRight] */

};

struct Font {
    struct FontGlyphTable *glyphs;
    struct FontKerning *kerning;
    uchar digitWidth; /* cell width for fixed-width digits */

    char spacing; /* added to every advance */
    uchar unk0a; /* added to the y of FUN_80082af8 */

};

typedef struct CardMenu CardMenu, *PCardMenu;

enum { CARDMODE_SAVE_REPLAY=0, CARDMODE_PLAY_REPLAY=1, CARDMODE_EDIT_REPLAY=2, CARDMODE_LOAD_GHOST=3, CARDMODE_SAVE_GAME=4, CARDMODE_LOAD_GAME=5 };
typedef unsigned short CardMode;

enum { PAD_UP=1, PAD_DOWN=2, PAD_LEFT=4, PAD_RIGHT=8, PAD_L1=16, PAD_L2=32, PAD_L3=64, PAD_TRIANGLE=256, PAD_CROSS=512, PAD_SQUARE=1024, PAD_CIRCLE=2048, PAD_R1=4096, PAD_R2=8192, PAD_R3=16384, PAD_START=65536, PAD_SELECT=131072 };
typedef unsigned int PadButton;
typedef struct WipeBar WipeBar, *PWipeBar;
typedef struct TextEntry TextEntry, *PTextEntry;
typedef struct CardProgressBar CardProgressBar, *PCardProgressBar;
typedef struct SectorMap SectorMap;
typedef struct SectorChain SectorChain;
typedef struct ReplayDir ReplayDir, *PReplayDir;
typedef struct ReplayDirEntry ReplayDirEntry, *PReplayDirEntry;
typedef struct GameSaveFile GameSaveFile, *PGameSaveFile;
typedef struct MenuList MenuList, *PMenuList;
typedef struct McFileHeader McFileHeader, *PMcFileHeader;

enum { MLE_INIT=0, MLE_REVEAL=1, MLE_CLOSE=2, MLE_UPDATE=3, MLE_DRAW=4, MLE_LEAVE=5, MLE_ENTER=6, MLE_OPEN=7, MLE_IS_ENABLED=8 };
typedef unsigned int MenuListEventId;
typedef struct MenuListEvent MenuListEvent, *PMenuListEvent;
typedef int (MenuListFn)(MenuListEventId, struct MenuListEvent *, int);
typedef struct PadState PadState, *PPadState;

struct ReplayDirEntry {
    char title[32];
    char info[31];
    signed char infoTruncated;
    uchar unk40;
    uchar raceMode;
    uchar kind;
    uchar pad43;
    uint courseId;
    uchar unk48[8];
    short firstSector;
    short sectorCount;
    int size;
    uint crc;
};

struct McFileHeader {
    char magic[2];
    uchar iconType;
    uchar blockCount;
    uchar title[64];
    uchar reserved44[28];
    ushort clut[16];
    uchar icon[3][128];
};

// The replay file's sectors: each one's next sector in its chain (-1 ends a chain,
// SECTOR_FREE marks a free sector).
struct SectorMap {
    short count;
    short next[960];
};

// A replay's sectors, in order.
struct SectorChain {
    short count;
    short sectors[960];
};

struct ReplayDir {
    struct McFileHeader header;
    signed char count;
    uchar pad201;
    short totalSectors;
    struct SectorMap sectorMap;
    uchar pad986[2];
    struct ReplayDirEntry entries[32];
    uint crc;
    uchar pad150c[116];
};

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

struct WipeBar {
    short width;
    short height;
    short duration;
    ushort flags;
    uint rgbTop;
    uint rgbBottom;
    uint rgbIn;
    uint rgbOut;
    short t;
    short pad1a;
};

struct TextEntry {
    struct Font *font;
    char cellWidth;
    char cellHeight;
    char cellGap;
    char rowGap;
    short textPri;
    short glyphY;
    short spacing;
    short x;
    short y;
    short pad12;
    short anim;
    short blinkTimer;
    short cursorTimer;
    uchar row;
    uchar col;
    short cursorPos;
    short maxLen;
    short maxWidth;
    short pad22;
    char *text;
    struct WipeBar bar;
    struct MenuList list;
    uchar pad74[4];
};

struct CardProgressBar {
    short x;
    short y;
    short segWidth;
    short segHeight;
    short segStep;
    short pad0a;
    uint color;
    uint total;
    signed char segFade[32];
    short visible;
    short pad36;
};

struct CardMenu { /* Shared memory-card UI context (g_CardMenu). Handlers in g_CardMenu_States drive it; they return the next state, -1 to stay, 0x27 CARD_EXIT (user left) or 0x28 CARD_REPLAY_LOADED. */

    CardMode mode;
    short state;
    void *handler;
    void *kanjiFont;
    short *slotStatus;
    short slot;
    short delay;
    PadButton pressed;
    PadButton held;
    PadButton repeat;
    uchar deleteMode;
    uchar pad21[3];
    char *errorMsg;
    char *line1;
    char *line2;
    short textPri;
    short timer;
    struct WipeBar slotBar;
    struct CardDialog dlgError;
    struct CardDialog dlgSelectSlot;
    struct CardDialog dlgNoCard;
    struct CardDialog dlgFormat;
    struct CardDialog dlgSaveFailed;
    struct CardDialog dlgSaveError;
    struct CardDialog dlgComplete;
    struct CardDialog dlgOverwrite;
    struct CardDialog dlgConfirmSave;
    struct CardDialog dlgConfirmLoad;
    struct CardDialog dlgDiscard;
    struct TextEntry titleEntry;
    short titleResult;
    char titleBuf[256];
    short blockPickResult;
    short listResult;
    struct CardProgressBar progress;
    uchar rows[33][4];
    uchar pad914[28];
    short selEntry;
    short editEntry;
    short usedSectors;
    short entrySectors;
    struct ReplayDir dir;
    uchar pad1eb8[12];
    struct ReplayDirEntry newEntry;
    short newSectors[962];
    uchar replayData[32768];
    int replaySize;
    struct GameSaveFile *saveFile;
};

struct GameSaveFile {
    struct McFileHeader header;
    uchar state[31900];
    uint crc;
};

struct PadState { /* Button events since the previous PadButtons_Read. */

    PadButton held; /* held at any time since the last read */

    PadButton pressed;
    PadButton released;
    PadButton repeat;
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

typedef struct CarInfoA CarInfoA, *PCarInfoA;
typedef struct CarInfoAEntry CarInfoAEntry, *PCarInfoAEntry;

struct CarInfoAEntry {
    uint carId;
    uint dataOffset:18;
    uint colorCountMinus1:5;
    uint hiddenRegions:4; /* bit set = car hidden: bit0 Japanese, bit1 US, bit2 French/German/Italian/Spanish, bit3 UK (game language byte, see CarInfoA_IsAvailable) */

    uint unused:5;
};

struct CarInfoA { /* Header of .carinfoa (VOLID_CARINFOA): magic 'CAR', count, then count CarInfoAEntry sorted by carId. Each entry's dataOffset (from file start) points to: ushort rgb[n] (PS1 15-bit swatch colours); char codes[n] (colour letters from '-0123456789abcdefghijklmnopqrstuvwxyz'); uchar nameLen; char name[] (NUL-terminated, may start with 0x7f). n = colorCountMinus1+1. .carcolor is a parallel table indexed by the same entry index. */

    char magic[4];
    ushort count;
    ushort pad;
    struct CarInfoAEntry entries[1110]; /* count entries (1110 on the US Simulation disc) */

};

typedef struct CarModelHeader CarModelHeader, *PCarModelHeader;
typedef struct CarModelWheelPos CarModelWheelPos, *PCarModelWheelPos;

struct CarModelWheelPos {
    short unk0;
    short y;
    short z;
    short x;
};

struct CarModelHeader {
    int unk00;
    void *shadowMesh;
    uint colors[4];
    short frontWheel[2];
    short rearWheel[2];
    struct CarModelWheelPos wheelPos[4];
    uint wheelTexture;
    short frontHubZ;
    short rearHubZ;
    uint frontHubQuad[4];
    int pad58;
    uint rearHubQuad[4];
    int pad6c;
    uchar wheelMeshes[2040];
    int lodCount;
};

typedef struct CarParams CarParams, *PCarParams;

struct CarParams { /* Computed race parameters of a car (0x1c0 bytes, CarParams_Build). Only the fields used by the GT-mode code are named; names are tentative. */

    uchar unk00[19];
    uchar unk13; /* x10 -> EngineCurve.unk8e */

    uchar unk14[20];
    short gear28; /* used by Gearbox_ComputeRatios (/1000) */

    uchar gearCount; /* number of forward gears (at least 5 for the ratio table) */

    uchar unk2b;
    uchar unk2c; /* x39.2 -> EngineCurve.unk90 (Fixed_Div by the top curve rpm) */

    uchar turboBoost; /* max boost x0.1 (0 = no turbo); EngineCurve.maxBoost */

    uchar unk2e[3];
    uchar minRedlineDiv100; /* redline floor = value*100 + 500 */

    uchar redlineDiv100; /* redline/100; 0 = last curve point */

    uchar unk33;
    uchar curveRpmDiv100[34]; /* rpm of each torque-curve point / 100 */

    uchar curvePoints; /* number of torque-curve points */

    uchar unk57[3];
    short weight; /* kg; Car.stat94 bits 0-12 */

    uchar unk5c[10];
    uchar tyreA[2]; /* front/rear tyre values for CarParams_GetTyreRadius */

    uchar tyreB[2];
    uchar tyreC[2];
    uchar unk6c[30];
    uchar drivetrain; /* 0 FR, 1 FF, 2 AWD, 3 MR, 4 RR; Car.stat94 bits 13-15 */

    uchar unk8b[142];
    uchar awdFrontPct; /* AWD front torque share % */

    uchar unk11a[36];
    short curveTorque[29]; /* torque of each curve point */

    uchar turbo1RpmDiv100;
    uchar turbo1Boost;
    uchar turbo1K;
    uchar turbo2RpmDiv100;
    uchar turbo2Boost;
    uchar turbo2K;
    uchar unk17e[30];
    uchar turbo1Type; /* boost curve shape 0..2 (EngineCurve.boostType) */

    uchar turbo2Type;
    uchar unk19e[12];
    uchar unk1aa;
    uchar gearTopSpeedDiv10; /* top speed (km/h /10) used to compute full-custom gear ratios */

    uchar lowRpmFactorPct; /* torque factor % at the first curve point (0 = 100) */

    uchar unk1ad;
    ushort powerFactor; /* overall torque factor per mille (<0x100: percent) */

    ushort highRpmFactor; /* torque factor per mille at the last curve point (0 = 1000) */

    uchar unk1b2[14];
};

typedef struct CarSetup CarSetup, *PCarSetup;

struct CarSetup {
    uint wheel; /* Wheel code from table 0x1d entry (see Wheel_ParseCode) | variant << 8. Rebuilt by CarSetup_SetWheels */

    ushort idx_t00; /* Part index into game-data table 0x00; tune slot 1, part 1 */

    ushort idx_t01; /* Part index into game-data table 0x01; tune slot 2, part 2 */

    ushort idx_08; /* From car entry +0x08; no tune part writes it */

    ushort idx_0a; /* From car entry +0x0a; no tune part writes it */

    ushort idx_0c; /* From car entry +0x10; no tune part writes it */

    ushort idx_t0d; /* Part index into game-data table 0x0d; no tune part writes it */

    ushort idx_t11; /* Part index into game-data table 0x11 (transmission); tune slot 6, parts 13-15 */

    ushort idx_t12; /* Part index into game-data table 0x12 (suspension); tune slot 0, parts 35-37 */

    ushort idx_t15; /* Part index into game-data table 0x15; tune slot 0x16, parts 21-25 (5 stages) */

    ushort idx_t16_front; /* Part index into game-data table 0x16; tune slot 3, parts 39-45 (7 stages): front tyres */

    ushort idx_t16_rear; /* Part index into game-data table 0x16; tune slot 4, parts 39-45: rear tyres */

    ushort idx_t04; /* Part index into game-data table 0x04; tune slot 0x12, parts 18-20: weight reduction stages 1-3 (each requires the previous) */

    ushort idx_t05; /* Part index into game-data table 0x05; tune slot 0x13, part 34: racing modification (requires part 20; also changes baseCarId) */

    ushort idx_t07; /* Part index into game-data table 0x07; tune slot 0xb, part 32 */

    ushort idx_t08; /* Part index into game-data table 0x08; tune slot 0xc, part 9 */

    ushort idx_t09; /* Part index into game-data table 0x09; tune slot 0xe, parts 7-8 */

    ushort idx_t0a; /* Part index into game-data table 0x0a; tune slot 10, part 6 */

    ushort idx_t0b; /* Part index into game-data table 0x0b; tune slot 0xd, parts 29-31 (3 stages) */

    ushort idx_t0c; /* Part index into game-data table 0x0c; tune slot 0xf, parts 46-49 (4 stages; likely turbo) */

    ushort idx_t0e; /* Part index into game-data table 0x0e; tune slot 8, parts 10-12 (3 stages) */

    ushort idx_t0f; /* Part index into game-data table 0x0f; tune slot 7, parts 3-5 (3 stages) */

    ushort idx_t10; /* Part index into game-data table 0x10; tune slot 9, part 33 */

    ushort idx_t14; /* Part index into game-data table 0x14; tune slot 0x11, parts 26-28 (3 stages) */

    ushort idx_t13; /* Part index into game-data table 0x13; tune slot 0x10, parts 16-17 (2 stages) */

    ushort idx_t1b; /* Part index into game-data table 0x1b; tune slot 0x14, part 0 */

    ushort idx_t1c; /* Part index into game-data table 0x1c; tune slot 0x15, part 38 */

    ushort idx_t1d; /* Part index into game-data table 0x1d: wheels (tune slot 5, set by the wheel shop) */

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

typedef struct CarSpec CarSpec, *PCarSpec;

struct CarSpec { /* Setup resolved to game-data entry pointers (CarSpec_Resolve, CarSpec_FromCarWork), input of CarParams_Build. */

    uint unk00;
    void *brakes;
    void *brakeBalance;
    void *t02;
    void *t03;
    void *t06;
    void *t0d;
    void *transmission;
    void *suspension;
    void *lsd;
    void *frontTyre18;
    void *frontTyre19;
    void *frontTyre1a;
    void *rearTyre18;
    void *rearTyre19;
    void *weightReduction;
    void *racingMod;
    void *portPolish;
    void *balancing;
    void *displacement;
    void *chip;
    void *naTune;
    void *turbo;
    void *flywheel;
    void *clutch;
    void *driveshaft;
    void *muffler;
    void *intercooler;
    void *asc;
    void *tcs;
    ushort unk78;
    short unk7a;
    short t11_values[8]; /* gear ratios x1000 (see CarSetup.t11_values) */

    short t11_4c;
    uchar t11_4e;
    uchar unk8f;
    uchar t01_50[2];
    uchar t05_52;
    uchar t05_53;
    uchar unk94[16];
    uchar settings[10];
    uchar unkae[6];
    uchar settingB4;
    uchar settingB5;
    short outB6;
    uchar outB8;
    uchar manufacturerId;
    uchar flags;
    uchar unkbb[9];
};

typedef struct CarWork CarWork, *PCarWork;

// Tuning workspace for one car (0x17e8 bytes). setup is the car's CarSetup;
// each tuning category keeps the game data entry of every stage the car can use
// (stage 0 = stock, copied from the default setup) and its entry index (-1 =
// not available). Built by CarWork_Clear (memset -1), CarWork_LoadParts and
// CarWork_SetStagesFromSetup. Instances: g_CarWork (OVL4 tune shops),
// g_RaceCarWork and g_RaceCarWorkAlt (resident, read by OVL0).
// Game data entries, by size, copied as words. Most hold their stage at byte 8.
union PartEntry12 { unsigned int words[3]; unsigned char bytes[12]; };
union PartEntry16 { unsigned int words[4]; unsigned char bytes[16]; };
union PartEntry20 { unsigned int words[5]; unsigned char bytes[20]; };
union PartEntry24 { unsigned int words[6]; unsigned char bytes[24]; };
union PartEntry28 { unsigned int words[7]; unsigned char bytes[28]; };
union PartEntry32 { unsigned int words[8]; unsigned char bytes[32]; };
union PartEntry36 { unsigned int words[9]; unsigned char bytes[36]; };
union PartEntry76 { unsigned int words[19]; unsigned char bytes[76]; };

// A front tyre (table 0x16), with its entries of tables 0x18 to 0x1a.
struct FrontTyreEntry {
    unsigned int unk0[2];
    unsigned char stage;          // 0x8
    unsigned char unk9;
    unsigned short idx_t18;       // 0xa
    unsigned short idx_t19;       // 0xc
    unsigned short idx_t1a;       // 0xe
};

// A rear tyre (table 0x17), with its entries of tables 0x18 and 0x19.
struct RearTyreEntry {
    unsigned int unk0;
    unsigned char stage;          // 0x4
    unsigned char unk5;
    unsigned short idx_t18;       // 0x6
    unsigned short idx_t19;       // 0x8
    unsigned short unka;
};

struct TyreEntry18 { unsigned char data[4]; };
struct TyreEntry19 { unsigned char data[64]; };
struct TyreEntry1a { unsigned char data[8]; };

// Tuning workspace for one car (0x17e8 bytes). setup is the car's CarSetup;
// each tuning category keeps the game data entry of every stage the car can use
// (stage 0 = stock, copied from the default setup) and its entry index (-1 =
// not available). Built by CarWork_Clear (memset -1), CarWork_LoadParts and
// CarWork_SetStagesFromSetup. Instances: g_CarWork (OVL4 tune shops),
// g_RaceCarWork and g_RaceCarWorkAlt (resident, read by OVL0).
struct CarWork {
    struct CarSetup setup;
    uchar unk0084[0x604];              // not written by CarWork_LoadParts (stays 0xff)
    int wheelIdx[192];                 // 0x688: table 0x1d entry per wheel choice (CarWork_SetPartSlot slot 5)
    PartEntry76 susp[4];               // 0x988: table 0x12: stock, sports, semi-racing, full
    int suspIdx[4];
    PartEntry12 brakes[2];             // 0xac8: table 0x00
    int brakesIdx[2];
    PartEntry16 brakeBal[2];           // 0xae8: table 0x01
    int brakeBalIdx[2];
    FrontTyreEntry frontTyres[10];     // 0xb10: table 0x16: [0] stock, [1..7] parts 39-45, [8], [9] also stock
    int frontTyresIdx[10];
    TyreEntry18 frontTyres18[10];      // 0xbd8
    TyreEntry19 frontTyres19[10];      // 0xc00
    TyreEntry1a frontTyres1a[10];      // 0xe80
    RearTyreEntry rearTyres[10];       // 0xed0: table 0x17 (same stages as the front)
    int rearTyresIdx[10];
    TyreEntry18 rearTyres18[10];       // 0xf70
    TyreEntry19 rearTyres19[10];       // 0xf98
    PartEntry36 trans[4];              // 0x1218: table 0x11: stock, sports, semi, full. +0x20 != 0: ratios computed (CarWork_TransIsCustom)
    int transIdx[4];
    PartEntry16 clutch[4];             // 0x12b8: table 0x0f
    int clutchIdx[4];
    PartEntry12 flywheel[4];           // 0x1308: table 0x0e
    int flywheelIdx[4];
    PartEntry12 driveshaft[2];         // 0x1348: table 0x10
    int driveshaftIdx[2];
    PartEntry12 chip[2];               // 0x1368: table 0x0a
    int chipIdx[2];
    PartEntry12 portPolish[2];         // 0x1388: table 0x07
    int portPolishIdx[2];
    PartEntry12 balance[2];            // 0x13a8: table 0x08
    int balanceIdx[2];
    PartEntry12 naTune[4];             // 0x13c8: table 0x0b
    int naTuneIdx[4];
    PartEntry12 displ[2];              // 0x1408: table 0x09
    int displIdx[2];
    PartEntry20 turbo[5];              // 0x1428: table 0x0c
    int turboIdx[5];
    PartEntry12 intercooler[3];        // 0x14a0: table 0x13
    int intercoolerIdx[3];
    PartEntry12 muffler[4];            // 0x14d0: table 0x14
    int mufflerIdx[4];
    PartEntry12 weight[4];             // 0x1510: table 0x04, stage at 0xb
    int weightIdx[4];
    PartEntry28 racingMod[5];          // 0x1550: table 0x05, stage at 0xe; [1..4] racing modification stages (Part_LookupRacingModStage)
    int racingModIdx[5];
    PartEntry16 asc[2];                // 0x15f0: table 0x1b
    int ascIdx[2];
    PartEntry16 tcs[2];                // 0x1618: table 0x1c
    int tcsIdx[2];
    PartEntry32 lsd[6];                // 0x1640: table 0x15 (LSD / yaw control)
    int lsdIdx[6];
    PartEntry24 t02;                   // 0x1718: table 0x02, from setup idx_08 (no tune part)
    int t02Idx;
    PartEntry20 t03;                   // 0x1734: table 0x03, from setup idx_0a
    int t03Idx;
    PartEntry76 t06;                   // 0x174c: table 0x06, from setup idx_0c
    int t06Idx;
    PartEntry16 t0d;                   // 0x179c: table 0x0d, from setup idx_t0d
    int t0dIdx;
    // 0x17b0: installed stage per tune slot (CarWork_SetPartSlot, g_CarWork_PartSlots):
    // 0 susp, 1 brakes, 2 brake balance, 3 front tyres, 4 rear tyres, 5 wheels (not set
    // here), 6 transmission, 7 clutch, 8 flywheel, 9 driveshaft, 10 chip, 0xb port polish,
    // 0xc balancing, 0xd NA tune, 0xe displacement, 0xf turbo, 0x10 intercooler, 0x11
    // muffler, 0x12 weight, 0x13 racing mod, 0x14 ASC, 0x15 TCS, 0x16 LSD, 0x17 table 2,
    // 0x18 and 0x19 always 0, 0x1a table 0xd
    short partSlots[27];
    short pad17e6;
};

typedef struct CdAudio CdAudio, *PCdAudio;
struct CdAudio {
    struct VSyncCallback vsync;
    ushort pollTimer; /* counts down from 8 (7 after a poll) */

    short fadeStep; /* added to fade each vblank; 0 = no fade */

    void *onIdle;
    ushort masterVol; /* 1.15, 0x8000 = 1.0 */

    ushort volL;
    ushort volR;
    ushort fade;
    byte reqFlag; /* request copy of +0x40 (data read: 1 = report sector errors?) */

    byte reqMode; /* request CdlSetmode bits (0x80 = double speed) */

    byte reqFile; /* request XA filter file */

    byte reqChannel; /* request XA filter channel */

    uint reqStart; /* request first sector */

    uint reqEnd; /* request end sector (exclusive) */

    void *onSector;
    void *reqDst; /* request destination / ring buffer */

    uint reqSize; /* request byte count / ring sectors */

    ushort reqSkip; /* request bytes to skip in the first sector */

    ushort reqFill;
    ushort reqRead;
    ushort reqWrite;
    byte flag;
    byte mode; /* CdlSetmode bits */

    byte xaFile;
    byte xaChannel;
    uint nextSector; /* sector expected by the read handler */

    uint endSector; /* end sector (exclusive), copied from reqEnd */

    void *sectorFn;
    void *dst; /* destination / ring buffer base */

    uint remaining; /* bytes left (data read) or ring size in sectors (stream) */

    ushort skip; /* bytes to skip in the next sector */

    ushort ringFill; /* sectors buffered (stream) */

    ushort ringRead; /* read index */

    ushort ringWrite; /* write index */

    uint curSector;
    byte locFile; /* CdlGetlocL result: file and channel of the sector */
    byte locChannel;
    byte stallCount; /* CdlGetlocL returned the same sector this many times */
    undefined field38_0x67;
    void *savedSectorFn;
    byte locParam[4]; /* CdlSetloc parameter (BCD m/s/f) */

    byte filterParam[4]; /* CdlSetfilter parameter */

    byte modeParam; /* CdlSetmode parameter */

    undefined field43_0x75;
    undefined field44_0x76;
    undefined field45_0x77;
    byte cmd14Param[4]; /* parameter of command 0x14 */

    uint *script; /* next command word */

    uint *retryScript;
    uint *queuedScript;
    uint command;
    uint result[2]; /* last CD interrupt result */
    byte trackPos[100][2]; /* BCD minute and second of each track (1-based, then the lead-out), from CdlGetTD */
    byte firstTrack; /* from CdlGetTN */
    byte lastTrack;
    byte unk15e;
    byte noQueue; /* set by CdAudio_Reset: scripts aren't queued while a command is in flight */
    byte speedChanged; /* set when the double-speed bit changed */

    byte cddaBits; /* modeParam & 0x41 */

    byte lastCmd; /* last CdControlF command */

    byte playState; /* 1 = CdlPlay done, 2 = reading, 0 = paused */
    byte unk164;
    byte unk165;
    byte state; /* 3 = CdControlF in flight */

    byte unk167;
    byte unk168;
    byte tocRead; /* every track position has been read */
    byte halted; /* set by CdlPause and the fade-out op; sectors are ignored */
    undefined field274_0x16b;
};

typedef struct ChampRaceList ChampRaceList, *PChampRaceList;

struct ChampRaceList { /* Races of the current championship (ChampRaceList_Build), read by OVL0. */

    short unk00;
    short raceCount; /* from the last 2 digits of the event base code; 1 for a single race */

    uint courseHash[6]; /* Str_Hash of each race course name (random course when "none") */

    ushort param45[6]; /* event entry byte +0x45 of each race */

    char raceCode[6][16]; /* race event codes (base + %02d) */

    signed char points[6]; /* championship standings (ChampRaceList_SortStandings) */

    signed char racePoints[6]; /* the last race's, added by ChampRaceList_AddPoints */
};

typedef struct EngineCurve EngineCurve, *PEngineCurve;

struct EngineCurve { /* Torque curve built from CarParams (EngineCurve_Build), 0xb0 bytes. Values are 20.12 fixed point. */

    short count;
    short pad02;
    int *rpm; /* -> rpmFx */

    int *torque; /* -> torqueFx */

    int rpmFx[16];
    int torqueFx[16];
    ushort redline; /* max rpm used for the power peak */

    ushort unk8e;
    int unk90;
    ushort boostRpm[2]; /* turbo 1/2 boost rpm */

    short boost[2]; /* turbo 1/2 boost (x4096) */

    int boostA[2];
    int boostK[2]; /* boost / rpm^2 */

    short maxBoost; /* total boost cap */

    uchar boostType[2]; /* turbo 1/2 curve shape 0..2 */

};

typedef struct EnginePerf EnginePerf, *PEnginePerf;

struct EnginePerf { /* Power/torque summary (EngineCurve_GetPerf), 0x6c bytes. maxPower -> Car.flags98, maxTorque -> Car.stat96. */

    ushort maxPower; /* unsigned: OVL4 reads it with lhu */
    short maxPowerRpm;
    short maxTorque;
    short maxTorqueRpm;
    ushort redline;
    short pointCount;
    short rpm[16]; /* curve rpm points rounded to 100 */

    short power[16]; /* power at each rpm point */

    short torque[16]; /* torque at each rpm point */

};

typedef struct GameDataTable GameDataTable, *PGameDataTable;

struct GameDataTable {
    void *entries; /* Relocated at load by GameData_Relocate */

    ushort entrySize;
    ushort count;
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
typedef struct Car Car, *PCar;
typedef struct PadAxisRange PadAxisRange, *PPadAxisRange;

struct TimeRecord { /* Initialised by TimeRecord_Init. Also used by the 0x24-byte records at g_GameState+0x218. */

    uint time; /* -1 = empty: compared unsigned */

    uint splits[3]; /* -1 on init: the time at each sector line (Records_GetSectorTime). TimeRecord_Init fills a fourth, unk10 and unk12 */

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

    char name[12]; /* MachineTestRecords_SetName */
};

struct MachineTestRecords { /* Ranking for one machine test (race modes 7, 8, 9; mode 9 shows speeds). Tentatively the 0-400m, 0-1000m and top-speed tests. */

    signed char count;
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

    uchar arcadeCourses[32]; /* per arcade course (PlayerStats_SetArcadeCourseFlags): flags 1, 2 and 4, bits 3-5 count the wins with flags 2 (up to 5, then 2 and 4 are set) */
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

typedef struct GzipStream GzipStream, *PGzipStream;
typedef struct InflateState InflateState, *PInflateState;
typedef struct Huft Huft, *PHuft;

struct InflateState {
    InflateState * (*yield)(struct InflateState *); /* optional, called after each block header and table build */

    InflateState * (*refill)(struct InflateState *); /* called when in == inEnd; must set in/inEnd */

    byte *inBase;
    ushort *in; /* read 16 bits at a time */

    ushort *inEnd;
    uint bitBuf;
    int bitCount;
    InflateState * (*flush)(struct InflateState *); /* called when out == outEnd; must move outEnd */

    byte *outBase;
    byte *out;
    byte *outEnd;
    void *heapBase; /* Huft table storage */

    void *heapEnd;
    void *heapPtr; /* bump allocator, reset per block */

    struct Huft *litTable;
    struct Huft *distTable;
    uint litBits; /* lookup bits; Inflate_Codes turns it into a mask */

    uint distBits; /* same as litBits */

    uint saved; /* register spill kept across callbacks */

};

struct GzipStream {
    struct InflateState state;
    uint pad;
    byte heap[12288];
};

struct Huft {
    struct Huft *next; /* sub-table when op > 16 */

    ushort base; /* literal/length/distance base, or symbol */

    byte bits; /* bits consumed by this entry */

    byte op; /* 16 literal, 15 end of block, 99 invalid, >16 sub-table of op-16 bits, else extra bits */

};

typedef struct jmpbuf jmpbuf, *Pjmpbuf;

struct jmpbuf {
    undefined4 ra;
    undefined4 sp;
    undefined4 s8;
    undefined4 s0;
    undefined4 s1;
    undefined4 s2;
    undefined4 s3;
    undefined4 s4;
    undefined4 s5;
    undefined4 s6;
    undefined4 s7;
    undefined4 gp;
};

typedef struct LangCarParamFiles LangCarParamFiles, *PLangCarParamFiles;

enum { VOLID_CARCOLOR=0, VOLID_CARINFOA=1, VOLID_CARINFOE=2, VOLID_CARINFOJ=3, VOLID_CCJAPANESE=4, VOLID_CCLATAIN=5, VOLID_CRSINFO=6, VOLID_CRSTIMS_TSD=7, VOLID_TEXT_DATA_RACE_TXD=8, VOLID_USEDCAR=9, VOLID_USEDCAR_JPN=10, VOLID_USEDCAR_USA=11, VOLID_ARCADE_ARC_CARLOGO=12, VOLID_ARCADE_ARC_FONT_TIM=13, VOLID_ARCADE_ARC_FONTINFO=14, VOLID_ARCADE_ARC_GOODIES_FR_TIM=15, VOLID_ARCADE_ARC_GOODIES_GR_TIM=16, VOLID_ARCADE_ARC_GOODIES_IT_TIM=17, VOLID_ARCADE_ARC_GOODIES_JP_TIM=18, VOLID_ARCADE_ARC_GOODIES_SP_TIM=19, VOLID_ARCADE_ARC_GOODIES_UK_TIM=20, VOLID_ARCADE_ARC_GOODIES_US_TIM=21, VOLID_ARCADE_ARC_KEY_CONFIG_TIM=22, VOLID_ARCADE_ARC_MAKER_TIM=23, VOLID_ARCADE_ARC_OTHER_TIM=24, VOLID_ARCADE_ARC_PANELS_FR_TIM=25, VOLID_ARCADE_ARC_PANELS_GR_TIM=26, VOLID_ARCADE_ARC_PANELS_IT_TIM=27, VOLID_ARCADE_ARC_PANELS_JP_TIM=28, VOLID_ARCADE_ARC_PANELS_SP_TIM=29, VOLID_ARCADE_ARC_PANELS_UK_TIM=30, VOLID_ARCADE_ARC_PANELS_US_TIM=31, VOLID_ARCADE_CHAMPTIM_TIM=32, VOLID_ARCADE_COURSE_MAP=33, VOLID_ARCADE_COURSE_MAPINFO=34, VOLID_ARCADE_DEMOFILE_EU_GMR=35, VOLID_ARCADE_DEMOFILE_JP_GMR=36, VOLID_ARCADE_DEMOFILE_US_GMR=37, VOLID_ARCADE_GAME_STATUS_FILES=38, VOLID_ARCADE_GT_CUR_FR_TIM=39, VOLID_ARCADE_GT_CUR_GR_TIM=40, VOLID_ARCADE_GT_CUR_IT_TIM=41, VOLID_ARCADE_GT_CUR_SP_TIM=42, VOLID_ARCADE_GT_CUR_UK_TIM=43, VOLID_ARCADE_GT_CURSOR_TIM=44, VOLID_ARCADE_GT_ITEMS_TIM=45, VOLID_ARCADE_GTMODE_FONT_TIM=46, VOLID_ARCADE_L_SEL_TIM=47, VOLID_ARCADE_LANGSEL_TIM=48, VOLID_ARCADE_LICENSE_INFO_FR=49, VOLID_ARCADE_LICENSE_INFO_GR=50, VOLID_ARCADE_LICENSE_INFO_IT=51, VOLID_ARCADE_LICENSE_INFO_JP=52, VOLID_ARCADE_LICENSE_INFO_SP=53, VOLID_ARCADE_LICENSE_INFO_UK=54, VOLID_ARCADE_LICENSE_INFO_US=55, VOLID_ARCADE_LICENSE_TIM_TIM=56, VOLID_ARCADE_SETTING_TIM=57, VOLID_ARCADE_TITLE_ARCADE_JP_TIM=58, VOLID_ARCADE_TITLE_ARCADE_US_TIM=59, VOLID_ARCADE_TITLE_GTMODE_JP_TIM=60, VOLID_ARCADE_TITLE_GTMODE_US_TIM=61, VOLID_ARCADE_TITLE_ITEM_TIM=62, VOLID_ARCADE_TOPMENU_PANELS_FR_TIM=63, VOLID_ARCADE_TOPMENU_PANELS_GR_TIM=64, VOLID_ARCADE_TOPMENU_PANELS_IT_TIM=65, VOLID_ARCADE_TOPMENU_PANELS_JP_TIM=66, VOLID_ARCADE_TOPMENU_PANELS_SP_TIM=67, VOLID_ARCADE_TOPMENU_PANELS_UK_TIM=68, VOLID_ARCADE_TOPMENU_PANELS_US_TIM=69, VOLID_BGSOBJ=70, VOLID_CARPARAM_ARCADE_DATA_DAT=71, VOLID_CARPARAM_ENG_ARCADE_DATA_DAT=72, VOLID_CARPARAM_ENG_GTMODE_DATA_DAT=73, VOLID_CARPARAM_ENG_GTMODE_RACE_DAT=74, VOLID_CARPARAM_ENG_LICENSE_DATA_DAT=75, VOLID_CARPARAM_ENG_UNISTRDB_DAT=76, VOLID_CARPARAM_FRA_ARCADE_DATA_DAT=77, VOLID_CARPARAM_FRA_GTMODE_DATA_DAT=78, VOLID_CARPARAM_FRA_GTMODE_RACE_DAT=79, VOLID_CARPARAM_FRA_LICENSE_DATA_DAT=80, VOLID_CARPARAM_FRA_UNISTRDB_DAT=81, VOLID_CARPARAM_GER_ARCADE_DATA_DAT=82, VOLID_CARPARAM_GER_GTMODE_DATA_DAT=83, VOLID_CARPARAM_GER_GTMODE_RACE_DAT=84, VOLID_CARPARAM_GER_LICENSE_DATA_DAT=85, VOLID_CARPARAM_GER_UNISTRDB_DAT=86, VOLID_CARPARAM_GTMODE_DATA_DAT=87, VOLID_CARPARAM_GTMODE_DATA_DEV_DAT=88, VOLID_CARPARAM_GTMODE_RACE_DAT=89, VOLID_CARPARAM_ITA_ARCADE_DATA_DAT=90, VOLID_CARPARAM_ITA_GTMODE_DATA_DAT=91, VOLID_CARPARAM_ITA_GTMODE_RACE_DAT=92, VOLID_CARPARAM_ITA_LICENSE_DATA_DAT=93, VOLID_CARPARAM_ITA_UNISTRDB_DAT=94, VOLID_CARPARAM_JPN_UNISTRDB_DAT=95, VOLID_CARPARAM_LICENSE_DATA_DAT=96, VOLID_CARPARAM_SPA_ARCADE_DATA_DAT=97, VOLID_CARPARAM_SPA_GTMODE_DATA_DAT=98, VOLID_CARPARAM_SPA_GTMODE_RACE_DAT=99, VOLID_CARPARAM_SPA_LICENSE_DATA_DAT=100, VOLID_CARPARAM_SPA_UNISTRDB_DAT=101, VOLID_CARPARAM_USA_ARCADE_DATA_DAT=102, VOLID_CARPARAM_USA_GTMODE_DATA_DAT=103, VOLID_CARPARAM_USA_GTMODE_RACE_DAT=104, VOLID_CARPARAM_USA_LICENSE_DATA_DAT=105, VOLID_CARPARAM_USA_UNISTRDB_DAT=106, VOLID_CARWHEEL=107, VOLID_CRSOBJ=108, VOLID_CRSTIM_ARC=109, VOLID_DIRT_AA_NN_DIRT_2=110, VOLID_DIRT_AA_NN_DIRT_3=111, VOLID_DIRT_AA_NO_NAME_DIRT=112, VOLID_DIRT_AA_NO_NAME_DIRT_REV=113, VOLID_DIRT_AA_PIKES=114, VOLID_DIRT_AA_PIKES_REV=115, VOLID_DIRT_AA_TAHITI_D_NEW=116, VOLID_DIRT_AA_TAHITI_D_NEW_REV=117, VOLID_DIRT_AA_TAHITI_TEST7=118, VOLID_DIRT_AB_NN_DIRT_2=119, VOLID_DIRT_AB_NN_DIRT_3=120, VOLID_DIRT_AB_NO_NAME_DIRT=121, VOLID_DIRT_AB_NO_NAME_DIRT_REV=122, VOLID_DIRT_AB_PIKES=123, VOLID_DIRT_AB_PIKES_REV=124, VOLID_DIRT_AB_TAHITI_D_NEW=125, VOLID_DIRT_AB_TAHITI_D_NEW_REV=126, VOLID_DIRT_AB_TAHITI_TEST7=127, VOLID_DIRT_AC_NN_DIRT_2=128, VOLID_DIRT_AC_NN_DIRT_3=129, VOLID_DIRT_AC_NO_NAME_DIRT=130, VOLID_DIRT_AC_NO_NAME_DIRT_REV=131, VOLID_DIRT_AC_PIKES=132, VOLID_DIRT_AC_PIKES_REV=133, VOLID_DIRT_AC_TAHITI_D_NEW=134, VOLID_DIRT_AC_TAHITI_D_NEW_REV=135, VOLID_DIRT_AC_TAHITI_TEST7=136, VOLID_DIRT_EA_NN_DIRT_2=137, VOLID_DIRT_EA_NN_DIRT_3=138, VOLID_DIRT_EA_NO_NAME_DIRT=139, VOLID_DIRT_EA_NO_NAME_DIRT_REV=140, VOLID_DIRT_EA_PIKES=141, VOLID_DIRT_EA_PIKES_REV=142, VOLID_DIRT_EA_TAHITI_D_NEW=143, VOLID_DIRT_EA_TAHITI_D_NEW_REV=144, VOLID_DIRT_EA_TAHITI_TEST7=145, VOLID_DIRT_EB_NN_DIRT_2=146, VOLID_DIRT_EB_NN_DIRT_3=147, VOLID_DIRT_EB_NO_NAME_DIRT=148, VOLID_DIRT_EB_NO_NAME_DIRT_REV=149, VOLID_DIRT_EB_PIKES=150, VOLID_DIRT_EB_PIKES_REV=151, VOLID_DIRT_EB_TAHITI_D_NEW=152, VOLID_DIRT_EB_TAHITI_D_NEW_REV=153, VOLID_DIRT_EB_TAHITI_TEST7=154, VOLID_DIRT_EC_NN_DIRT_2=155, VOLID_DIRT_EC_NN_DIRT_3=156, VOLID_DIRT_EC_NO_NAME_DIRT=157, VOLID_DIRT_EC_NO_NAME_DIRT_REV=158, VOLID_DIRT_EC_PIKES=159, VOLID_DIRT_EC_PIKES_REV=160, VOLID_DIRT_EC_TAHITI_D_NEW=161, VOLID_DIRT_EC_TAHITI_D_NEW_REV=162, VOLID_DIRT_EC_TAHITI_TEST7=163, VOLID_DIRT_IA_NN_DIRT_2=164, VOLID_DIRT_IA_NN_DIRT_3=165, VOLID_DIRT_IA_NO_NAME_DIRT=166, VOLID_DIRT_IA_NO_NAME_DIRT_REV=167, VOLID_DIRT_IA_PIKES=168, VOLID_DIRT_IA_PIKES_REV=169, VOLID_DIRT_IA_TAHITI_D_NEW=170, VOLID_DIRT_IA_TAHITI_D_NEW_REV=171, VOLID_DIRT_IA_TAHITI_TEST7=172, VOLID_DIRT_IB_NN_DIRT_2=173, VOLID_DIRT_IB_NN_DIRT_3=174, VOLID_DIRT_IB_NO_NAME_DIRT=175, VOLID_DIRT_IB_NO_NAME_DIRT_REV=176, VOLID_DIRT_IB_PIKES=177, VOLID_DIRT_IB_PIKES_REV=178, VOLID_DIRT_IB_TAHITI_D_NEW=179, VOLID_DIRT_IB_TAHITI_D_NEW_REV=180, VOLID_DIRT_IB_TAHITI_TEST7=181, VOLID_DIRT_IC_NN_DIRT_2=182, VOLID_DIRT_IC_NN_DIRT_3=183, VOLID_DIRT_IC_NO_NAME_DIRT=184, VOLID_DIRT_IC_NO_NAME_DIRT_REV=185, VOLID_DIRT_IC_PIKES=186, VOLID_DIRT_IC_PIKES_REV=187, VOLID_DIRT_IC_TAHITI_D_NEW=188, VOLID_DIRT_IC_TAHITI_D_NEW_REV=189, VOLID_DIRT_IC_TAHITI_TEST7=190, VOLID_ENGINE=191, VOLID_ENGINE_ENE_N_ES=192, VOLID_ENGINE_ENE_T_ES=193, VOLID_FONT_RACEFONT_DAT=194, VOLID_GTMENU_COMMONPIC_DAT=195, VOLID_GTMENU_COMMONPIC_IDX=196, VOLID_GTMENU_ENG_GTMENUDAT_DAT=197, VOLID_GTMENU_ENG_GTMENUDAT_IDX=198, VOLID_GTMENU_ENG_ICONIMG_DAT=199, VOLID_GTMENU_ENG_SOLODATA_DAT=200, VOLID_GTMENU_FRA_GTMENUDAT_DAT=201, VOLID_GTMENU_FRA_GTMENUDAT_IDX=202, VOLID_GTMENU_FRA_ICONIMG_DAT=203, VOLID_GTMENU_FRA_SOLODATA_DAT=204, VOLID_GTMENU_GER_GTMENUDAT_DAT=205, VOLID_GTMENU_GER_GTMENUDAT_IDX=206, VOLID_GTMENU_GER_ICONIMG_DAT=207, VOLID_GTMENU_GER_SOLODATA_DAT=208, VOLID_GTMENU_ITA_GTMENUDAT_DAT=209, VOLID_GTMENU_ITA_GTMENUDAT_IDX=210, VOLID_GTMENU_ITA_ICONIMG_DAT=211, VOLID_GTMENU_ITA_SOLODATA_DAT=212, VOLID_GTMENU_JPN_GTMENUDAT_DAT=213, VOLID_GTMENU_JPN_GTMENUDAT_IDX=214, VOLID_GTMENU_JPN_ICONIMG_DAT=215, VOLID_GTMENU_JPN_SOLODATA_DAT=216, VOLID_GTMENU_SPA_GTMENUDAT_DAT=217, VOLID_GTMENU_SPA_GTMENUDAT_IDX=218, VOLID_GTMENU_SPA_ICONIMG_DAT=219, VOLID_GTMENU_SPA_SOLODATA_DAT=220, VOLID_GTMENU_USA_GTMENUDAT_DAT=221, VOLID_GTMENU_USA_GTMENUDAT_IDX=222, VOLID_GTMENU_USA_ICONIMG_DAT=223, VOLID_GTMENU_USA_SOLODATA_DAT=224, VOLID_LICENSE_A_LIA00_LGF=225, VOLID_LICENSE_E_LIA00_LGF=226, VOLID_LICENSE_I_LIA00_LGF=227, VOLID_REPLAY_SCEA_000=228, VOLID_REPLAY_SCEA_999=229, VOLID_REPLAY_SCEE_000=230, VOLID_REPLAY_SCEE_999=231, VOLID_REPLAY_SCEI_000=232, VOLID_REPLAY_SCEI_999=233, VOLID_SOUND_ARCADE_SEQ=234, VOLID_SOUND_ARCSEQ_INS=235, VOLID_SOUND_GTMSEQ_INS=236, VOLID_SOUND_SE01_INS=237, VOLID_SOUND_SPU_02_SEQ=238, VOLID_SOUND_SPU_03_SEQ=239, VOLID_SOUND_SPU_04_SEQ=240, VOLID_SOUND_SPU_05_SEQ=241, VOLID_SOUND_SPU_06_SEQ=242, VOLID_SOUND_SPU_07_SEQ=243, VOLID_SOUND_SPU_08_SEQ=244, VOLID_SOUND_SPU_09_SEQ=245, VOLID_SOUND_SPU_10_SEQ=246, VOLID_SOUND_SYS_INS=247, VOLID_COUNT=248 };
typedef unsigned short VolBootId16;

struct LangCarParamFiles { /* carparam files for one language. Rows: 0 default/JPN, 1 USA, 2 ENG, 3 FRA, 4 GER, 5 ITA, 6 SPA (European rows reuse ENG arcade/license data) */

    VolBootId16 gtmodeData;
    VolBootId16 arcadeData;
    VolBootId16 licenseData;
    VolBootId16 unistrdb;
    VolBootId16 gtmodeRace;
};

typedef struct McDirEntry McDirEntry, *PMcDirEntry;

struct McDirEntry {
    int size;
    uchar frame; /* its directory frame, 1-15 */
    uchar unk05;
    uchar state;
    signed char next; /* next block, -1 ends the chain */
    char name[24];
};

typedef struct NumberPicker NumberPicker, *PNumberPicker;

struct NumberPicker {
    short x;
    short y;
    short value;
    short min;
    short max;
    short height;
    uint rgb;
    struct Font *font;
    short unk14;
    short spacing;
    short unk18;
    short sfx;
    short anim;
    short pad1e;
};

typedef struct PadButtonState PadButtonState, *PPadButtonState;

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

typedef struct PadDevice PadDevice, *PPadDevice;
typedef struct PadHandler PadHandler, *PPadHandler;

struct PadDevice {
    short port;
    uchar type; /* controller type from the raw buffer, 0 = none */

    uchar connected;
    struct PadHandler *handlers; /* PadHandler table, e.g. 0x800a6f5c */

    struct PadDevice *prevDevice; /* device this one replaced on its port; restored on close */

    struct PadButtonState buttons;
    ushort analogMask; /* bit n set = analog[n] valid; 0xf for NeGcon and analog pads */

    ushort analog[4]; /* 0..0xff, centre 0x80 (NeGcon twist / sticks) */

    undefined field8_0x52[6];
    short unk58;
    short force; /* signed: its sign picks the direction bit of FUN_800834fc's actuator byte */

    short vibration1; /* 12.4 fixed point, sent as actuator byte 1 */

    short vibration0; /* 12.4 fixed point, sent as actuator byte 0 */

    short vibrationFrames; /* vibration fields are cleared when this counts down past 0 */

    undefined field21_0x62;
    undefined field22_0x63;
};

struct PadHandler { /* Entry of a PadDevice.handlers table, searched by type. */

    int type; /* controller type (typeLen>>4, 0 = none) */

    void (*update)(struct PadDevice *, ushort, ushort *, int);
    void (*post)(struct PadDevice *); /* also called when the device is closed */

};

typedef struct DrawLayer DrawLayer, *PDrawLayer;
typedef struct LayerHeader LayerHeader, *PLayerHeader;

struct DrawLayer {
    ushort priority; /* ViewDrawContext.head list is sorted by it */

    ushort otSize; /* OT entries for this layer (its slice of the view's OT) */

    struct DrawLayer *next;
    void *ot; /* this layer's OT slice; primitives are added here */

    struct LayerHeader *header; /* draw area and offset packet sent before the layer's primitives */

};

struct LayerHeader { /* 16-byte packet at the head of each layer, allocated by ViewDrawContext_Begin; DrawLayer_SetArea fills it */

    uint tag; /* packet link and length */

    uint drawAreaTopLeft; /* GP0(E3) */

    uint drawAreaBottomRight; /* GP0(E4) */

    uint drawOffset; /* GP0(E5) */

};

typedef struct RaceSetup RaceSetup, *PRaceSetup;
typedef struct RaceCar RaceCar, *PRaceCar;

struct RaceCar {
    uint carId;
    int colorCode;
    struct CarSetup setup;
    uchar active;
    uchar gridSlot;
    uchar driverType;
    uchar unk8f;
    char name[64];
};

struct RaceSetup {
    uchar options[8];
    uchar frameStep; /* vsyncs per race frame: 1 = 60 Hz, 2 = 30 Hz (sets g_FrameRate / g_FrameDt) */

    uchar game;
    uchar raceType;
    uchar licenseId;
    uchar licenseTest;
    uchar showIntro; /* 0 skips the pre-race camera intro */

    uchar unk0e;
    uchar lapCount; /* laps of the race; 0 = 100. 1-lap courses (Course flag 0x20) force 1 */

    char eventCode[16];
    char courseName[32];
    uint courseHash;
    char eventName[20];
    short dirt; /* dirt of the player car at the start (garage Car.dirt, 0..0x1000; x600000 >> 12 in the race) */

    uchar carCount;
    uchar pad5b;
    struct RaceCar cars[6];
    uchar unk53c[64];
    short eventIndex;
    char unk57e;
    uchar pad57f;
    char worldLeague; /* Event_IsWorldLeague */

    uchar pad581;
    short unk582;
    short garageSlot; /* garage slot of the player car (RaceSetup_SetCarFromGarage); -1 otherwise */

    short maxPower; /* Event power limit (Event_GetMaxPower); free race: player power + 5 */

    uint flags; /* bit0 tarmac event (not dirt), bits1-2 turbo rule (1 no turbo, 2 turbo required) */

};

typedef struct SeqChannel SeqChannel, *PSeqChannel;
typedef struct SeqRegion SeqRegion, *PSeqRegion;
typedef struct SeqProgram SeqProgram, *PSeqProgram;
typedef struct SeqBank SeqBank, *PSeqBank;

/* A sample and the notes it plays (0x14 bytes). */
struct SeqRegion {
    ushort addr; /* SPU address / 8 */
    short vol;
    ushort root; /* the note the sample plays at its pitch, in 1/256 semitones */
    ushort duration;
    uchar priority;
    uchar pan; /* 0x40 = centre */
    uchar flags;
    uchar pad0b;
    uchar low; /* notes played */
    uchar high;
    uchar bendDown; /* pitch bend range */
    uchar bendUp;
    uint adsr; /* ADSR registers 1 (low half) and 2 (high half) */
};

/* An instrument: count regions, each note played on all that cover it. */
struct SeqProgram {
    uchar count;
    uchar pan;
    short vol;
    uchar regions[1]; /* count of them, SeqBank.regions indices */
};

/* An instrument file ("INST"), relocated by Inst_Relocate. */
struct SeqBank {
    int magic;
    int base; /* the address its pointers are relative to */
    uchar unk08[4];
    int spu; /* the SPU address its regions' addr are relative to */
    int spuSize; /* bytes of sample data */
    void *unk14;
    uint regionCount;
    struct SeqRegion *regions;
    uint programCount;
    struct SeqProgram **programs;
};
typedef struct SeqPlayer SeqPlayer, *PSeqPlayer;
typedef struct SeqTrack SeqTrack, *PSeqTrack;
typedef struct SeqFade SeqFade, *PSeqFade;

struct SeqTrack {
    uchar *ptr;
    int delta;
    ushort trackVol;
    ushort effVol;
    short pitchBend;
    short pad0e;
    uchar pan; /* event 0x05 */

    uchar program; /* event 0x03 */

    signed char loopDepth; /* loop stack depth (events 0x01/0x20-0x3f push, 0x02 pops), -1 = none */

    uchar pad13;
    uchar *loopStart[4]; /* loop start pointers */

    uchar loopCount[4]; /* remaining loop counts (0xff = forever) */

};

struct SeqFade {
    ushort count;
    short step;
    short extra;
    short rate;
    short reload;
    short acc;
};

struct SeqPlayer {
    struct SeqPlayer *next;
    ushort baseVol;
    ushort vol;
    int tempo;
    int tickAcc;
    int tickPeriod;
    int ticksThisFrame;
    struct SeqBank *instBank;
    struct SeqFade fade; /* volume fade (SeqFade_Step each tick); count 0 = idle */

    struct SeqTrack tracks[16];
};

struct SeqChannel {
    struct SeqPlayer *player;
    struct SeqTrack *track;
    struct SeqProgram *program;
    struct SeqRegion *region;
    int releaseTime;
    ushort lastVol;
    short lastBend;
    uchar note;
    uchar velocity;
    uchar lastPan;
    signed char spuVoice; /* -1 = none */
};

typedef struct SfxLoop SfxLoop, *PSfxLoop;

/* A looping sound effect (SfxLoop_Update), started on its first update. */
struct SfxLoop {
    struct SeqRegion *sample;
    signed char voice; /* the SfxVoice handle, -1 = none */
    uchar pad05[3];
};

/* An RPM range of an engine sound file ("ENGN", 0x10 bytes): a sample and the RPMs it plays. */
typedef struct EngnRange EngnRange, *PEngnRange;

struct EngnRange {
    short rpm; /* the RPM the sample plays at pitch */
    short lo; /* the RPMs it plays alone */
    short hi;
    short vol;
    int pitch;
    uint addr; /* SPU address */
};

/* One of an engine sound's two voices, which cross-fade between ranges (0xc bytes). */
typedef struct EngineVoice EngineVoice, *PEngineVoice;

struct EngineVoice {
    signed char voice; /* the SfxVoice handle, -1 = none */
    signed char range; /* -1 = none */
    signed char prevRange;
    uchar pad03;
    union {
        short vol[2]; /* left, right */
        int volPair; /* both, copied as one word */
    };
    ushort pitch;
    ushort addr; /* SPU address / 8 */
};

/* A car's engine sound (0x28 bytes), playing one set of an ENGN file. */
typedef struct EngineSound EngineSound, *PEngineSound;

struct EngineSound {
    struct EngnRange *ranges;
    uchar count;
    uchar priority;
    short unk06;
    ushort rpm;
    short pitchScale; /* 0x1000 = 1 */
    short volL;
    short volR;
    struct EngineVoice voices[2];
};

/* The sound driver's state (g_Seq, 0x2d4 bytes): the playing sequences and the 25 channels they play on. */
typedef struct SeqState SeqState, *PSeqState;

struct SeqState {
    struct SeqPlayer *players; /* linked through next (SeqPlayer_Link) */
    ushort tickRate; /* Seq_SetTickRate */
    ushort masterVol;
    uchar nextChannel; /* where the channel search starts */
    uchar pad09[3];
    struct SeqFade masterFade;
    struct SeqChannel channels[25];
};

/* A sound effect's SPU settings, given to SfxVoice_Start (0x10 bytes). */
typedef struct SfxParams SfxParams, *PSfxParams;

struct SfxParams {
    union {
        short vol[2]; /* left, right */
        int volPair; /* both, copied as one word */
    };
    ushort pitch;
    ushort addr; /* SPU start address / 8 */
    uint adsr; /* ADSR registers 1 (low half) and 2 (high half) */
    ushort duration; /* in 1/256 ticks; 0 = until the envelope ends */
    uchar priority;
    char flags; /* bit 0: reverb */
};

/* One of the 24 SPU voices (0x28 bytes). The main code sets lock while it edits one, so
   SfxVoices_Update (in the vsync) skips it; lock 2 means its key-on and registers wait. */
typedef struct SfxVoice SfxVoice, *PSfxVoice;

struct SfxVoice {
    uchar state; /* 1 = started by SfxVoice_Start, owned through owner */
    uchar playing;
    uchar keyedOn;
    uchar priority;
    uchar lock;
    uchar dirty; /* registers to write: 1 pitch, 2 volume, 4 address and ADSR */
    uchar keyOn;
    uchar keyOff;
    ushort volL;
    ushort volR;
    ushort pitch;
    ushort addr;
    ushort adsr1;
    ushort adsr2;
    signed char *owner; /* the caller's voice handle, set to -1 when the voice stops */
    int age; /* ticks since key-on */
    uint duration;
    uint elapsed; /* sums the SPU pitch each tick, against duration */
    uchar reverb;
    uchar pad25[3];
};

/* g_SoundDriver (0x3c8 bytes). */
typedef struct SoundDriver SoundDriver, *PSoundDriver;

struct SoundDriver {
    signed char nextVoice; /* where the voice search starts */
    uchar reverbOn;
    uchar reverbDirty; /* the voices' reverb bits need writing */
    uchar pad03;
    ushort tickRate; /* Sound_SetTickRate */
    ushort tickScale; /* tickRate * 256 / 60 */
    struct SfxVoice voices[24];
};

typedef uint size_t;

/* WARNING! conflicting data type names: /stddef.h/size_t - /TYPES.H/size_t */

typedef struct SpuFileLoad SpuFileLoad, *PSpuFileLoad;

struct SpuFileLoad {
    uchar *sectorBuf;
    uchar *ramBase;
    uchar *ramCur;
    int headerSize;
    int headerLeft;
    int spuBase;
    int spuCur;
    int spuLeft;
    ushort carry;
    uchar phase;
    uchar started;
    uchar isEngine;
    uchar pad25[3];
};

typedef struct TextCtx TextCtx, *PTextCtx;

struct TextCtx {
    struct Font font; /* copy of the current font */

    ushort clut;
    ushort tpage;
    uint *ot; /* ordering-table entry glyphs are linked into */

    uint rgb; /* SPRT color, top byte cleared */

    uint unk18;
    uchar unk1c;
};

typedef struct VECTOR VECTOR, *PVECTOR;

struct VECTOR {
    long vx;
    long vy;
    long vz;
    long pad;
};

enum {  };
typedef unsigned int VolBootId;
typedef struct CarAsset CarAsset;

struct CarAsset { /* g_CarAssets, sorted by carCode: a car's files start at file, up to the next entry's */
    uint carCode; /* CarCode_Encode */
    ushort file; /* the car's first file: model, then texture */
    ushort logoFile; /* its maker logo (a TIM) */
};

typedef struct VolHeader VolHeader, *PVolHeader;

struct VolHeader { /* First 0xc000 bytes of GT2.VOL, copied to g_VolHeader at boot */

    char magic[4]; /* 'GTFS' */

    uint unk_04; /* 0 */

    uint unk_08; /* 0x2d642d3d on NTSC-U 1.2 */

    uint unk_0c; /* 0 */

    uint toc[12284]; /* File table: entry & ~0x7ff = byte offset in GT2.VOL, entry & 0x7ff = unused bytes at the end of the file's last sector. size(i) = (toc[i+1] & ~0x7ff) - toc[i]. Ends with the VOL size, then 0. #0 = this table, #1 = the directory */

};

typedef struct WheelDims WheelDims, *PWheelDims;

struct WheelDims {
    short radius;
    short width;
    short scale;
    short offset;
};

// Types declared with the decompiled resident code, by the file that
// first needed them.

// exe/bootlogo.cpp

// A splash screen of g_SplashScreens, which ends with a null timGz.
struct SplashScreen {
    void *timGz;                // gzip-compressed TIM
    unsigned short frames;
    unsigned char fadeOut;
    unsigned char hiRes;        // 640 pixels wide
};

class VendorBootLogo : public psxViewLoopDoubleBuffer {
public:
    DrawLayer layer;
    int frame;
    int duration;               // frames to show
    int fadeOut;                // fade out over the last 16 frames
    int yOffset;
    int hiRes;

    VendorBootLogo();
    virtual ~VendorBootLogo();
    virtual int Update();
    virtual void Draw();
    void InitLayer();
};

// exe/cdstream.cpp

// The frame timers' samples of root counter 1 (hblanks) and 2 (system clock / 8).
struct TimerSample {
    unsigned short hblank;
    unsigned short clock;
    unsigned short hblankDelta;
};

// A read request as CdAudio holds it at 0x20 (reqFlag..reqWrite, the next one)
// and 0x40 (flag..ringWrite, the current one).
struct CdReq {
    unsigned char flag;
    unsigned char mode;         // CdlSetmode bits
    unsigned char file;         // XA filter file and channel
    unsigned char channel;
    unsigned int start;         // first sector
    unsigned int end;           // end sector (exclusive)
    void *onSector;
    void *dst;                  // destination or ring buffer
    unsigned int size;          // bytes, or ring sectors
    unsigned short skip;        // bytes to skip in the first sector
    unsigned short fill;        // the ring's indices
    unsigned short read;
    unsigned short write;
};

// exe/carmodel.cpp

// A car instance's wheel: position, and the spin direction (+-0x400) and angles.
struct WheelInst {
    short x;
    short y;
    short z;
    short pad6;
    short spin;
    short angleA;
    short angleB;
    short padE;
};

struct CarModelInst {
    unsigned char lod;          // 0: by distance, else LOD + 1
    unsigned char detail;
    unsigned char unk02;
    unsigned char drawShadow;
    unsigned char brightness;
    unsigned char pad05;
    short unk06;
    unsigned short tpage;
    short pad0a;
    CarModelHeader *model;
    MATRIX world;
    WheelInst wheels[4];
    MATRIX shadow;
};

// The colour codes of a car texture: count, then the letters.
struct CarTex {
    unsigned short count;
    char codes[1];
};

// The wheels of a car model (CarModelHeader from wheelTexture): the hub half widths, the hub
// sprite corners and the front and rear wheel meshes of the 3 LODs.
struct WheelGeom {
    unsigned int texture;
    short frontHalfWidth;
    short rearHalfWidth;
    unsigned int frontQuad[4];
    int pad18;
    unsigned int rearQuad[4];
    int pad2c;
    int frontLod0[0x82];
    int rearLod0[0x82];
    int frontLod1[0x46];
    int rearLod1[0x46];
    int frontLod2[0x37];
    int rearLod2[0x37];
};

// A car body mesh (one LOD): the counts, then pointers set by CarModel_Init to the vertices,
// normals and each primitive type's list, which follow the 0x50-byte header.
struct CarMesh {
    unsigned short vertCount;
    unsigned short normalCount;
    unsigned short primCount[8];
    char *verts;
    char *normals;
    char *prims[8];
    short bboxMin[4];
    short bboxMax[4];
    unsigned short unk4c;
    unsigned short unk4e;
};

struct LodEntry {
    int dist;
    CarMesh *mesh;
};

// CarModelHeader.lodCount and the LOD entries after it; the meshes follow.
struct LodTable {
    int count;
    LodEntry lods[1];
};

// The camera fields CarModel_Draw and CarModel_DrawShadow copy to the scratchpad.
struct CameraView {
    int data[0x1a];
};

// What CarModel_Draw sets up in the scratchpad (0x1f800000) for the body and wheel drawing.
struct CarScratch {
    // A copy of the Camera from view to ot (CameraView), the view composed with the
    // car's world matrix.
    MATRIX view;
    int negPos[3];
    short clipX[9];
    short pad3e;
    short clipY[9];
    short pad52;
    int ofx;
    int ofy;
    short projH;
    short screenW;
    short screenH;
    short nearZ;                // 0x62
    unsigned int *ot;           // 0x64
    char *packetPtr;            // 0x68
    char *packetLimit;
    char *verts;                // 0x70: the mesh's vertices (ShadowMesh_Draw: transformed ones)
    char *normals;              // the environment map coordinates of the normals
    int savedTrans[3];          // 0x78: the body transform, which CarBody_Draw* reload
    int savedRot[5];
    short otShift;              // 0x98: depth << otShift >> 13 is the OT index
    short pad9a;
    MATRIX wheelMat;            // 0x9c: CarModel_DrawWheel's wheel rotation and position
    char unkbc[0x2dc];
    signed char mirror;         // 0x398
    signed char lit;
    char pad39a[2];
    unsigned int detail;        // 0x39c: added to the textured faces' CLUT word
    unsigned int clutSelect;    // 0x3a0: masks a face's flags >> 14 into its CLUT word
    unsigned int envFT3;        // 0x3a4: the brightness, with semi-transparent POLY_FT3's code
    unsigned int envFT4;        // 0x3a8: the same with POLY_FT4's
    short envClut;              // 0x3ac: the environment map's CLUT and texture page
    unsigned short envTpage;
    unsigned int wheelColor;    // 0x3b0
};

// A body vertex as the GTE loads it (VXY, VZ).
struct BodyVert {
    int xy;
    int z;
};

// A vertex of a face being subdivided: its model position, texture coordinates, screen
// position and colour.
struct SubVert {
    short x;
    short y;
    short z;
    unsigned char u;
    unsigned char v;
    int sxy;
    unsigned int rgb;
};

// What the subdivision of one face shares: where its packets go, its code and texture
// words, the area above which a part is subdivided again, and the chain of packets.
struct SubFace {
    unsigned int *ot;
    unsigned int code;          // the face's colour and command word
    unsigned int texWord;       // the texture page (top half) and CLUT (bottom half)
    int threshold;
    int gouraud;
    char *first;
    char *last;
};

// A transformed vertex (CarBody_DrawClipped, ShadowMesh_Draw): view-space x, y, z, the
// clip code (bit 0 nearer than half the near plane, bits 1-4 outside the planes in the
// light and light colour matrices, bit 5 a GTE error), the screen position and the or of
// |x|, |y| and |z|.
struct ShadowVert {
    short x;
    short y;
    short clip;
    short z;
    int sxy;
    unsigned int extent;
};

// One vertex pair of a wheel strip: the first vertex's x and y, both z, the second's x and y.
struct WheelPair {
    int xyA;
    unsigned short zA;
    unsigned short zB;
    int xyB;
};

// A shadow mesh: the counts, then the vertices (x and z, a word each), the triangles and
// the quads (6-bit vertex indices packed in a word, bit 31 darkening the first two).
struct ShadowMesh {
    unsigned short vertCount;
    unsigned short triCount;
    unsigned short quadCount;
    char unk6[0x12];
    unsigned short unk18;
    unsigned short unk1a;
    unsigned int data[1];
};

// Poly_Clip's input, in the scratchpad at 0x1f80009c.
struct ShadowClipVert {
    int xy;
    short z;
    short extent;
    int sxy;
    unsigned int rgbc;          // the colour, with the clip code in the top byte
};

struct ShadowClip {
    unsigned int *ot;
    unsigned int code;
    int pad;
    ShadowClipVert v[4];
    ShadowVert *src[4];
};

// exe/buttonprompt.cpp

struct ButtonPromptDesc {
    short x;                    // centre
    short y;
    char *text;
    uint textRgb;
    uint color;                 // box colour
    uint color2;                // bottom of the box gradient
    uchar style;                // bits 1-2: size; bit 2: text 4 pixels higher
    signed char spacing;
    char textHeight;
    uchar pad17;
    Font *font;
};

struct ButtonPrompt {
    short x;
    short y;
    TextItem text;
    ushort flags;               // style; bit 7: highlighted while opening
    ushort width;
    ushort height;
    short anim;                 // frames since Open; < 0 closing, -1 closed
    uint color;
    uint color2;
};

// exe/carddialog.cpp

// What CardDialog_Init builds a dialog from (0x30 bytes).
struct CardDialogDesc {
    short x;                    // centre
    short y;
    char *message;
    char *option0;
    char *option1;
    uint messageRgb;            // bit 25: the text's alternative style
    uint optionRgb;
    uint color;                 // box colour
    uint color2;                // bottom of the box gradient
    ushort flags;               // bits 1-2: size; bit 2: text 4 pixels higher;
                                // bit 0: boxes slide in from the left; bit 3: close on a choice
    signed char spacing;        // of the options
    uchar pad23;
    short textHeight;
    short pad26;
    Font *font;
    signed char sfx;
    uchar pad2d[3];
};

// A rectangle of a texture page (FUN_8006ea38).
struct Sprite {
    uchar u;
    uchar v;
    ushort clut;
    short w;
    short h;
    ushort tpage;
};

// exe/cardmenu.cpp

// What an overlay hands CardMenu_Setup.
struct CardMenuConfig {
    CardMenu *work; // becomes g_CardMenu
    void *kanjiFont;
    short *slotStatus;
    Font *fontSmall;
    Font *fontMedium;
    Font *fontLarge;
    short textPri;
};

// exe/carinfo.cpp

struct CourseEntry {
    char *name;
    unsigned int hash;
    unsigned short flags;
    char unkA[0xe];
};

struct CourseInfo {
    char magic[6];
    unsigned short count;
    CourseEntry entries[1];
};

// exe/carparams.cpp

// A CarSpec pointer and the CarSetup index it comes from, with the game-data table.
struct CarSpecPartLink {
    short specOffset;
    short setupOffset;
    short table;
};

// One part category of CarParams_Build: the game-data table, the CarSpec field holding the
// part's entry and the descriptors that copy it into the CarParams.
struct CarParamsPartDesc {
    unsigned short table;
    unsigned short specOffset;
    unsigned int *desc;
};

// exe/enginecurve.cpp

// The car name file: a count, then 0x48-byte entries starting with the car id.
struct CarNameEntry {
    int id;
    char name[0x44];
};

struct CarNameTable {
    short count;
    CarNameEntry entries[1];
};

// exe/font.cpp

// Upload of the race font (VOL entry 0xc2) to VRAM, eight lines per sector.
struct RaceFontUpload {
    void *buf;      // sector buffer
    RECT rect;      // VRAM rectangle of the next sector
    ushort tpageId; // texture page the font goes to
    short unkE;
    short step;     // lines per sector
    ushort remaining; // sectors left
};

// exe/gamedata.cpp

// A game data file: a header, then tableCount tables, the second half being the
// string pools of the first half's tables.
struct GameData {
    int unk0;
    ushort unk4;
    ushort tableCount;
    GameDataTable tables[1];
};

struct IdList {
    int count;
    unsigned int ids[1];        // count of them
};

// exe/garage.cpp

// One car setting: its value and range (CarWork_GetSettingRange); id is set for the
// LSD settings only.
struct SettingRange {
    short value;
    short min;
    short max;
    short id;
};

// exe/gte.cpp

// The scratchpad's first 0x9c bytes as the drawing code lays them out (carmodel.cpp's
// CarScratch): a copy of a Camera from its view matrix on, then the drawing state.
struct ViewScratch {
    MATRIX view;
    int negPos[3];
    short clipX[9];             // 0x2c: loaded as the light matrix; rows 0 and 1 are the
    short pad3e;                // left and right planes' normals
    short clipY[9];             // 0x40: loaded as the light colour matrix; the top and
    short pad52;                // bottom planes' normals
    int ofx;
    int ofy;
    short projH;
    short screenW;
    short screenH;
    short nearZ;                // 0x62
    unsigned int *ot;
    char *packetPtr;            // 0x68
    char *packetLimit;
    char *verts;
    char *normals;
    int savedTrans[3];          // 0x78: the view, scaled down by Gte_LoadScaled
    short savedRot[10];
    short otShift;              // 0x98: the translation's shift
    short rotShift;             // 0x9a: the rotation's shift
};

// A point for Clip_TestPoints.
struct ClipPoint {
    short x;
    short y;
    short z;
    short pad;
};

// A MATRIX's rotation alone, for the temporary matrices that Matrix_Mul takes.
struct MatrixRot {
    short m[3][3];
};

// The scratchpad with Clip_TestBox's corners after the drawing state.
struct BoxScratch {
    ViewScratch view;
    ClipPoint box[8];
};

// Poly_Clip's input.
struct PolyClipIn {
    unsigned int *ot;           // 0x9c
    unsigned int code;          // the POLY_G3 or POLY_G4 code, in the top byte
    int pad;
    struct {
        int xy;                 // view-space position
        short z;
        short extent;           // |x| | |y| | |z|
        int sxy;
        unsigned int rgbc;      // the colour (the top byte isn't used)
    } v[4];
};

struct ClipVert {
    short x;                    // the position, scaled
    short y;
    short z;
    short pad;
    int sxy;
    unsigned int rgbc;          // a cut point keeps the record's old top byte
    ClipVert *prev;
    ClipVert *next;
    unsigned short code;        // 0x18: bit 0 nearer than nearZ, bits 1-4 outside a side
    short dist[5];              // the distance to each plane, negative outside
};

// The working state after the input, through 0x2e8.
struct PolyClip {
    ClipVert pool[14];          // 0xe8
    ClipVert *head;             // 0x2e0
    ClipVert *free;             // 0x2e4
};

// exe/kanjifont.cpp

// A cached glyph: a node of the binary trie KanjiFont_FindGlyph walks with the code's
// bits, and the texture coordinates of its cell.
struct KanjiGlyph {
    KanjiGlyph *left;  // code bit clear
    KanjiGlyph *right; // code bit set
    short x, y;        // cell position in the pixel buffer
    unsigned short code;
    unsigned char rendered;
    unsigned char wide; // 24 rows tall instead of 16
    unsigned short clut;
    unsigned short tpage;
    unsigned short uv;
};

// A glyph cache: a 4-bit texture of 16-pixel-wide cells and the glyph nodes that own them.
struct KanjiFont {
    unsigned char *pixels;
    RECT texRect; // in VRAM (w in 16-bit units)
    unsigned short tpage;
    unsigned short clut;
    KanjiGlyph *nodes;
    KanjiGlyph *root;
    KanjiGlyph *next; // the next free node
    short col, row;   // the next free cell
    short cols;       // cells a row
    unsigned char pad22;
    unsigned char wideMode;
};

// exe/overlay.cpp

// The overlay pack: compressed overlays, by offset from the pack's start.
struct OverlayEntry {
    int offset;
    int size;
};

struct OverlayPack {
    int unk0;
    int unk4;
    char *data; // the pack in memory, or 0 to read it from the CD
    OverlayEntry entries[1];
};

// exe/powergraph.cpp

// The power and torque graph of a car (g_ChangeParts_PowerGraph): two vertical scales,
// the rpm scale and the curves, which fade in tick by tick.
struct PowerGraphView {
    short stagger;          // frames from the first tick's fade-in to the last one's
    short x, y, w, h;
    signed char tpage;      // given to Text_Init
    signed char labelGap;   // between a scale's labels
    signed char textMode;
    signed char charH;
    unsigned char unk0e[2];
    Font *font;
    unsigned int powerRgb;
    unsigned int torqueRgb;
    unsigned int rpmRgb;
    short maxRpm;
    short maxPower;         // hp
    short maxTorque;        // lb-ft
    short frame;            // the fade-in frame; -1 hidden, below -1 closing
    short powerTop;         // the power scale: its top, ticks and step
    short powerTicks;
    short powerStep;
    short torqueTop;
    short torqueTicks;
    short torqueStep;
    short rpmTicks;         // one per 1000 rpm
    short samples;          // the curves' samples, one per 250 rpm
    short endFrame;
};

// exe/records.cpp

// A race's lap record: the last 10 laps, the best one and the lap being driven.
struct RaceRecord {
    short unk00;
    short lapCount;         // laps done, up to 999
    short lapsKept;         // laps in laps, up to 10
    short bestLap;          // the lap number of best, -1 for none
    TimeRecord laps[10];    // the last lapsKept laps, oldest first
    TimeRecord best;        // 0xd0
    TimeRecord current;     // 0xe4
    int unkf8;
};

// The 9 laps RaceRecord_AddLap moves down, copied as one block.
struct TimeRecords9 {
    TimeRecord r[9];
};

// The game's state from g_GameState on: g_RaceSetup is part of it.
struct GameStateRace {
    char unk0[0x218];
    CourseRecord courseRecords[128];
    char unk1418[0xbf7c - 0x1418];
    RaceSetup race; // g_RaceSetup
};

// exe/replaydata.cpp

// A car's replay record in g_GameState, 0x4518 bytes, copied as one block.
struct ReplayRecord {
    char data[0x4518];
};

// exe/replaydir.cpp

struct Str3 {
    char c[3];
};

// exe/replaydraw.cpp

// How the card menu's replay list draws its rows (D_800921BC).
struct ReplayEntryStyle {
    Font *titleFont;
    short unk4;
    short titleArg; // Text_DrawString's last argument
    Font *fieldFont;
    short unkc;
    short spacing; // the fields' letter spacing
    uint rgb;      // the row's background
};

// What ReplayEntry_DrawField draws with.
struct ReplayField {
    uint *ot;
    TextCtx *text;
    short spacing;
    short fade; // 0-128, towards black
    uint rgb;   // the text's colour
    uint tickRgb;
};

// TextCtx's tpage as the GPU's bits.
struct TextTpage {
    uint clut : 16;
    uint page : 5;
    uint abr : 2; // semi-transparency mode
    uint rest : 9;
};

// exe/seq.cpp

struct SeqSong {
    ushort vol;
    ushort pad02;
    int tempo;
    uchar *tracks[16];
};

// A sequence file ("SEQG"): its songs, with the track pointers as offsets from base.
struct SeqFile {
    int magic;
    int base;
    unsigned int count;
    SeqSong songs[1];
};

// exe/sfx.cpp

// A sound bank in SPU RAM: its instrument file in main RAM and where its samples went.
struct SpuBank {
    SeqBank *ram;
    int size;
    int spu;
};

// An engine sound file ("ENGN"): its two sets of RPM ranges.
struct EngnFile {
    int magic;
    int base;
    uchar unk08[4];
    int spu;
    int spuSize;
    void *unk14;
    struct {
        uint count;
        EngnRange *ranges;
    } sets[2];
};

// An SPU voice's registers (0x1f801c00 + voice * 0x10).
struct SpuVoiceRegs {
    ushort volL;
    ushort volR;
    ushort pitch;
    ushort addr;
    ushort adsr1;
    ushort adsr2;
    ushort envx; // the envelope's current level
    ushort repeat;
};

// exe/textitem.cpp

// The style template TextItem_Init copies (the first 0x1c bytes of a TextItem).
struct TextStyle {
    uchar charsPerStep;
    uchar unk01;
    short revealSteps;
    short zoomWidth;
    short closeSteps;
    short fadeSteps;
    signed char spacing;
    signed char height;
    ushort flags;
    ushort pad0e;
    Font *font;
    uint rgbFrom;
    uint rgbTo;
};

// exe/widgets.cpp

// A bar that grows from its centre over duration frames, its color going from
// rgbFrom to rgbTo; a negative t counts back down.
struct GrowBar {
    uint rgbFrom;
    uint rgbTo;
    short x;
    short y;
    short width;
    short height;
    short duration;
    short t;
};

// A disc of 24 triangles around its centre point, flat on the ground.
struct DiscPoint {
    int x, y, z;
};

struct GroundDisc {
    DiscPoint rim[25];
    DiscPoint centre;
};

// A projected point in the scratchpad (Points_Project).
struct ScreenPoint {
    short x, y;
    short z;
    short pad;
};

// Types of the subsystems with headers of their own.
#include "mccard.h"
#include "pad.h"

// ovl3/initevent.cpp

// An event of the race data (the event data, or g_GtModeRaceData in GT mode).
struct RaceEvent {
    unsigned short codeString;      // strings of the race data
    unsigned short courseString;    // 0x2
    char unk4[0x44 - 0x4];
    unsigned char noIntro;          // 0x44: RaceEventParams start here
    unsigned char lapCount;         // 0x45, copied to ChampRaceList.param45
    char unk46;
    unsigned char license;          // 0x47, see EventFlags::License
    char unk48[0x75 - 0x48];
    unsigned char dirt;             // 0x75
    unsigned char allowedCarsList;  // 0x76, 1-based list of game data table 2, 0 = any car
    unsigned char drivetrain;       // 0x77
    unsigned short prizes[6];       // 0x78, in hundreds of credits
    unsigned int prizeCars[4];      // 0x84, 0 ends the list
    unsigned short nameString;      // 0x94
    short maxPower;                 // 0x96, 0 = no limit
    unsigned short championshipPrize; // 0x98, in hundreds of credits
    unsigned char turbo;            // 0x9a
    unsigned char racingMod;        // 0x9b
};

// The race parameters of an event (g_RaceEventParams), copied from the event.
struct RaceEventParams {
    char data[0x40];
};

struct RaceOptions {
    char data[8];
};

// An entry of game data table 5.
struct Table5Entry {
    char unk0[8];
    unsigned int carId;            // 0x8
    char unkc[2];
    unsigned char fixedCar;        // 0xe
};

// ovl3/racesetup.cpp

// A car picked for a race: car, colour and the entry it came from.
struct RaceCarPick {
    unsigned int carId;
    signed char flags;            // 0x4
    signed char colorCode;        // 0x5
    unsigned short colorRgb;      // 0x6
    short entryIndex;             // 0x8
    short pad0a;
};

// ovl3/garagerace.cpp

// The arcade race selection that OVL2 leaves at 0x801d5fa0 (g_RaceCarWorkAlt).
struct ArcadeRaceSel {
    char unk0[2];
    signed char raceType;     // 0: 2-player battle, 4: single race, 6: time trial
    char unk3[3];
    short garage;             // 0x6
    short slot;               // 0x8
    char unkA[0x18 - 0xa];
    signed char unk18;        // RaceCar.unk8f of the player's car
    char unk19[0xa0 - 0x19];
    short garages[2];         // 0xa0: 2-player battle, per player
    short slots[2];           // 0xa4
    char unkA8[0xb6 - 0xa8];
    signed char unkB6[2];     // 0xb6: RaceCar.unk8f per player
    char unkB8[0x2c8 - 0xb8];
    int useGarageCars;        // 0x2c8
    int dirt;                 // 0x2cc: dirt course (dirt tyres)
    char unk2D0[0x2d4 - 0x2d0];
};

// ovl3/storesetup.cpp

// The top 3 bits of a halfword (Car.stat94's drivetrain).
struct Hi3 { unsigned short lo:13; unsigned short hi:3; };

#endif
