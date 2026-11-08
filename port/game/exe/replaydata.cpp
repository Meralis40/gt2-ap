// Resident code 80069890-80069ff8 (replaydata). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#include "gptr.h"
#include "common.h"

extern "C" {
extern unsigned char (&D_80091C8C);
extern unsigned short (&g_VolBootIds)[248];
extern char (&g_GameState)[];

void *Buf_Append(void *dst, void *src, int size)
{
    memcpy(dst, src, size);
    return (void *)rt::from_guest(((int)rt::to_guest(dst) + (size + 3U & 0xfffffffc)));
}

void *Buf_Read(void *src, void *dst, unsigned int size)
{
    memcpy(dst, src, size);
    return (void *)rt::from_guest(((int)rt::to_guest(src) + (size + 3 & 0xfffffffc)));
}

void *Buf_AppendByte(unsigned char *dst, unsigned char *src)
{
    *dst = *src;
    return dst + 4;
}

signed char *Buf_ReadCharAsShort(signed char *src, short *dst)
{
    *dst = (short)*src;
    return src + 4;
}

// A block of replay data: a 0x19-byte header whose short at 0x10 is the size of the rest.
static inline char *Buf_AppendBlock(char *dst, char *block)
{
    return (char *)Buf_Append(dst, block, *(unsigned short *)(block + 0x10) + 0x19);
}

static inline char *Buf_ReadBlock(char *src, char *block)
{
    return (char *)Buf_Read(src, block, *(unsigned short *)(src + 0x10) + 0x19);
}

// The parts of g_GameState a replay saves, each from its own g_GameState and offset local
// (the offsets are too big for an addiu, so the game adds them as registers).
static inline RaceSetup *Replay_GetRaceSetup()
{
    char *base = g_GameState;
    int off = 0xbf7c;

    return (RaceSetup *)(base + off); // g_RaceSetup
}

// The record of car 0 or 1: a RaceRecord (0xfc bytes), then the replay data.
static inline char *Replay_GetRecord(int car)
{
    char *base = g_GameState;
    int off = 0xc5a8;

    return base + (off + car * 0x4518);
}

// g_RaceCarParams[car].
static inline char *Replay_GetCarParams(int car)
{
    char *base = g_GameState;
    int off = 0x14fda;

    return base + (off + car * 0x1c0);
}

// Packs the race into buf: g_RaceSetup, then each car's params and record. Race type 6
// has one record holding a count of laps, each 0xe0 bytes and a block. Returns the size.
int ReplayData_BuildRace(char *buf)
{
    RaceSetup *race = Replay_GetRaceSetup();
    char *p = (char *)Buf_Append(buf, race, sizeof(RaceSetup));
    int type = race->raceType;

    if (type != 6) {
        int n = 1;
        int i;

        if (type == 0) {
            n = 2;
        }
        for (i = 0; i < n; i++) {
            char *rec;

            if (type != 0) {
                p = (char *)Buf_Append(p, Replay_GetCarParams(i), 0x1c0);
            }
            rec = Replay_GetRecord(i);
            p = (char *)Buf_Append(p, rec, 0xfc);
            p = Buf_AppendBlock(p, rec + 0xfc);
        }
    } else {
        char *rec = Replay_GetRecord(0);
        int count;
        int j;

        p = (char *)Buf_Append(p, rec, 0xfc);
        p = (char *)Buf_AppendByte((unsigned char *)p, (unsigned char *)rec + 0xfc);
        count = *(short *)(rec + 0xfc);
        for (j = 0; j < count; j++) {
            char *e = rec + (0x100 + j * 0x10fc);

            p = (char *)Buf_Append(p, e, 0xe0);
            p = Buf_AppendBlock(p, e + 0xe0);
        }
    }
    return p - buf;
}

// Replay_Unpack's second car is car 1.
static inline int Replay_CarIndex(int i, int second)
{
    if (second) {
        return i + 1;
    }
    return i;
}

// Unpacks ReplayData_BuildRace's data; second puts the cars one later. GCC hoists
// Replay_GetCarParams's 0x14fda out of the loop, where the game loads it on each pass:
// its loop is bigger than ReplayData_BuildRace's, where the game does hoist it, and every
// form of the offset that GCC doesn't hoist here it doesn't hoist there either.
void Replay_Unpack(char *src, int second)
{
    RaceSetup *race = Replay_GetRaceSetup();
    int type;

    src = (char *)Buf_Read(src, race, sizeof(RaceSetup));
    type = race->raceType;
    if (type != 6) {
        int n = 1;
        int i;

        if (type == 0) {
            n = 2;
        }
        for (i = 0; i < n; i++) {
            char *rec;

            if (type != 0) {
                src = (char *)Buf_Read(src, Replay_GetCarParams(i + second), 0x1c0);
            }
            rec = Replay_GetRecord(Replay_CarIndex(i, second));
            RaceRecord_Init((RaceRecord *)rec);
            src = (char *)Buf_Read(src, rec, 0xfc);
            src = Buf_ReadBlock(src, Replay_GetRecord(Replay_CarIndex(i, second)) + 0xfc);
        }
    } else {
        char *rec = Replay_GetRecord(0);
        int count;
        int j;

        RaceRecord_Init((RaceRecord *)rec);
        src = (char *)Buf_Read(src, rec, 0xfc);
        src = (char *)Buf_ReadCharAsShort((signed char *)src, (short *)(rec + 0xfc));
        count = *(short *)(rec + 0xfc);
        for (j = 0; j < count; j++) {
            char *e = rec + (0x100 + j * 0x10fc);

            src = (char *)Buf_Read(src, e, 0xe0);
            src = Buf_ReadBlock(src, e + 0xe0);
        }
    }
}

// Packs the ghost, car 1: its RaceCar, params and first lap. Returns the size.
int ReplayData_BuildGhost(char *buf)
{
    char *rec = Replay_GetRecord(1);
    char *p = (char *)Buf_Append(buf, &Replay_GetRaceSetup()->cars[1], 0xd0);

    p = (char *)Buf_Append(p, Replay_GetCarParams(1), 0x1c0);
    rec += 0x100; // walked as a cursor: with rec + 0x1e0, GCC swaps rec's and base's registers
    p = (char *)Buf_Append(p, rec, 0xe0);
    rec += 0xe0;
    p = Buf_AppendBlock(p, rec);
    return p - buf;
}

void GhostReplay_Unpack(char *src)
{
    char *rec = Replay_GetRecord(1);

    src = (char *)Buf_Read(src, &Replay_GetRaceSetup()->cars[1], 0xd0);
    src = (char *)Buf_Read(src, Replay_GetCarParams(1), 0x1c0);
    rec += 0x100;
    src = (char *)Buf_Read(src, rec, 0xe0);
    rec += 0xe0;
    Buf_ReadBlock(src, rec);
}

// A ghost file: a RaceSetup whose car 0 becomes the ghost car, and at 0x62c its record.
void GhostEvent_ApplyFile(char *file)
{
    RaceCar *car = &Replay_GetRaceSetup()->cars[1];

    *car = ((RaceSetup *)file)->cars[0];
    car->driverType = 2;
    file += 0x62c;
    *(ReplayRecord *)Replay_GetRecord(1) = *(ReplayRecord *)file;
}

int License_DemoFileId(int license, int test)
{
    return (unsigned int)g_VolBootIds[0xe1] + (signed char)(&D_80091C8C)[license] * 10 + test;
}

void Replay_RestoreCarParams()
{
    *(CarParams *)Replay_GetCarParams(0) = *(CarParams *)(Replay_GetRecord(1) + 0xfc);
    Replay_GetRaceSetup()->game = 1;
}
}
