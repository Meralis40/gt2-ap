// Resident code 8005e7b0-80060924 (garage). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
int CarParams_Build(CarSpec *spec, void *params);
int CarParams_GetPerformance(CarParams *params, EnginePerf *out);
void CarSpec_CopySetupFields(CarSpec *spec, CarWork *work);
void Gearbox_ComputeLimits(int gearCount, short *ratios, short *minRatios, short *maxRatios);
void SoundBank_Play(void *bank, int index, int volume, int pitch, int voice);
extern char g_CommonSoundBank[];
extern unsigned char D_8009160C[][2]; // the two samples of each menu sound
void CarSpec_FromCarWork(CarWork *work, CarSpec *spec);
void Gearbox_ComputeRatios(CarParams *params, int topSpeedKmh, short *ratios);
extern char D_80091620[];
void CarSetup_ApplyPartEntry(int table, void *entry, CarSetup *setup);
void CarSpec_ApplyPartEntry(int table, void *entry, CarSpec *spec);
void CarWork_ComputeGearRatios(CarWork *work, short *ratios);
int CarWork_TransIsCustom(CarWork *work, int stage);
int CarWork_HasFullSuspension(CarWork *work);
int CarWork_GetGearCount(CarWork *work);
void CarWork_GetGearRange(CarWork *work, short *ratios, short *minRatios, short *maxRatios);
extern int D_800915A0[];     // CarSetup offset of each LSD setting
extern int D_800915B8[][3];  // LSD entry offsets of each LSD setting's minimum ([1]) and maximum ([2])
extern short D_80091600[];   // id of each LSD setting

// One car setting: its value and range (CarWork_GetSettingRange); id is set for the
// LSD settings only.
struct SettingRange {
    short value;
    short min;
    short max;
    short id;
};

// Recomputes a full-custom transmission's ratios and keeps what they were computed
// with.
static inline void CarWork_SetCustomRatios(CarWork *work)
{
    short ratios[8];
    int i;

    CarWork_ComputeGearRatios(work, ratios);
    for (i = 0; i < 8; i++) {
        work->setup.t11_values[i] = ratios[i];
    }
    work->setup.t11_4e_copy = work->setup.t11_4e;
    work->setup.t11_4c_copy = work->setup.t11_4c;
    work->setup.work_17b6 = work->partSlots[3];
    work->setup.work_17b8 = work->partSlots[4];
}
int CarWork_GetSettingRange(CarWork *work, int setting, SettingRange *out);

void Garage_AddCredits(Garage *garage, int amount)
{
    int iVar1;

    iVar1 = garage->credits + amount;
    garage->credits = iVar1;
    if (99999999 < iVar1) {
        garage->credits = 99999999;
    }
    if (garage->credits < 0) {
        garage->credits = 0;
    }
}

// Adds car to the garage; returns 0 if it is full (100 cars).
int Garage_AddCar(Garage *garage, Car *car)
{
    if (garage->carCount >= 100) return 0;
    garage->cars[garage->carCount] = *car;
    garage->carCount++;
    return 1;
}

int Car_HasPart(Car *car, int part)
{
    int bit = 1 << (part % 8);

    return (car->partsOwned[part >> 3] & bit) != 0;
}

// Marks part installed and adds its cost to the car's value.
void Car_InstallPart(Car *car, int part, int cost)
{
    int mask = 1 << (part % 8);

    car->partsOwned[part >> 3] |= mask;
    car->value += cost;
}

void PartsOwned_Set(int part, unsigned char *bits)
{
    int mask = 1 << (part % 8);

    bits[part >> 3] |= mask;
}

void CarWork_ComputeGearRatios(CarWork *work, short *ratios)
{
    CarSpec CStack_298;
    CarParams CStack_1d0;

    CarSpec_FromCarWork(work, &CStack_298);
    CarParams_Build(&CStack_298, &CStack_1d0);
    CStack_1d0.unk1aa = '\0';
    Gearbox_ComputeRatios(&CStack_1d0, (unsigned int)CStack_1d0.gearTopSpeedDiv10 * 10, ratios);
}

