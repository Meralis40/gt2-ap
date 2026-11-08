// OVL0 80017e18-80018a68 (carsound): Wheels and car sounds.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "gptr.h"
#include "common.h"
#include "gamestate.h"

// The car sizes RaceVehicle_Init hands RaceVehicle_SetupModelDims.
struct CarDims {
    char unk0[8];
    short unk8;
    short unkA;
    char unkC[8];
    short hubOffset[2];
    char unk18[0x50];
    unsigned char rim[2];
};

// The body's sizes in a car model.
struct CarBodyDims {
    char unk0[0x3c];
    short front;
    char unk3e[2];
    short y;
    char unk42[2];
    short length;
    char unk46[2];
    short rear;
    char unk4a[2];
    short shift;
};

// The race memory's sound banks.
struct RaceBanks {
    char unk0[0xd48a0];
    SpuBank engine[4];
};

// g_RaceSetup, as the game reaches it: g_GameState plus 0xbf7c, added at run time.
static inline RaceSetup *RaceSetup_Get(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;

    return (RaceSetup *)(base + offset);
}

static inline RaceCar *RaceSetup_Car(RaceSetup *setup, int i)
{
    int offset = i * (int)sizeof(RaceCar) + 0x5c;

    return (RaceCar *)rt::from_guest((offset + (int)rt::to_guest(setup)));
}

// Soft limit of a voice's volume.
static inline int Vol_Clamp(int vol)
{
    if (vol > 0x3fff) {
        vol = 0x3fff;
    }
    return vol;
}

#define WHEEL_WIDTH(v, i) (((short *)((char *)(v) + 0x418))[i])
#define WHEEL_RIM(v, i) (((short *)((char *)(v) + 0x41c))[i])

