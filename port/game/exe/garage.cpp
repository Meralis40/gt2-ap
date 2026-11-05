// Resident code 8005e7b0-80060410 (garage). Ghidra's decompilation; the functions
// that don't match yet are the original assembly.
#include "common.h"

extern "C" {
extern int D_800915A0[];     // CarSetup offset of each LSD setting
extern int D_800915B8[][3];  // LSD entry offsets of each LSD setting's minimum ([1]) and maximum ([2])
extern short D_80091600[];   // id of each LSD setting

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
    spec.frontTyre18 = work->frontTyres18[front].data;
    spec.frontTyre19 = work->frontTyres19[front].data;
    spec.frontTyre1a = work->frontTyres1a[front].data;
    spec.rearTyre18 = work->rearTyres18[rear].data;
    spec.rearTyre19 = work->rearTyres19[rear].data;
    spec.t11_4c = work->setup.t11_4c_copy;
    spec.t11_4e = work->setup.t11_4e_copy;
    CarParams_Build(&spec, &params);
    params.unk1aa = 0;
    Gearbox_ComputeRatios(&params, params.gearTopSpeedDiv10 * 10, ratios);
    Gearbox_ComputeLimits(params.gearCount, ratios, minRatios, maxRatios);
}

int CarWork_TransIsCustom(CarWork *work, int stage)
{
    return (unsigned int)(work->trans[stage].bytes[0x20] != '\0');
}

// Fits stage of slot: sets the setup's part index and applies the part's entry.
void CarWork_SetPartSlot(CarWork *work, int slot, int stage)
{
    *(work->partSlots + slot) = stage;
    switch (slot) {
    case 0:
        work->setup.idx_t12 = work->suspIdx[stage];
        CarSetup_ApplyPartEntry(0x12, work->susp[stage].bytes, &work->setup);
        break;
    case 1:
        work->setup.idx_t00 = work->brakesIdx[stage];
        break;
    case 2:
        work->setup.idx_t01 = work->brakeBalIdx[stage];
        CarSetup_ApplyPartEntry(1, work->brakeBal[stage].bytes, &work->setup);
        break;
    case 3:
        work->setup.idx_t16_front = work->frontTyresIdx[stage];
        break;
    case 4:
        work->setup.idx_t16_rear = work->rearTyresIdx[stage];
        break;
    case 5:
        work->setup.idx_t1d = work->wheelIdx[stage];
        break;
    case 6:
        work->setup.idx_t11 = work->transIdx[stage];
        CarSetup_ApplyPartEntry(0x11, work->trans[stage].bytes, &work->setup);
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
        CarSetup_ApplyPartEntry(0xc, work->turbo[0].bytes, &work->setup);
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
        CarSetup_ApplyPartEntry(0xc, work->turbo[stage].bytes, &work->setup);
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
        CarSetup_ApplyPartEntry(5, work->racingMod[stage].bytes, &work->setup);
        break;
    case 0x14:
        work->setup.idx_t1b = work->ascIdx[stage];
        CarSetup_ApplyPartEntry(0x1b, work->asc[stage].bytes, &work->setup);
        break;
    case 0x15:
        work->setup.idx_t1c = work->tcsIdx[stage];
        CarSetup_ApplyPartEntry(0x1c, work->tcs[stage].bytes, &work->setup);
        break;
    case 0x16:
        work->setup.idx_t15 = work->lsdIdx[stage];
        CarSetup_ApplyPartEntry(0x15, work->lsd[stage].bytes, &work->setup);
        break;
    }
}

