// Types of the resident code, exported from the Ghidra project (DataTypeWriter)
// and adapted for cc1plus: enums as sized integers, C++ keywords renamed, bool
// as a byte (Ghidra's size; GCC 2.8's bool is a word).
// Only the types that the decompiled resident files use. See docs/decomp.md.
#ifndef RESIDENT_H
#define RESIDENT_H

#define bool unsigned char
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
    char spacing;
    char height;
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
    char sfx;
    uchar choice;
    short boxWidth;
    short boxHeight;
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

};

typedef struct CardMenu CardMenu, *PCardMenu;

enum { CARDMODE_SAVE_REPLAY=0, CARDMODE_PLAY_REPLAY=1, CARDMODE_EDIT_REPLAY=2, CARDMODE_LOAD_GHOST=3, CARDMODE_SAVE_GAME=4, CARDMODE_LOAD_GAME=5 };
typedef unsigned short CardMode;

enum { PAD_UP=1, PAD_DOWN=2, PAD_LEFT=4, PAD_RIGHT=8, PAD_L1=16, PAD_L2=32, PAD_L3=64, PAD_TRIANGLE=256, PAD_CROSS=512, PAD_SQUARE=1024, PAD_CIRCLE=2048, PAD_R1=4096, PAD_R2=8192, PAD_R3=16384, PAD_START=65536, PAD_SELECT=131072 };
typedef unsigned int PadButton;
typedef struct WipeBar WipeBar, *PWipeBar;
typedef struct TextEntry TextEntry, *PTextEntry;
typedef struct CardProgressBar CardProgressBar, *PCardProgressBar;
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
    uchar infoTruncated;
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