// The gear ratios of the car's transmission and the range each can be set in, with
// the tyres of the car's setup.
void CarWork_GetGearRange(CarWork *work, short *ratios, short *minRatios, short *maxRatios)
{
    CarSpec spec;
    CarParams params;
    int front = (ushort)work->setup.work_17b6;
    int rear = (ushort)work->setup.work_17b8;

    CarSpec_FromCarWork(work, &spec);
    spec.frontTyre18 = work->frontTyre18[front];
    spec.frontTyre19 = work->frontTyre19[front];
    spec.frontTyre1a = work->frontTyre1a[front];
    spec.rearTyre18 = work->rearTyre18[rear];
    spec.rearTyre19 = work->rearTyre19[rear];
    spec.t11_4c = work->setup.t11_4c_copy;
    spec.t11_4e = work->setup.t11_4e_copy;
    CarParams_Build(&spec, &params);
    params.unk1aa = 0;
    Gearbox_ComputeRatios(&params, params.gearTopSpeedDiv10 * 10, ratios);
    Gearbox_ComputeLimits(params.gearCount, ratios, minRatios, maxRatios);
}

int CarWork_TransIsCustom(CarWork *work, int stage)
{
    return (unsigned int)(work->transEntries[stage][0x20] != '\0');
}

// Fits stage of slot: sets the setup's part index and applies the part's entry.
void CarWork_SetPartSlot(CarWork *work, int slot, int stage)
{
    *(work->partSlots + slot) = stage;
    switch (slot) {
    case 0:
        work->setup.idx_t12 = work->suspIdx[stage];
        CarSetup_ApplyPartEntry(0x12, work->suspEntries[stage], &work->setup);
        break;
    case 1:
        work->setup.idx_t00 = work->brakeIdx[stage];
        break;
    case 2:
        work->setup.idx_t01 = work->brakeBalIdx[stage];
        CarSetup_ApplyPartEntry(1, work->brakeBalEntries[stage], &work->setup);
        break;
    case 3:
        work->setup.idx_t16_front = work->frontTyreIdx[stage];
        break;
    case 4:
        work->setup.idx_t16_rear = work->rearTyreIdx[stage];
        break;
    case 5:
        work->setup.idx_t1d = work->wheelIdx[stage];
        break;
    case 6:
        work->setup.idx_t11 = work->transIdx[stage];
        CarSetup_ApplyPartEntry(0x11, work->transEntries[stage], &work->setup);
        if (CarWork_TransIsCustom(work, stage)) {
            CarWork_SetCustomRatios(work);
        }
        break;
    case 7:
        work->setup.idx_t0f = work->clutchIdx[stage];
        break;
    case 8:
        work->setup.idx_t0e = work->flywheelIdx[stage];
        break;
    case 9:
        work->setup.idx_t10 = work->driveshaftIdx[stage];
        break;
    case 10:
        work->setup.idx_t0a = work->chipIdx[stage];
        break;
    case 0xb:
        work->setup.idx_t07 = work->portPolishIdx[stage];
        break;
    case 0xc:
        work->setup.idx_t08 = work->balanceIdx[stage];
        break;
    case 0xd:
        // An NA tune takes the turbo and intercooler off.
        work->setup.idx_t0c = work->turboIdx[0];
        CarSetup_ApplyPartEntry(0xc, work->turboEntries[0], &work->setup);
        work->partSlots[0xf] = 0;
        work->partSlots[0x10] = 0;
        work->setup.idx_t13 = work->intercoolerIdx[0];
        work->setup.idx_t0b = work->naTuneIdx[stage];
        break;
    case 0xe:
        work->setup.idx_t09 = work->displIdx[stage];
        break;
    case 0xf:
        // A turbo takes the NA tune off.
        work->partSlots[0xd] = 0;
        work->setup.idx_t0b = work->naTuneIdx[0];
        work->setup.idx_t0c = work->turboIdx[stage];
        CarSetup_ApplyPartEntry(0xc, work->turboEntries[stage], &work->setup);
        break;
    case 0x10:
        work->setup.idx_t13 = work->intercoolerIdx[stage];
        break;
    case 0x11:
        work->setup.idx_t14 = work->mufflerIdx[stage];
        break;
    case 0x12:
        work->setup.idx_t04 = work->weightIdx[stage];
        break;
    case 0x13:
        work->setup.idx_t05 = work->racingModIdx[stage];
        CarSetup_ApplyPartEntry(5, work->racingModEntries[stage], &work->setup);
        break;
    case 0x14:
        work->setup.idx_t1b = work->ascIdx[stage];
        CarSetup_ApplyPartEntry(0x1b, work->ascEntries[stage], &work->setup);
        break;
    case 0x15:
        work->setup.idx_t1c = work->tcsIdx[stage];
        CarSetup_ApplyPartEntry(0x1c, work->tcsEntries[stage], &work->setup);
        break;
    case 0x16:
        work->setup.idx_t15 = work->lsdIdx[stage];
        CarSetup_ApplyPartEntry(0x15, work->lsdEntries[stage], &work->setup);
        break;
    }
}