// The entry index of stage of slot (-1 = not available).
int CarWork_GetPartIdx(CarWork *work, int slot, int stage)
{
    int idx;

    switch (slot) {
    case 0: idx = work->suspIdx[stage]; break;
    case 1: idx = work->brakesIdx[stage]; break;
    case 2: idx = work->brakeBalIdx[stage]; break;
    case 3: idx = work->frontTyresIdx[stage]; break;
    case 4: idx = work->rearTyresIdx[stage]; break;
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
        spec->suspension = work->susp[stage].bytes;
        CarSpec_ApplyPartEntry(0x12, spec->suspension, spec);
        break;
    case 1:
        spec->brakes = work->brakes[stage].bytes;
        CarSpec_ApplyPartEntry(0, spec->brakes, spec);
        break;
    case 2:
        spec->brakeBalance = work->brakeBal[stage].bytes;
        CarSpec_ApplyPartEntry(1, spec->brakeBalance, spec);
        break;
    case 3:
        spec->frontTyre18 = work->frontTyres18[stage].data;
        spec->frontTyre19 = work->frontTyres19[stage].data;
        spec->frontTyre1a = work->frontTyres1a[stage].data;
        break;
    case 4:
        spec->rearTyre18 = work->rearTyres18[stage].data;
        spec->rearTyre19 = work->rearTyres19[stage].data;
        break;
    case 5:
        break;
    case 6:
        spec->transmission = work->trans[stage].bytes;
        CarSpec_ApplyPartEntry(0x11, spec->transmission, spec);
        if (CarWork_TransIsCustom(work, stage)) {
            CarWork_ComputeGearRatios(work, ratios);
            for (i = 0; i < 8; i++) {
                spec->t11_values[i] = ratios[i];
            }
        }
        break;
    case 7:
        spec->clutch = work->clutch[stage].bytes;
        CarSpec_ApplyPartEntry(0xf, spec->clutch, spec);
        break;
    case 8:
        spec->flywheel = work->flywheel[stage].bytes;
        CarSpec_ApplyPartEntry(0xe, spec->flywheel, spec);
        break;
    case 9:
        spec->driveshaft = work->driveshaft[stage].bytes;
        CarSpec_ApplyPartEntry(0x10, spec->driveshaft, spec);
        break;
    case 10:
        spec->chip = work->chip[stage].bytes;
        CarSpec_ApplyPartEntry(0xa, spec->chip, spec);
        break;
    case 0xb:
        spec->portPolish = work->portPolish[stage].bytes;
        CarSpec_ApplyPartEntry(7, spec->portPolish, spec);
        break;
    case 0xc:
        spec->balancing = work->balance[stage].bytes;
        CarSpec_ApplyPartEntry(8, spec->balancing, spec);
        break;
    case 0xd:
        spec->naTune = work->naTune[stage].bytes;
        CarSpec_ApplyPartEntry(0xb, spec->naTune, spec);
        spec->turbo = work->turbo[0].bytes;
        CarSpec_ApplyPartEntry(0xc, spec->turbo, spec);
        spec->intercooler = work->intercooler[0].bytes;
        CarSpec_ApplyPartEntry(0x13, spec->intercooler, spec);
        break;
    case 0xe:
        spec->displacement = work->displ[stage].bytes;
        CarSpec_ApplyPartEntry(9, spec->displacement, spec);
        break;
    case 0xf:
        spec->turbo = work->turbo[stage].bytes;
        CarSpec_ApplyPartEntry(0xc, spec->turbo, spec);
        spec->naTune = work->naTune[0].bytes;
        // The game applies the first LSD entry here, not the stock NA tune.
        CarSpec_ApplyPartEntry(0x15, work->lsd[0].bytes, spec);
        break;
    case 0x10:
        spec->intercooler = work->intercooler[stage].bytes;
        CarSpec_ApplyPartEntry(0x13, spec->intercooler, spec);
        break;
    case 0x11:
        spec->muffler = work->muffler[stage].bytes;
        CarSpec_ApplyPartEntry(0x14, spec->muffler, spec);
        break;
    case 0x12:
        spec->weightReduction = work->weight[stage].bytes;
        CarSpec_ApplyPartEntry(4, spec->weightReduction, spec);
        break;
    case 0x13:
        spec->racingMod = work->racingMod[stage].bytes;
        CarSpec_ApplyPartEntry(5, spec->racingMod, spec);
        break;
    case 0x14:
        spec->asc = work->asc[stage].bytes;
        CarSpec_ApplyPartEntry(0x1b, spec->asc, spec);
        break;
    case 0x15:
        spec->tcs = work->tcs[stage].bytes;
        CarSpec_ApplyPartEntry(0x1c, spec->tcs, spec);
        break;
    case 0x16:
        spec->lsd = work->lsd[stage].bytes;
        CarSpec_ApplyPartEntry(0x15, spec->lsd, spec);
        break;
    }
}