struct ReplayDir {
    struct McFileHeader header;
    char count;
    uchar pad201;
    short totalSectors;
    short sectorMapCount;
    short sectorMap[960];
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
    char segFade[32];
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

typedef struct CdAudio CdAudio, *PCdAudio;
typedef struct VSyncCallback VSyncCallback, *PVSyncCallback;

struct VSyncCallback {
    struct VSyncCallback *prev;
    struct VSyncCallback *next;
    void (*func)(void);
    uint priority; /* list is sorted by ascending priority */

};

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
    undefined field35_0x64;
    undefined field36_0x65;
    undefined field37_0x66;
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
    undefined field51_0x8c;
    undefined field52_0x8d;
    undefined field53_0x8e;
    undefined field54_0x8f;
    undefined field55_0x90;
    undefined field56_0x91;
    undefined field57_0x92;
    undefined field58_0x93;
    undefined field59_0x94;
    undefined field60_0x95;
    undefined field61_0x96;
    undefined field62_0x97;
    undefined field63_0x98;
    undefined field64_0x99;
    undefined field65_0x9a;
    undefined field66_0x9b;
    undefined field67_0x9c;
    undefined field68_0x9d;
    undefined field69_0x9e;
    undefined field70_0x9f;
    undefined field71_0xa0;
    undefined field72_0xa1;
    undefined field73_0xa2;
    undefined field74_0xa3;
    undefined field75_0xa4;
    undefined field76_0xa5;
    undefined field77_0xa6;
    undefined field78_0xa7;
    undefined field79_0xa8;
    undefined field80_0xa9;
    undefined field81_0xaa;
    undefined field82_0xab;
    undefined field83_0xac;
    undefined field84_0xad;
    undefined field85_0xae;
    undefined field86_0xaf;
    undefined field87_0xb0;
    undefined field88_0xb1;
    undefined field89_0xb2;
    undefined field90_0xb3;
    undefined field91_0xb4;
    undefined field92_0xb5;
    undefined field93_0xb6;
    undefined field94_0xb7;
    undefined field95_0xb8;
    undefined field96_0xb9;
    undefined field97_0xba;
    undefined field98_0xbb;
    undefined field99_0xbc;
    undefined field100_0xbd;
    undefined field101_0xbe;
    undefined field102_0xbf;
    undefined field103_0xc0;
    undefined field104_0xc1;
    undefined field105_0xc2;
    undefined field106_0xc3;
    undefined field107_0xc4;
    undefined field108_0xc5;
    undefined field109_0xc6;
    undefined field110_0xc7;
    undefined field111_0xc8;
    undefined field112_0xc9;
    undefined field113_0xca;
    undefined field114_0xcb;
    undefined field115_0xcc;
    undefined field116_0xcd;
    undefined field117_0xce;
    undefined field118_0xcf;
    undefined field119_0xd0;
    undefined field120_0xd1;
    undefined field121_0xd2;
    undefined field122_0xd3;
    undefined field123_0xd4;
    undefined field124_0xd5;
    undefined field125_0xd6;
    undefined field126_0xd7;
    undefined field127_0xd8;
    undefined field128_0xd9;
    undefined field129_0xda;
    undefined field130_0xdb;
    undefined field131_0xdc;
    undefined field132_0xdd;
    undefined field133_0xde;
    undefined field134_0xdf;
    undefined field135_0xe0;
    undefined field136_0xe1;
    undefined field137_0xe2;
    undefined field138_0xe3;
    undefined field139_0xe4;
    undefined field140_0xe5;
    undefined field141_0xe6;
    undefined field142_0xe7;
    undefined field143_0xe8;
    undefined field144_0xe9;
    undefined field145_0xea;
    undefined field146_0xeb;
    undefined field147_0xec;
    undefined field148_0xed;
    undefined field149_0xee;
    undefined field150_0xef;
    undefined field151_0xf0;
    undefined field152_0xf1;
    undefined field153_0xf2;
    undefined field154_0xf3;
    undefined field155_0xf4;
    undefined field156_0xf5;
    undefined field157_0xf6;
    undefined field158_0xf7;
    undefined field159_0xf8;
    undefined field160_0xf9;
    undefined field161_0xfa;
    undefined field162_0xfb;
    undefined field163_0xfc;
    undefined field164_0xfd;
    undefined field165_0xfe;
    undefined field166_0xff;
    undefined field167_0x100;
    undefined field168_0x101;
    undefined field169_0x102;
    undefined field170_0x103;
    undefined field171_0x104;
    undefined field172_0x105;
    undefined field173_0x106;
    undefined field174_0x107;
    undefined field175_0x108;
    undefined field176_0x109;
    undefined field177_0x10a;
    undefined field178_0x10b;
    undefined field179_0x10c;
    undefined field180_0x10d;
    undefined field181_0x10e;
    undefined field182_0x10f;
    undefined field183_0x110;
    undefined field184_0x111;
    undefined field185_0x112;
    undefined field186_0x113;
    undefined field187_0x114;
    undefined field188_0x115;
    undefined field189_0x116;
    undefined field190_0x117;
    undefined field191_0x118;
    undefined field192_0x119;
    undefined field193_0x11a;
    undefined field194_0x11b;
    undefined field195_0x11c;
    undefined field196_0x11d;
    undefined field197_0x11e;
    undefined field198_0x11f;
    undefined field199_0x120;
    undefined field200_0x121;
    undefined field201_0x122;
    undefined field202_0x123;
    undefined field203_0x124;
    undefined field204_0x125;
    undefined field205_0x126;
    undefined field206_0x127;
    undefined field207_0x128;
    undefined field208_0x129;
    undefined field209_0x12a;
    undefined field210_0x12b;
    undefined field211_0x12c;
    undefined field212_0x12d;
    undefined field213_0x12e;
    undefined field214_0x12f;
    undefined field215_0x130;
    undefined field216_0x131;
    undefined field217_0x132;
    undefined field218_0x133;
    undefined field219_0x134;
    undefined field220_0x135;
    undefined field221_0x136;
    undefined field222_0x137;
    undefined field223_0x138;
    undefined field224_0x139;
    undefined field225_0x13a;
    undefined field226_0x13b;
    undefined field227_0x13c;
    undefined field228_0x13d;
    undefined field229_0x13e;
    undefined field230_0x13f;
    undefined field231_0x140;
    undefined field232_0x141;
    undefined field233_0x142;
    undefined field234_0x143;
    undefined field235_0x144;
    undefined field236_0x145;
    undefined field237_0x146;
    undefined field238_0x147;
    undefined field239_0x148;
    undefined field240_0x149;
    undefined field241_0x14a;
    undefined field242_0x14b;
    undefined field243_0x14c;
    undefined field244_0x14d;
    undefined field245_0x14e;
    undefined field246_0x14f;
    undefined field247_0x150;
    undefined field248_0x151;
    undefined field249_0x152;
    undefined field250_0x153;
    undefined field251_0x154;
    undefined field252_0x155;
    undefined field253_0x156;
    undefined field254_0x157;
    undefined field255_0x158;
    undefined field256_0x159;
    undefined field257_0x15a;
    undefined field258_0x15b;
    undefined field259_0x15c;
    undefined field260_0x15d;
    undefined field261_0x15e;
    undefined field262_0x15f;
    byte speedChanged; /* set when the double-speed bit changed */