// The entry index of stage of slot (-1 = not available).
int CarWork_GetPartIdx(CarWork *work, int slot, int stage)
{
    int idx;

    switch (slot) {
    case 0: idx = work->suspIdx[stage]; break;
    case 1: idx = work->brakeIdx[stage]; break;
    case 2: idx = work->brakeBalIdx[stage]; break;
    case 3: idx = work->frontTyreIdx[stage]; break;
    case 4: idx = work->rearTyreIdx[stage]; break;
    case 5: idx = work->wheelIdx[stage]; break;
    case 6: idx = work->transIdx[stage]; break;
    case 7: idx = work->clutchIdx[stage]; break;
    case 8: idx = work->flywheelIdx[stage]; break;
    case 9: idx = work->driveshaftIdx[stage]; break;
    case 10: idx = work->chipIdx[stage]; break;
    case 0xb: idx = work->portPolishIdx[stage]; break;
    case 0xc: idx = work->balanceIdx[stage]; break;
    case 0xd: idx = work->naTuneIdx[stage]; break;
    case 0xe: idx = work->displIdx[stage]; break;
    case 0xf: idx = work->turboIdx[stage]; break;
    case 0x10: idx = work->intercoolerIdx[stage]; break;
    case 0x11: idx = work->mufflerIdx[stage]; break;
    case 0x12: idx = work->weightIdx[stage]; break;
    case 0x13: idx = work->racingModIdx[stage]; break;
    case 0x14: idx = work->ascIdx[stage]; break;
    case 0x15: idx = work->tcsIdx[stage]; break;
    case 0x16: idx = work->lsdIdx[stage]; break;
    }
    return idx;
}

// CarSpec_FromCarWork with stage fitted in slot instead (-1 = none).
void CarSpec_FromCarWorkWithPart(CarWork *work, CarSpec *spec, int slot, int stage)
{
    int i;
    short ratios[8];

    CarSpec_FromCarWork(work, spec);
    switch (slot) {
    case -1:
        break;
    case 0:
        spec->suspension = work->suspEntries[stage];
        CarSpec_ApplyPartEntry(0x12, spec->suspension, spec);
        break;
    case 1:
        spec->brakes = work->brakeEntries[stage];
        CarSpec_ApplyPartEntry(0, spec->brakes, spec);
        break;
    case 2:
        spec->brakeBalance = work->brakeBalEntries[stage];
        CarSpec_ApplyPartEntry(1, spec->brakeBalance, spec);
        break;
    case 3:
        spec->frontTyre18 = work->frontTyre18[stage];
        spec->frontTyre19 = work->frontTyre19[stage];
        spec->frontTyre1a = work->frontTyre1a[stage];
        break;
    case 4:
        spec->rearTyre18 = work->rearTyre18[stage];
        spec->rearTyre19 = work->rearTyre19[stage];
        break;
    case 5:
        break;
    case 6:
        spec->transmission = work->transEntries[stage];
        CarSpec_ApplyPartEntry(0x11, spec->transmission, spec);
        if (CarWork_TransIsCustom(work, stage)) {
            CarWork_ComputeGearRatios(work, ratios);
            for (i = 0; i < 8; i++) {
                spec->t11_values[i] = ratios[i];
            }
        }
        break;
    case 7:
        spec->clutch = work->clutchEntries[stage];
        CarSpec_ApplyPartEntry(0xf, spec->clutch, spec);
        break;
    case 8:
        spec->flywheel = work->flywheelEntries[stage];
        CarSpec_ApplyPartEntry(0xe, spec->flywheel, spec);
        break;
    case 9:
        spec->driveshaft = work->driveshaftEntries[stage];
        CarSpec_ApplyPartEntry(0x10, spec->driveshaft, spec);
        break;
    case 10:
        spec->chip = work->chipEntries[stage];
        CarSpec_ApplyPartEntry(0xa, spec->chip, spec);
        break;
    case 0xb:
        spec->portPolish = work->portPolishEntries[stage];
        CarSpec_ApplyPartEntry(7, spec->portPolish, spec);
        break;
    case 0xc:
        spec->balancing = work->balanceEntries[stage];
        CarSpec_ApplyPartEntry(8, spec->balancing, spec);
        break;
    case 0xd:
        spec->naTune = work->naTuneEntries[stage];
        CarSpec_ApplyPartEntry(0xb, spec->naTune, spec);
        spec->turbo = work->turboEntries[0];
        CarSpec_ApplyPartEntry(0xc, spec->turbo, spec);
        spec->intercooler = work->intercoolerEntries[0];
        CarSpec_ApplyPartEntry(0x13, spec->intercooler, spec);
        break;
    case 0xe:
        spec->displacement = work->displEntries[stage];
        CarSpec_ApplyPartEntry(9, spec->displacement, spec);
        break;
    case 0xf:
        spec->turbo = work->turboEntries[stage];
        CarSpec_ApplyPartEntry(0xc, spec->turbo, spec);
        spec->naTune = work->naTuneEntries[0];
        // The game applies the first LSD entry here, not the stock NA tune.
        CarSpec_ApplyPartEntry(0x15, work->lsdEntries[0], spec);
        break;
    case 0x10:
        spec->intercooler = work->intercoolerEntries[stage];
        CarSpec_ApplyPartEntry(0x13, spec->intercooler, spec);
        break;
    case 0x11:
        spec->muffler = work->mufflerEntries[stage];
        CarSpec_ApplyPartEntry(0x14, spec->muffler, spec);
        break;
    case 0x12:
        spec->weightReduction = work->weightEntries[stage];
        CarSpec_ApplyPartEntry(4, spec->weightReduction, spec);
        break;
    case 0x13:
        spec->racingMod = work->racingModEntries[stage];
        CarSpec_ApplyPartEntry(5, spec->racingMod, spec);
        break;
    case 0x14:
        spec->asc = work->ascEntries[stage];
        CarSpec_ApplyPartEntry(0x1b, spec->asc, spec);
        break;
    case 0x15:
        spec->tcs = work->tcsEntries[stage];
        CarSpec_ApplyPartEntry(0x1c, spec->tcs, spec);
        break;
    case 0x16:
        spec->lsd = work->lsdEntries[stage];
        CarSpec_ApplyPartEntry(0x15, spec->lsd, spec);
        break;
    }
}

