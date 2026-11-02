// Loads every part a car can use into a CarWork (copy of the OVL4 function).
#include "common.h"

// The stage byte at offset off of a game data entry.
#define STAGE(entry, off) (((unsigned char *)(entry))[off])

// Loads the stock entry of a category (its index in the default setup) at its
// stage, then the entry of each of count parts from part on at stages 1 on.
#define LOAD_CATEGORY(type, table, setupIdx, stageOff, field, part, count) \
    idx = setup.setupIdx;                                                   \
    e = GameData_GetEntry(table, idx);                                      \
    stage = STAGE(e, stageOff);                                             \
    work->field[stage] = *(type *)e;                                        \
    work->field##Idx[stage] = idx;                                          \
    for (i = 0; i < count; i++) {                                           \
        idx = Part_Lookup(i + part, carId, 0);                              \
        if (idx >= 0) {                                                     \
            work->field[i + 1] = *(type *)GameData_GetEntry(table, idx);    \
            work->field##Idx[i + 1] = idx;                                  \
        }                                                                   \
    }

// Fills work with the entries of every stage of every part carId can use.
extern "C" void CarWork_LoadParts(CarWork *work, unsigned int carId)
{
    CarSetup setup;
    int idx;
    int stage;
    int i;
    void *e;
    PartEntry20 *turbo;
    PartEntry28 *racingMod;

    Car_GetDefaultSetup(carId, &setup);
    LOAD_CATEGORY(PartEntry76, 0x12, idx_t12, 0x8, susp, 0x23, 3);
    LOAD_CATEGORY(PartEntry12, 0x0, idx_t00, 0x8, brakes, 0x1, 1);
    LOAD_CATEGORY(PartEntry16, 0x1, idx_t01, 0x8, brakeBal, 0x2, 1);

    // The tyres: the stock ones are also stages 8 and 9.
    {
        FrontTyreEntry *t;
        TyreEntry18 *t18;
        TyreEntry19 *t19;
        TyreEntry1a *t1a;

        idx = setup.idx_t16_front;
        t = (FrontTyreEntry *)GameData_GetEntry(0x16, idx);
        work->frontTyres[0] = *t;
        work->frontTyresIdx[0] = idx;
        work->frontTyres[8] = *t;
        work->frontTyresIdx[8] = idx;
        work->frontTyres[9] = *t;
        work->frontTyresIdx[9] = idx;
        t18 = (TyreEntry18 *)GameData_GetEntry(0x18, t->idx_t18);
        work->frontTyres18[0] = *t18;
        work->frontTyres18[8] = *t18;
        work->frontTyres18[9] = *t18;
        t19 = (TyreEntry19 *)GameData_GetEntry(0x19, t->idx_t19);
        work->frontTyres19[0] = *t19;
        t1a = (TyreEntry1a *)GameData_GetEntry(0x1a, t->idx_t1a);
        work->frontTyres1a[0] = *t1a;
        work->frontTyres1a[8] = *t1a;
        work->frontTyres1a[9] = *t1a;
        for (i = 0; i < 7; i++) {
            idx = Part_Lookup(i + 0x27, carId, 0);
            if (idx >= 0) {
                t = (FrontTyreEntry *)GameData_GetEntry(0x16, idx);
                work->frontTyres[i + 1] = *t;
                work->frontTyresIdx[i + 1] = idx;
                work->frontTyres18[i + 1] = *(TyreEntry18 *)GameData_GetEntry(0x18, t->idx_t18);
                t19 = (TyreEntry19 *)GameData_GetEntry(0x19, t->idx_t19);
                work->frontTyres19[i + 1] = *t19;
                work->frontTyres1a[i + 1] = *(TyreEntry1a *)GameData_GetEntry(0x1a, t->idx_t1a);
            }
        }
    }
    {
        RearTyreEntry *t;
        TyreEntry18 *t18;
        TyreEntry19 *t19;

        idx = setup.idx_t16_rear;
        t = (RearTyreEntry *)GameData_GetEntry(0x17, idx);
        work->rearTyres[0] = *t;
        work->rearTyresIdx[0] = idx;
        work->rearTyres[8] = *t;
        work->rearTyresIdx[8] = idx;
        work->rearTyres[9] = *t;
        work->rearTyresIdx[9] = idx;
        t18 = (TyreEntry18 *)GameData_GetEntry(0x18, t->idx_t18);
        work->rearTyres18[0] = *t18;
        work->rearTyres18[8] = *t18;
        work->rearTyres18[9] = *t18;
        t19 = (TyreEntry19 *)GameData_GetEntry(0x19, t->idx_t19);
        work->rearTyres19[0] = *t19;
        work->rearTyres19[8] = *t19;
        work->rearTyres19[9] = *t19;
        for (i = 0; i < 7; i++) {
            idx = Part_Lookup(i + 0x27, carId, 0);
            if (idx >= 0) {
                t = (RearTyreEntry *)GameData_GetEntry(0x17, idx);
                work->rearTyres[i + 1] = *t;
                work->rearTyresIdx[i + 1] = idx;
                work->rearTyres18[i + 1] = *(TyreEntry18 *)GameData_GetEntry(0x18, t->idx_t18);
                t19 = (TyreEntry19 *)GameData_GetEntry(0x19, t->idx_t19);
                work->rearTyres19[i + 1] = *t19;
            }
        }
    }

    LOAD_CATEGORY(PartEntry36, 0x11, idx_t11, 0x8, trans, 0xd, 3);
    LOAD_CATEGORY(PartEntry16, 0xf, idx_t0f, 0x8, clutch, 0x3, 3);
    LOAD_CATEGORY(PartEntry12, 0xe, idx_t0e, 0x8, flywheel, 0xa, 3);
    LOAD_CATEGORY(PartEntry12, 0x10, idx_t10, 0x8, driveshaft, 0x21, 1);
    LOAD_CATEGORY(PartEntry12, 0xa, idx_t0a, 0x8, chip, 0x6, 1);
    LOAD_CATEGORY(PartEntry12, 0x7, idx_t07, 0x8, portPolish, 0x20, 1);
    LOAD_CATEGORY(PartEntry12, 0x8, idx_t08, 0x8, balance, 0x9, 1);
    LOAD_CATEGORY(PartEntry12, 0xb, idx_t0b, 0x8, naTune, 0x1d, 3);
    LOAD_CATEGORY(PartEntry12, 0x9, idx_t09, 0x8, displ, 0x7, 1);

    // The turbo and racing modification entries are loaded through their own
    // pointers.
    idx = setup.idx_t0c;
    turbo = (PartEntry20 *)GameData_GetEntry(0xc, idx);
    stage = STAGE(turbo, 0x8);
    work->turbo[stage] = *turbo;
    work->turboIdx[stage] = idx;
    for (i = 0; i < 4; i++) {
        idx = Part_Lookup(i + 0x2e, carId, 0);
        if (idx >= 0) {
            turbo = (PartEntry20 *)GameData_GetEntry(0xc, idx);
            work->turbo[i + 1] = *turbo;
            work->turboIdx[i + 1] = idx;
        }
    }

    LOAD_CATEGORY(PartEntry12, 0x13, idx_t13, 0x8, intercooler, 0x10, 2);
    LOAD_CATEGORY(PartEntry12, 0x14, idx_t14, 0x8, muffler, 0x1a, 3);
    LOAD_CATEGORY(PartEntry12, 0x4, idx_t04, 0xb, weight, 0x12, 3);

    // The racing modification stages are looked up by stage.
    idx = setup.idx_t05;
    racingMod = (PartEntry28 *)GameData_GetEntry(0x5, idx);
    stage = STAGE(racingMod, 0xe);
    work->racingMod[stage] = *racingMod;
    work->racingModIdx[stage] = idx;
    for (i = 1; i < 5; i++) {
        int count;
        idx = Part_LookupRacingModStage(carId, i, &count);
        if (idx >= 0) {
            racingMod = (PartEntry28 *)GameData_GetEntry(0x5, idx);
            work->racingMod[i] = *racingMod;
            work->racingModIdx[i] = idx;
        }
    }

    LOAD_CATEGORY(PartEntry16, 0x1b, idx_t1b, 0x8, asc, 0x0, 1);
    LOAD_CATEGORY(PartEntry16, 0x1c, idx_t1c, 0x8, tcs, 0x26, 1);
    LOAD_CATEGORY(PartEntry32, 0x15, idx_t15, 0x8, lsd, 0x15, 5);

    // Categories without parts.
    idx = setup.idx_08;
    work->t02 = *(PartEntry24 *)GameData_GetEntry(0x2, idx);
    work->t02Idx = idx;
    idx = setup.idx_0a;
    work->t03 = *(PartEntry20 *)GameData_GetEntry(0x3, idx);
    work->t03Idx = idx;
    idx = setup.idx_0c;
    work->t06 = *(PartEntry76 *)GameData_GetEntry(0x6, idx);
    work->t06Idx = idx;
    idx = setup.idx_t0d;
    work->t0d = *(PartEntry16 *)GameData_GetEntry(0xd, idx);
    work->t0dIdx = idx;
}