    byte cddaBits; /* modeParam & 0x41 */

    byte lastCmd; /* last CdControlF command */

    undefined field266_0x163;
    undefined field267_0x164;
    undefined field268_0x165;
    byte state; /* 3 = CdControlF in flight */

    undefined field270_0x167;
    undefined field271_0x168;
    undefined field272_0x169;
    undefined field273_0x16a;
    undefined field274_0x16b;
};

typedef struct ChampRaceList ChampRaceList, *PChampRaceList;

struct ChampRaceList { /* Races of the current championship (ChampRaceList_Build), read by OVL0. */

    short unk00;
    short raceCount; /* from the last 2 digits of the event base code; 1 for a single race */

    uint courseHash[6]; /* Str_Hash of each race course name (random course when "none") */

    ushort param45[6]; /* event entry byte +0x45 of each race */

    char raceCode[6][16]; /* race event codes (base + %02d) */

    uchar unk88[12];
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

    short maxPower;
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

typedef struct GfxIrqState GfxIrqState, *PGfxIrqState;

struct GfxIrqState { /* Game-side IRQ and display state, cleared by Irq_Install */

    uint vblankTotal; /* every vblank; Irq_WaitVSyncs returns it */

    int vblanksSinceFlip; /* every vblank; reset to 0 by Irq_WaitVSyncs and Gpu_WaitField. psxViewLoop::UpdateDisplay swaps once it reaches swapPending */

    struct VSyncCallback *vsyncCallbacks; /* priority-sorted list run by Irq_VSyncHandler (VSyncList_Add / VSyncList_Remove) */

    void (*gpuDmaHook)(void); /* called after each GPU DMA completion; overlays chain their own (old value saved by the installer) */

    uint displayMode; /* last GP1(08) mode written by Gpu_SetDisplayMode: bits 0-1 hres, 2 vres/interlace, 3 PAL, 6 hres 368 */

    int gpuDmaCount;
    int gpuDmaDoneTime; /* Timer_SinceFrameStart at the last GPU DMA completion; menus copy it as their draw time */

    int gpuDmaDoneHblanks;
    uint gpustatAtWait; /* GPUSTAT when Irq_WaitVSyncs started; Gpu_WaitField waits for its bit 31 (odd/even field) to flip */

    uchar imageTransferBusy; /* 1 during Gpu_TransferImage */

    uchar imageDmaPending; /* 1 from Gpu_TransferImage's DMA start until Irq_GpuDmaHandler */

    undefined field11_0x26;
    undefined field12_0x27;
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
    uchar unk04;
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

typedef struct PacketBuffer PacketBuffer, *PPacketBuffer;

struct PacketBuffer { /* Double-buffered primitive packet pool; g_PacketPtr allocates from the current half */