// Points spec at the entries of the stages fitted in work's part slots.
void CarSpec_FromCarWork(CarWork *work, CarSpec *spec)
{
    int stage;

    CarSpec_CopySetupFields(spec, work);
    spec->suspension = work->suspEntries[work->partSlots[0]];
    spec->brakes = work->brakeEntries[work->partSlots[1]];
    spec->brakeBalance = work->brakeBalEntries[work->partSlots[2]];
    spec->transmission = work->transEntries[work->partSlots[6]];
    spec->clutch = work->clutchEntries[work->partSlots[7]];
    spec->flywheel = work->flywheelEntries[work->partSlots[8]];
    spec->driveshaft = work->driveshaftEntries[work->partSlots[9]];
    spec->chip = work->chipEntries[work->partSlots[10]];
    spec->portPolish = work->portPolishEntries[work->partSlots[0xb]];
    spec->balancing = work->balanceEntries[work->partSlots[0xc]];
    spec->naTune = work->naTuneEntries[work->partSlots[0xd]];
    spec->displacement = work->displEntries[work->partSlots[0xe]];
    spec->turbo = work->turboEntries[work->partSlots[0xf]];
    spec->intercooler = work->intercoolerEntries[work->partSlots[0x10]];
    spec->muffler = work->mufflerEntries[work->partSlots[0x11]];
    spec->weightReduction = work->weightEntries[work->partSlots[0x12]];
    spec->racingMod = work->racingModEntries[work->partSlots[0x13]];
    spec->asc = work->ascEntries[work->partSlots[0x14]];
    spec->tcs = work->tcsEntries[work->partSlots[0x15]];
    spec->lsd = work->lsdEntries[work->partSlots[0x16]];
    spec->t02 = &work->t02Entry[work->partSlots[0x17] * 24];
    spec->t03 = &work->t03Entry[work->partSlots[0x18] * 20];
    spec->t06 = &work->t06Entry[work->partSlots[0x19] * 76];
    spec->t0d = &work->t0dEntry[work->partSlots[0x1a] * 16];
    stage = work->partSlots[3];
    spec->frontTyre18 = work->frontTyre18[stage];
    spec->frontTyre19 = work->frontTyre19[stage];
    spec->frontTyre1a = work->frontTyre1a[stage];
    stage = work->partSlots[4];
    spec->rearTyre18 = work->rearTyre18[stage];
    spec->rearTyre19 = work->rearTyre19[stage];
}

void FUN_8005f788(void)
{
}

// Whether the car has an NA tune stage.
int CarWork_HasNaTune(CarWork *work)
{
    int found = 0;
    int i;

    for (i = 1; i < 4; i++) {
        if (work->naTuneIdx[i] >= 0) found = 1;
    }
    return found;
}