extern "C" {
extern SpuBank (&D_8017D894);
extern unsigned char (&D_801C9994);
extern unsigned short (&D_801E306E);
extern unsigned char (&g_CarSfxIds)[];
extern unsigned char (&g_RaceSfxStereoPairs)[][2];
extern short (&g_WheelOffsetTable)[];
extern int (&g_SpuAllocPtr);
void CarSound_PlayOneShot(CarSound *cs, int index, int pan, int volume);

// How far the wheel's half width pokes out of the tyre of rim size rim.
int Tyre_GetHubOffset(int rim, int width)
{
    int hub = (rim * 0xa000 + 0x5000) / 1000;

    if (width < 0) {
        width = -width;
    }
    return ((width - (hub >> 1)) * 1000 >> 12) * 2;
}

// Takes the car's size from its model: the length, the tyre offsets, and moves
// the body up by its height.
void RaceVehicle_SetupModelDims(RaceVehicle *v, CarDims *dims)
{
    if (v->model != 0) {
        CarBodyDims *body = *(game_pointer<CarBodyDims> *)((char *)v->model + 0x870);
        int y = body->y;
        int rear = body->rear;
        int front = body->front;
        int shift = (unsigned short)body->shift - 0x10;
        int length = body->length;

        if (front < 0) {
            front = -front;
        }
        if (length < front) {
            length = front;
        }
        if (shift >= 0) {
            y <<= shift;
            rear <<= shift;
            length <<= shift;
        } else {
            shift = -shift;
            y >>= shift;
            rear >>= shift;
            length >>= shift;
        }
        v->bodyLength = length << 4;
        dims->unk8 = (unsigned int)(-y * 125) >> 9;
        dims->unkA = (unsigned int)(rear * 125) >> 9;
        {
            CarModelHeader *model = v->model;
            VECTOR up;

            dims->hubOffset[0] = Tyre_GetHubOffset(dims->rim[0], model->wheelPos[0].unk0);
            dims->hubOffset[1] = Tyre_GetHubOffset(dims->rim[1], model->wheelPos[2].unk0);
            up.vx = 0;
            up.vy = 0;
            up.vz = y << 4;
            Matrix_Translate(&v->world, &up);
        }
    }
}

// Sets the wheels' positions and builds the model's wheels from the physics'
// tyre sizes.
void RaceVehicle_SetupWheels(RaceVehicle *v)
{
    WheelDims front;
    WheelDims rear;
    int frontWidth;
    int rearWidth;
    int frontRadius;
    int rearRadius;
    int frontScale;
    int rearScale;
    int x;
    int i;

    CarModel_GetWheelDims(v->model, &front, &rear);
    rearWidth = 0;
    frontWidth = rearWidth;
    rearRadius = rearWidth;
    frontRadius = rearWidth;
    rearScale = rearWidth;
    frontScale = rearWidth;
    for (i = rearWidth; i < 4; i++) {
        int z;

        if ((i & 2) == 0) {
            frontRadius = v->phys.tyreRadius[0];
            frontScale = 0x1000 - (WHEEL_RIM(v, 0) << 12) / frontRadius;
            x = v->phys.halfTrackF;
            frontWidth = WHEEL_WIDTH(v, 0);
        } else {
            rearRadius = v->phys.tyreRadius[1];
            rearScale = 0x1000 - (WHEEL_RIM(v, 1) << 12) / rearRadius;
            x = v->phys.halfTrackR;
            rearWidth = WHEEL_WIDTH(v, 1);
        }
        if ((i & 1) == 0) {
            x = -x;
        }
        z = v->model->wheelPos[i].z;
        v->wheelDraw[i].rest[0] = x;
        v->wheelDraw[i].rest[1] = 0;
        v->wheelDraw[i].rest[2] = z;
    }
    {
        int index = RaceSetup_Get()->cars[v->index].setup.wheel >> 8 & 3;
        int wheel = g_WheelOffsetTable[index];

        front.radius = frontRadius;
        front.width = frontWidth;
        front.scale = frontScale;
        rear.radius = rearRadius;
        rear.width = rearWidth;
        rear.scale = rearScale;
        front.offset = wheel;
        rear.offset = wheel;
        CarModel_BuildWheels((WheelGeom *)&v->model->wheelTexture, &front, &rear);
    }
}

// Turns reverb on for both layer voices of es.
void EngineSound_EnableReverb(EngineSound *es)
{
    SfxVoice_SetReverb(&es->voices[0].voice, 1);
    SfxVoice_SetReverb(&es->voices[1].voice, 1);
}

// Mode 0 plays both engine banks (a player's car), mode 1 only one.
void CarSound_Init(CarSound *cs, SpuBank *banks, unsigned char mode)
{
    SpuBank *bank;
    unsigned char *ids;

    cs->mode = mode;
    cs->volume = 0;
    cs->unk95 = 0;
    cs->unk94 = 0;
    switch (cs->mode) {
    case 0:
        EngineSound_Init(&cs->engine, (EngnFile *)banks[0].ram, 0, 0);
        EngineSound_Init(&cs->engine2, (EngnFile *)banks[1].ram, 0, 0);
        EngineSound_EnableReverb(&cs->engine);
        EngineSound_EnableReverb(&cs->engine2);
        break;
    case 1:
        EngineSound_Init(&cs->engine2, (EngnFile *)banks[0].ram, 0, 0);
        break;
    }
    bank = &D_8017D894;
    ids = g_CarSfxIds;
    SfxLoop_Init(&cs->loops[0], SoundBank_GetSample(bank, ids[0]));
    SfxLoop_Init(&cs->loops[1], SoundBank_GetSample(bank, ids[1]));
    SfxLoop_Init(&cs->loops[2], SoundBank_GetSample(bank, ids[2]));
    SfxLoop_Init(&cs->loops[3], SoundBank_GetSample(bank, ids[3]));
}

// Mixes the car's engine and loops down to the SPU for this frame.
void CarSound_Update(CarSound *cs)
{
    short vol[2];
    short *p = vol;
    int pan = cs->pan;
    int volume = cs->volume;
    int doppler = cs->doppler;
    int left;
    int right;
    int priority;

    pan = (pan + 0x1000) >> 1;
    pan = pan * 127 >> 12;
    cs->engine.pitchScale = doppler;
    cs->engine2.pitchScale = doppler;
    {
        unsigned short rpm = cs->rpm;

        cs->engine.rpm = rpm;
        cs->engine2.rpm = rpm;
    }
    Spu_PanVolume(p, volume, pan);
    left = p[0];
    right = p[1];
    {
        int master = D_801C9994;

        left = master * left / 255;
        right = master * right / 255;
    }
    left = Vol_Clamp(left);
    right = Vol_Clamp(right);
    cs->engine.volL = left * cs->volEngine >> 14;
    cs->engine.volR = right * cs->volEngine >> 14;
    cs->engine2.volL = left * cs->volEngine2 >> 14;
    cs->engine2.volR = right * cs->volEngine2 >> 14;
    if (cs->mode == 1) {
        cs->engine2.volL = (long long)cs->engine2.volL * 0xccc >> 12;
        cs->engine2.volR = (long long)cs->engine2.volR * 0xccc >> 12;
    }
    priority = cs->carIndex * 6;
    EngineSound_SetPriority(&cs->engine, priority);
    EngineSound_SetPriority(&cs->engine2, priority + 1);
    SfxLoop_Update(&cs->loops[0], left * cs->volLoop[0] >> 14, right * cs->volLoop[0] >> 14, priority + 2,
                   doppler * cs->pitchLoop0 >> 12);
    SfxLoop_Update(&cs->loops[1], left * cs->volLoop[1] >> 14, right * cs->volLoop[1] >> 14, priority + 4, doppler);
    SfxLoop_Update(&cs->loops[2], left * cs->volLoop[2] >> 14, right * cs->volLoop[2] >> 14, priority + 3,
                   doppler * cs->pitchLoop2 >> 12);
    SfxLoop_Update(&cs->loops[3], left * cs->volLoop[3] >> 14, right * cs->volLoop[3] >> 14, priority + 3,
                   doppler * cs->pitchLoop3 >> 12);
    if (cs->mode == 0 && cs->volOneShot != 0) {
        CarSound_PlayOneShot(cs, 4, pan, cs->volOneShot * volume >> 14);
    }
    switch (cs->mode) {
    case 0:
        EngineSound_Update(&cs->engine);
    case 1:
        EngineSound_Update(&cs->engine2);
    }
}

void CarSound_Stop(CarSound *cs)
{
    switch (cs->mode) {
    case 0:
        EngineSound_Stop(&cs->engine);
    case 1:
        EngineSound_Stop(&cs->engine2);
    }
    SfxLoop_Stop(&cs->loops[0]);
    SfxLoop_Stop(&cs->loops[1]);
    SfxLoop_Stop(&cs->loops[2]);
    SfxLoop_Stop(&cs->loops[3]);
}

// Plays g_CarSfxIds[index] on the car's voices.
void CarSound_PlayOneShot(CarSound *cs, int index, int pan, int volume)
{
    int voice = cs->carIndex * 6;

    SoundBank_Play(&D_8017D894, g_CarSfxIds[index], pan, volume, voice);
}

// Once per race context (flag +0x11d702): loads the race sound bank (boot file bootId,
// 0xed from Race_Entry) into the SpuBank at +0xd4394 with its header at +0xd43a0
// (0x500 bytes).
void Race_LoadSfxBank(void *race, int bootId)
{
    if (*(char *)rt::from_guest(((int)rt::to_guest(race) + 0x11d702)) == '\0') {
        *(unsigned char *)rt::from_guest(((int)rt::to_guest(race) + 0x11d702)) = 1;
        SpuBank_Init((SpuBank *)rt::from_guest(((int)rt::to_guest(race) + 0xd4394)), (SeqBank *)rt::from_guest(((int)rt::to_guest(race) + 0xd43a0)), 0x500);
        SpuBank_Load((SpuBank *)rt::from_guest(((int)rt::to_guest(race) + 0xd4394)), bootId, 0, 1);
    }
}

// Loads each player's two engine files into the race's engine banks; with one
// player, also the two files 0xc0 and 0xc1.
void Race_LoadEngineBanks(void *race)
{
    RaceBanks *banks = (RaceBanks *)race;
    int slot = 0;
    RaceSetup *setup;
    int saved;
    int players;
    int firstFile;
    int i;

    SpuBankPool_Set((char *)race + 0xd48d0, 0x400);
    saved = g_SpuAllocPtr;
    players = 1;
    if (RaceSetup_Get()->raceType == 0) {
        players = 2;
    }
    firstFile = D_801E306E;
    i = 0;
    setup = RaceSetup_Get();
    for (; i < players; i++) {
        RaceCar *car = RaceSetup_Car(setup, i);
        int file = firstFile + EngineSound_FindIndex(car->setup.engineSound) * 9;
        int file2 = file + car->setup.engineSoundVariant + 1;

        SpuBank_LoadFromPool(&banks->engine[slot++], file, 1, 0);
        SpuBank_LoadFromPool(&banks->engine[slot++], file2, 1, 0);
    }
    if (players < 2) {
        SpuBank_LoadFromPool(&banks->engine[slot++], 0xc0, 1, 1);
        SpuBank_LoadFromPool(&banks->engine[slot], 0xc1, 1, 1);
    }
    SpuAlloc_SetPtr(saved);
}

// Plays a g_RaceSfxStereoPairs entry: in the middle when both sides are the same.
void RaceSfx_PlayStereo(int sfx)
{
    unsigned char (*t)[2] = g_RaceSfxStereoPairs;
    unsigned char *e = t[sfx];
    int left = e[0];
    int right = e[1];
    SpuBank *bank = &D_8017D894;

    if (left == right) {
        SoundBank_Play(bank, right, 0x40, 0x4000, -1);
    } else {
        SoundBank_Play(bank, left, 0, 0x4000, -1);
        SoundBank_Play(bank, right, 0x7f, 0x4000, -1);
    }
}
}