    void *base;
    uint size; /* words, both halves */

    uint half; /* 0 or size/2 */

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

struct PadHandler { /* Entry of a PadDevice.handlers table, searched by type. */

    int type; /* controller type (typeLen>>4, 0 = none) */

    void (*update)(struct PadDevice *, ushort, ushort *, int);
    void (*post)(struct PadDevice *); /* also called when the device is closed */

};

typedef struct psxViewLoop psxViewLoop, *PpsxViewLoop;
typedef struct psxViewLoop_vtbl psxViewLoop_vtbl, *PpsxViewLoop_vtbl;
typedef struct ViewDrawContext ViewDrawContext, *PViewDrawContext;
typedef struct DrawLayer DrawLayer, *PDrawLayer;
typedef struct LayerHeader LayerHeader, *PLayerHeader;

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

struct DrawLayer {
    ushort priority; /* ViewDrawContext.head list is sorted by it */

    ushort otSize; /* OT entries for this layer (its slice of the view's OT) */

    struct DrawLayer *next;
    void *ot; /* this layer's OT slice; primitives are added here */

    struct LayerHeader *header; /* draw area and offset packet sent before the layer's primitives */

};

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

struct LayerHeader { /* 16-byte packet at the head of each layer, allocated by ViewDrawContext_Begin; DrawLayer_SetArea fills it */

    uint tag; /* packet link and length */

    uint drawAreaTopLeft; /* GP0(E3) */

    uint drawAreaBottomRight; /* GP0(E4) */

    uint drawOffset; /* GP0(E5) */

};

typedef struct psxViewLoopDoubleBuffer psxViewLoopDoubleBuffer, *PpsxViewLoopDoubleBuffer;

struct psxViewLoopDoubleBuffer {
    struct psxViewLoop base;
    short bufferY[2]; /* VRAM Y of each buffer */

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

typedef struct RECT RECT, *PRECT;

struct RECT {
    short x;
    short y;
    short w;
    short h;
};

typedef struct Rect16 Rect16, *PRect16;

struct Rect16 { /* PsyQ RECT: x, y, w, h */

    short x;
    short y;
    short w;
    short h;
};

typedef struct ScreenViewLoop ScreenViewLoop, *PScreenViewLoop;
typedef struct ScreenViewLoop_vtbl ScreenViewLoop_vtbl, *PScreenViewLoop_vtbl;

struct ScreenViewLoop {
    struct ScreenViewLoop_vtbl *vptr;
};

struct ScreenViewLoop_vtbl {
    int offset; /* always 0 */

    void * (*tf)(void *); /* __tf type_info function */

    void (*dtor)(void *, int);
    void (*Run)(void *);
    void (*Enter)(void *);
    int (*Step)(void *);
    void (*Flip)(void *);
    void (*Leave)(void *);
};

typedef struct SeqChannel SeqChannel, *PSeqChannel;
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

    uchar loopDepth; /* loop stack depth (events 0x01/0x20-0x3f push, 0x02 pops) */

    uchar pad13;
    uchar unk14[4];
    uchar *loopStart[3]; /* loop start pointers */

    uchar unk24;
    uchar loopCount[3]; /* remaining loop counts (0xff = forever) */

};

struct SeqFade {
    short count;
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
    void *instBank;
    struct SeqFade fade; /* volume fade (SeqFade_Step each tick); count 0 = idle */

    struct SeqTrack tracks[16];
};

struct SeqChannel {
    struct SeqPlayer *player;
    struct SeqTrack *track;
    uchar *program;
    void *sample;
    int releaseTime;
    ushort lastVol;
    short lastBend;
    uchar note;
    uchar velocity;
    uchar lastPan;
    char spuVoice;
};

typedef struct SfxLoop SfxLoop, *PSfxLoop;

struct SfxLoop {
    void *sample;
    uchar voice;
    uchar pad05[3];
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

#endif