// Whether the car has a turbo kit stage.
int CarWork_HasTurboKits(CarWork *work)
{
    int found = 0;
    int i;

    for (i = 1; i < 5; i++) {
        if (work->turboIdx[i] >= 0) found = 1;
    }
    return found;
}

int CarWork_HasFullSuspension(CarWork *work)
{
    return (unsigned int)(work->partSlots[0] == 3);
}

int CarWork_HasBrakeBalance(CarWork *work)
{
    return (unsigned int)(0 < work->partSlots[2]);
}

int CarWork_HasFullTransmission(CarWork *work)
{
    return (unsigned int)(work->partSlots[6] == 3);
}

int CarWork_GetGearCount(CarWork *work)
{
    return (unsigned int)work->transEntries[work->partSlots[6]][9];
}

int CarWork_CountRacingModStages(CarWork *work)
{
    int n = 0;
    int i;

    for (i = 1; i < 5; i++) {
        if (work->racingModIdx[i] < 0) break;
        n++;
    }
    return n;
}

// Whether any of the LSD settings (10 to 12) can be changed.
int CarWork_HasLsdSettings(CarWork *work)
{
    SettingRange range[8];

    if (CarWork_GetSettingRange(work, 10, range) > 0) return 1;
    if (CarWork_GetSettingRange(work, 11, range) > 0) return 1;
    return CarWork_GetSettingRange(work, 12, range) > 0;
}

int CarWork_HasYawControl(CarWork *work)
{
    return (unsigned int)(work->partSlots[0x16] == 5);
}

int CarWork_HasAsc(CarWork *work)
{
    return (unsigned int)(work->partSlots[0x14] == 1);
}

int CarWork_HasTcs(CarWork *work)
{
    return (unsigned int)(work->partSlots[0x15] == 1);
}

int CarWork_SuspHasSetting0(CarWork *work)
{
    return (unsigned int)(work->suspEntries[work->partSlots[0]][0x4b] == '\0');
}

// The garage stats of the car: weight, power, torque and drivetrain, as
// Car_StoreSetup stores them. The parameters are built in the scratchpad.
void CarWork_ComputeStats(CarWork *work, unsigned short *out)
{
    CarSpec spec;
    EnginePerf *perf = (EnginePerf *)0x1f8001c0;

    CarSpec_FromCarWork(work, &spec);
    CarParams_Build(&spec, (CarParams *)0x1f800000);
    CarParams_GetPerformance((CarParams *)0x1f800000, (EnginePerf *)0x1f8001c0);
    out[1] = perf->maxPower;
    out[2] = perf->maxTorque;
    out[0] = ((CarParams *)0x1f800000)->weight;
    ((unsigned char *)out)[6] = ((CarParams *)0x1f800000)->drivetrain;
}