// Points spec at the entries of the stages fitted in work's part slots.
void CarSpec_FromCarWork(CarWork *work, CarSpec *spec)
{
    int stage;

    CarSpec_CopySetupFields(spec, &work->setup);
    spec->suspension = work->susp[work->partSlots[0]].bytes;
    spec->brakes = work->brakes[work->partSlots[1]].bytes;
    spec->brakeBalance = work->brakeBal[work->partSlots[2]].bytes;
    spec->transmission = work->trans[work->partSlots[6]].bytes;
    spec->clutch = work->clutch[work->partSlots[7]].bytes;
    spec->flywheel = work->flywheel[work->partSlots[8]].bytes;
    spec->driveshaft = work->driveshaft[work->partSlots[9]].bytes;
    spec->chip = work->chip[work->partSlots[10]].bytes;
    spec->portPolish = work->portPolish[work->partSlots[0xb]].bytes;
    spec->balancing = work->balance[work->partSlots[0xc]].bytes;
    spec->naTune = work->naTune[work->partSlots[0xd]].bytes;
    spec->displacement = work->displ[work->partSlots[0xe]].bytes;
    spec->turbo = work->turbo[work->partSlots[0xf]].bytes;
    spec->intercooler = work->intercooler[work->partSlots[0x10]].bytes;
    spec->muffler = work->muffler[work->partSlots[0x11]].bytes;
    spec->weightReduction = work->weight[work->partSlots[0x12]].bytes;
    spec->racingMod = work->racingMod[work->partSlots[0x13]].bytes;
    spec->asc = work->asc[work->partSlots[0x14]].bytes;
    spec->tcs = work->tcs[work->partSlots[0x15]].bytes;
    spec->lsd = work->lsd[work->partSlots[0x16]].bytes;
    spec->t02 = &work->t02.bytes[work->partSlots[0x17] * 24];
    spec->t03 = &work->t03.bytes[work->partSlots[0x18] * 20];
    spec->t06 = &work->t06.bytes[work->partSlots[0x19] * 76];
    spec->t0d = &work->t0d.bytes[work->partSlots[0x1a] * 16];
    stage = work->partSlots[3];
    spec->frontTyre18 = work->frontTyres18[stage].data;
    spec->frontTyre19 = work->frontTyres19[stage].data;
    spec->frontTyre1a = work->frontTyres1a[stage].data;
    stage = work->partSlots[4];
    spec->rearTyre18 = work->rearTyres18[stage].data;
    spec->rearTyre19 = work->rearTyres19[stage].data;
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
    return (unsigned int)work->trans[work->partSlots[6]].bytes[9];
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
    return (unsigned int)(work->susp[work->partSlots[0]].bytes[0x4b] == '\0');
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
// Close, but the game swaps the saved registers of work and the gear index. Its jump
// table is in resident_data_8008f990.
void CarWork_ApplySetting(CarWork *work, int setting, SettingRange *values, int count)
{
    int i, j;
    int none;

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
        // ^ 1 into a local gives the game's xori.
        none = CarWork_HasFullSuspension(work) ^ 1;
        if (none) {
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

// Fills out with setting's values and their ranges; returns how many, or -1 if the
// fitted parts don't allow changing it.
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
        e = work->susp[work->partSlots[0]].bytes;
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
        e = work->susp[work->partSlots[0]].bytes;
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
        e = work->susp[work->partSlots[0]].bytes;
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
        e = work->susp[work->partSlots[0]].bytes;
        out[0].min = 1;
        out[0].max = e[0x2e];
        out[1].value = work->setup.t12_60[10];
        out[1].min = 1;
        out[1].max = e[0x3c];
        count = 2;
        break;
    case 4:
        if (work->partSlots[0] <= 0) return count;
        e = work->susp[work->partSlots[0]].bytes;
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
        e = work->susp[work->partSlots[0]].bytes;
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
        e = work->susp[work->partSlots[0]].bytes;
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
        e = work->brakeBal[work->partSlots[2]].bytes;
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
        e = work->trans[work->partSlots[6]].bytes;
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
        e = work->trans[work->partSlots[6]].bytes;
        out[0].value = work->setup.t11_4e;
        out[0].max = e[0x23];
        out[0].min = e[0x22];
        count = 1;
        break;
    case 0x10:
        if (work->partSlots[0x13] <= 0) return count;
        e = work->racingMod[work->partSlots[0x13]].bytes;
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
        e = work->asc[work->partSlots[0x14]].bytes;
        out[0].value = work->setup.t1b_74;
        out[0].max = e[0xd];
        out[0].min = e[0xc];
        count = 1;
        break;
    case 0xf:
        if (work->partSlots[0x15] <= 0) return count;
        e = work->tcs[work->partSlots[0x15]].bytes;
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
        e = work->lsd[work->partSlots[0x16]].bytes;
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
        e = work->lsd[work->partSlots[0x16]].bytes;
        out[0].value = work->setup.t15_6e[1];
        count = 1;
        out[0].max = e[0x19];
        out[0].id = count;
        out[0].min = e[0x18];
        break;
    }
    return count;
}
}