// Sets setting from count values (CarWork_GetSettingRange's records).
#ifdef NON_MATCHING
// Close, but the game tests CarWork_HasFullSuspension with xori and beqz, keeps the
// gear index's increment in another register and uses other saved registers.
// Its jump table is in resident_data_8008f990.
void CarWork_ApplySetting(CarWork *work, int setting, SettingRange *values, int count)
{
    int i, j;

    switch (setting) {
    case 0:
        work->setup.t12_60[0] = values[0].value;
        work->setup.t12_60[1] = values[1].value;
        break;
    case 1:
        work->setup.t12_5a[2] = values[0].value;
        work->setup.t12_5a[3] = values[1].value;
        break;
    case 2:
        if (!CarWork_HasFullSuspension(work)) {
            work->setup.t12_60[6] = values[0].value;
            work->setup.t12_60[10] = values[1].value;
            work->setup.t12_60[7] = values[0].value;
            work->setup.t12_60[11] = values[1].value;
        }
        work->setup.t12_60[4] = values[0].value;
        work->setup.t12_60[8] = values[1].value;
        work->setup.t12_60[5] = values[0].value;
        work->setup.t12_60[9] = values[1].value;
        break;
    case 3:
        work->setup.t12_60[6] = values[0].value;
        work->setup.t12_60[10] = values[1].value;
        work->setup.t12_60[7] = values[0].value;
        work->setup.t12_60[11] = values[1].value;
        break;
    case 4:
        work->setup.t12_5a[0] = values[0].value;
        work->setup.t12_5a[1] = values[1].value;
        break;
    case 5:
        work->setup.setting_5e[0] = values[0].value - 0x80;
        work->setup.setting_5e[1] = values[1].value - 0x80;
        break;
    case 6:
        work->setup.t12_60[12] = values[0].value;
        work->setup.t12_60[13] = values[1].value;
        break;
    case 7:
        work->setup.t01_50[0] = values[0].value;
        work->setup.t01_50[1] = values[1].value;
        break;
    case 8:
        i = 0;
loop:
        if (i < CarWork_GetGearCount(work)) {
            int n = i + 1;
            short v = values->value;
            values++;
            i = n;
            work->setup.t11_values[i] = v;
            goto loop;
        }
        work->setup.t11_4c = values->value;
        break;
    case 9:
        work->setup.t11_4e = values->value;
        if (CarWork_TransIsCustom(work, work->partSlots[6])) {
            CarWork_SetCustomRatios(work);
        }
        break;
    case 0x10:
        work->setup.t05_52 = values[0].value;
        work->setup.t05_53 = values[1].value;
        break;
    case 10:
    case 11:
    case 12:
    case 13:
        for (j = 0; j < count; j++, values++) {
            ((unsigned char *)work)[D_800915A0[values->id]] = values->value;
        }
        break;
    case 0xe:
        work->setup.t1b_74 = values->value;
        break;
    case 0xf:
        work->setup.t1c_75 = values->value;
        break;
    }
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/garage", CarWork_ApplySetting);
#endif

// Fills out with setting's values and their ranges; returns how many, or -1 if the
// fitted parts don't allow changing it.
#ifdef NON_MATCHING
// The game uses one more saved register (the transmission entry in case 8), other
// register numbers throughout, and shares more case tails. Its jump table is in
// resident_data_8008f990.
int CarWork_GetSettingRange(CarWork *work, int setting, SettingRange *out)
{
    int count = -1;
    unsigned char *e;
    int i, n;
    short ratios[8];
    short minRatios[8];
    short maxRatios[8];

    switch (setting) {
    case 0:
        if (work->partSlots[0] < 3) return count;
        e = work->suspEntries[work->partSlots[0]];
        out[0].value = work->setup.t12_60[0];
        out[0].max = e[0x1e];
        out[0].min = e[0x1d];
        out[1].value = work->setup.t12_60[1];
        out[1].max = e[0x21];
        out[1].min = e[0x20];
        count = 2;
        break;
    case 1:
        if (work->partSlots[0] < 2) return count;
        e = work->suspEntries[work->partSlots[0]];
        out[0].value = work->setup.t12_5a[2];
        out[0].max = e[0x14];
        out[0].min = e[0x13];
        out[1].value = work->setup.t12_5a[3];
        out[1].max = e[0x17];
        out[1].min = e[0x16];
        count = 2;
        break;
    case 2:
        if (work->partSlots[0] <= 0) break;
        e = work->suspEntries[work->partSlots[0]];
        out[0].value = work->setup.t12_60[4];
        out[0].min = 1;
        out[0].max = e[0x27];
        out[1].value = work->setup.t12_60[8];
        out[1].min = 1;
        out[1].max = e[0x35];
        count = 2;
        break;
    case 3:
        if (!CarWork_HasFullSuspension(work)) break;
        out[0].value = work->setup.t12_60[6];
        e = work->suspEntries[work->partSlots[0]];
        out[0].min = 1;
        out[0].max = e[0x2e];
        out[1].value = work->setup.t12_60[10];
        out[1].min = 1;
        out[1].max = e[0x3c];
        count = 2;
        break;
    case 4:
        if (work->partSlots[0] <= 0) return count;
        e = work->suspEntries[work->partSlots[0]];
        out[0].value = work->setup.t12_5a[0];
        out[0].max = e[0xa];
        out[0].min = e[9];
        out[1].value = work->setup.t12_5a[1];
        out[1].max = e[0xd];
        out[1].min = e[0xc];
        count = 2;
        break;
    case 5:
        if (work->partSlots[0] < 3) return count;
        e = work->suspEntries[work->partSlots[0]];
        out[0].value = work->setup.setting_5e[0] - 0x80;
        out[0].max = e[0x10] - 0x80;
        out[0].min = e[0xf] - 0x80;
        out[1].value = work->setup.setting_5e[1] - 0x80;
        out[1].max = e[0x12] - 0x80;
        out[1].min = e[0x11] - 0x80;
        count = 2;
        break;
    case 6:
        if (work->partSlots[0] < 3) return count;
        e = work->suspEntries[work->partSlots[0]];
        out[0].value = work->setup.t12_60[12];
        out[0].min = 1;
        out[0].max = e[0x43];
        out[1].value = work->setup.t12_60[13];
        out[1].min = 1;
        out[1].max = e[0x47];
        count = 2;
        break;
    case 7:
        if (work->partSlots[2] <= 0) break;
        e = work->brakeBalEntries[work->partSlots[2]];
        out[0].value = work->setup.t01_50[0];
        out[0].min = 1;
        out[0].max = e[9];
        out[1].value = work->setup.t01_50[1];
        out[1].min = 1;
        out[1].max = e[0xd];
        count = 2;
        break;
    case 8:
        if (work->partSlots[6] < 3) return count;
        e = work->transEntries[work->partSlots[6]];
        CarWork_GetGearRange(work, ratios, minRatios, maxRatios);
        for (i = 0; i < CarWork_GetGearCount(work); out++) {
            i++;
            out->value = work->setup.t11_values[i];
            out->max = maxRatios[i];
            out->min = minRatios[i];
            if (out->max < out->value) out->max = out->value;
            if (out->value < out->min) out->min = out->value;
        }
        out->value = work->setup.t11_4c;
        out->max = *(unsigned short *)&e[0x1c];
        out->min = *(unsigned short *)&e[0x1e];
        count = CarWork_GetGearCount(work) + 1;
        break;
    case 9:
        if (work->partSlots[6] < 3) return count;
        e = work->transEntries[work->partSlots[6]];
        out[0].value = work->setup.t11_4e;
        out[0].max = e[0x23];
        out[0].min = e[0x22];
        count = 1;
        break;
    case 0x10:
        if (work->partSlots[0x13] <= 0) return count;
        e = work->racingModEntries[work->partSlots[0x13]];
        out[0].value = work->setup.t05_52;
        out[0].max = e[0x11];
        out[0].min = e[0x10];
        out[1].value = work->setup.t05_53;
        out[1].max = e[0x14];
        out[1].min = e[0x13];
        if (out[0].min < out[0].max) count = 2;
        break;
    case 0xe:
        if (work->partSlots[0x14] <= 0) return count;
        e = work->ascEntries[work->partSlots[0x14]];
        out[0].value = work->setup.t1b_74;
        out[0].max = e[0xd];
        out[0].min = e[0xc];
        count = 1;
        break;
    case 0xf:
        if (work->partSlots[0x15] <= 0) return count;
        e = work->tcsEntries[work->partSlots[0x15]];
        out[0].value = work->setup.t1c_75;
        out[0].max = e[0xe];
        out[0].min = e[0xd];
        count = 1;
        break;
    case 10:
    case 11:
    case 12:
        // Two settings each, kept only when they have a range.
        n = (setting - 10) * 2;
        count = 0;
        e = work->lsdEntries[work->partSlots[0x16]];
        for (i = 0; i < 2; i++) {
            out->value = ((unsigned char *)work)[D_800915A0[i + n]];
            out->max = e[D_800915B8[i + n][2]];
            out->min = e[D_800915B8[i + n][1]];
            out->id = D_80091600[i + n];
            if (out->min < out->max) {
                out++;
                count++;
            }
        }
        if (count > 0) return count;
        count = -1;
        break;
    case 13:
        if (work->partSlots[0x16] < 5) return count;
        e = work->lsdEntries[work->partSlots[0x16]];
        out[0].value = work->setup.t15_6e[1];
        count = 1;
        out[0].max = e[0x19];
        out[0].id = count;
        out[0].min = e[0x18];
        break;
    }
    return count;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/garage", CarWork_GetSettingRange);
#endif

// Sets setting back to the fitted parts' defaults.
#ifdef NON_MATCHING
// Close, but the game tests CarWork_HasFullSuspension with xori and beqz, keeps the
// gear index's increment in another register and adds some entry offsets the other
// way round. Its jump table is in resident_data_8008f990.
void CarWork_ResetSetting(CarWork *work, int setting)
{
    int i, j;
    short ratios[8];
    short minRatios[8];
    short maxRatios[8];

    switch (setting) {
    case 0: {
        unsigned char *e = work->suspEntries[work->partSlots[0]];
        work->setup.t12_60[0] = e[0x1f];
        work->setup.t12_60[1] = e[0x22];
        break;
    }
    case 1: {
        unsigned char *e = work->suspEntries[work->partSlots[0]];
        work->setup.t12_5a[2] = e[0x15];
        work->setup.t12_5a[3] = e[0x18];
        break;
    }
    case 2: {
        unsigned char *e = work->suspEntries[work->partSlots[0]];
        work->setup.t12_60[4] = e[0x2a];
        work->setup.t12_60[5] = e[0x2d];
        work->setup.t12_60[8] = e[0x38];
        work->setup.t12_60[9] = e[0x3b];
        if (!CarWork_HasFullSuspension(work)) {
            work->setup.t12_60[6] = e[0x31];
            work->setup.t12_60[7] = e[0x34];
            work->setup.t12_60[10] = e[0x3f];
            work->setup.t12_60[11] = e[0x42];
        }
        break;
    }
    case 3: {
        unsigned char *e = work->suspEntries[work->partSlots[0]];
        work->setup.t12_60[6] = e[0x31];
        work->setup.t12_60[7] = e[0x34];
        work->setup.t12_60[10] = e[0x3f];
        work->setup.t12_60[11] = e[0x42];
        break;
    }
    case 4: {
        unsigned char *e = work->suspEntries[work->partSlots[0]];
        work->setup.t12_5a[0] = e[0xb];
        work->setup.t12_5a[1] = e[0xe];
        break;
    }
    case 5:
        work->setup.setting_5e[0] = 0x80;
        work->setup.setting_5e[1] = 0x80;
        break;
    case 6: {
        unsigned char *e = work->suspEntries[work->partSlots[0]];
        work->setup.t12_60[12] = e[0x46];
        work->setup.t12_60[13] = e[0x4a];
        break;
    }
    case 7: {
        unsigned char *e = work->brakeBalEntries[work->partSlots[2]];
        work->setup.t01_50[0] = e[0xc];
        work->setup.t01_50[1] = e[0xc];
        break;
    }
    case 8: {
        unsigned char *e = work->transEntries[work->partSlots[6]];
        work->setup.t11_4c = *(unsigned short *)&e[0x1a];
        i = 0;
    loop:
        if (i < CarWork_GetGearCount(work)) {
            short r = ((short *)&e[0xc])[i];
            int n = i + 1;
            i = n;
            work->setup.t11_values[i] = r;
            goto loop;
        }
        if (CarWork_TransIsCustom(work, work->partSlots[6])) {
            CarWork_GetGearRange(work, ratios, minRatios, maxRatios);
            for (j = 0; j < 8; j++) {
                work->setup.t11_values[j] = ratios[j];
            }
        }
        break;
    }
    case 9: {
        work->setup.t11_4e = work->transEntries[work->partSlots[6]][0x21];
        break;
    }
    case 0x10: {
        unsigned char *e = work->racingModEntries[work->partSlots[0x13]];
        work->setup.t05_52 = e[0x12];
        work->setup.t05_53 = e[0x15];
        break;
    }
    case 0xe:
        work->setup.t1b_74 = work->ascEntries[work->partSlots[0x14]][0xb];
        break;
    case 0xf:
        work->setup.t1c_75 = work->tcsEntries[work->partSlots[0x15]][0xc];
        break;
    case 10:
    case 13: {
        unsigned char *e = work->lsdEntries[work->partSlots[0x16]];
        work->setup.t15_6e[0] = e[0xd];
        work->setup.t15_6e[1] = e[0x17];
        break;
    }
    case 11: {
        unsigned char *e = work->lsdEntries[work->partSlots[0x16]];
        work->setup.t15_6e[2] = e[0x10];
        work->setup.t15_6e[3] = e[0x1a];
        break;
    }
    case 12: {
        unsigned char *e = work->lsdEntries[work->partSlots[0x16]];
        work->setup.t15_6e[4] = e[0x13];
        work->setup.t15_6e[5] = e[0x1d];
        break;
    }
    }
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/garage", CarWork_ResetSetting);
#endif

// Plays menu sound sfx: one sample in the centre, or two panned left and right.
void Sfx_PlayMenu(int sfx)
{
    unsigned char (*t)[2] = D_8009160C;
    unsigned char *e = t[sfx];
    int left = e[0];
    int right = e[1];

    if (left == right) {
        SoundBank_Play(&g_CommonSoundBank, right, 0x40, 0x4000, -1);
    } else {
        SoundBank_Play(&g_CommonSoundBank, left, 0, 0x4000, -1);
        SoundBank_Play(&g_CommonSoundBank, right, 0x7f, 0x4000, -1);
    }
}

int FUN_800608e8(unsigned int param_1, int param_2)
{
    unsigned int uVar1;
    char *pcVar2;
    int iVar3;

    iVar3 = 4;
    do {
        uVar1 = param_1 & 0x3f;
        param_1 = param_1 >> 6;
        pcVar2 = (char *)(param_2 + iVar3);
        iVar3 = iVar3 + -1;
        *pcVar2 = D_80091620[uVar1];
    } while (iVar3 != -1);
    *(unsigned char *)(param_2 + 5) = 0;
    return param_2;
}
}
