// OVL0 80057628-80057a70 (powergraph): Power graph.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "gptr.h"
#include "common.h"

// A part to try, by type and stage.
struct PowerGraphPart {
    short type;
    short stage;
};

// A candidate's stats and curves (0x434 bytes).
struct PowerGraphCand {
    PowerGraphPart part;
    CarSpec spec;
    CarParams params;
    EnginePerf perf;
    short power[80];
    short torque[80];
};

struct PowerGraph {
    game_pointer<CarWork> work;
    game_pointer<PowerGraphCand> cands;
    signed char step;
    signed char count;
    signed char ready[10];
};

extern "C" {
// Clears the computed-curve flags; buffer receives the curves (0x434 bytes per
// candidate).
void PowerGraph_Reset(void *g, void *buffer)
{
    int iVar1;
    int iVar2;

    iVar2 = 9;
    iVar1 = (int)rt::to_guest(g) + 9;
    *(unsigned char *)rt::from_guest(((int)rt::to_guest(g) + 8)) = 0;
    *(unsigned char *)rt::from_guest(((int)rt::to_guest(g) + 9)) = 0;
    *(game_pointer<void> *)rt::from_guest(((int)rt::to_guest(g) + 4)) = buffer;
    do {
        *(unsigned char *)rt::from_guest((iVar1 + 10)) = 0;
        iVar2 = iVar2 + -1;
        iVar1 = iVar1 + -1;
    } while (-1 < iVar2);
}

// Sets up the candidates: the car as it is (0), then each part in parts.
void PowerGraph_Start(PowerGraph *g, int count, CarWork *work, PowerGraphPart *parts)
{
    int i;

    g->step = 0;
    g->count = count + 1;
    g->work = work;
    g->ready[0] = 0;
    for (i = 0; i < count; i++) {
        g->cands[i + 1].part = parts[i];
        g->ready[i + 1] = 0;
    }
}

// Drops the candidates (count 0, flags cleared).
void PowerGraph_Clear(void *g)
{
    int iVar1;
    int iVar2;

    *(unsigned char *)rt::from_guest(((int)rt::to_guest(g) + 9)) = 0;
    iVar1 = 9;
    iVar2 = (int)rt::to_guest(g) + 9;
    do {
        *(unsigned char *)rt::from_guest((iVar2 + 10)) = 0;
        iVar1 = iVar1 + -1;
        iVar2 = iVar2 + -1;
    } while (-1 < iVar1);
}

// Computes the next candidate's curves. Returns 1 when that was the last one.
int PowerGraph_Step(PowerGraph *g)
{
    int done = 0;
    int ok = 0;
    int i = g->step;

    if (i < g->count) {
        PowerGraphCand *c = &g->cands[i];

        if (i == 0) {
            CarSpec_FromCarWork(g->work, &c->spec);
            ok = 1;
        } else {
            int stage;
            int type;

            type = c->part.type;
            stage = c->part.stage;

            if (CarWork_GetPartIdx(g->work, type, stage) >= 0) {
                CarSpec_FromCarWorkWithPart(g->work, &c->spec, type, stage);
                ok = 1;
            }
        }
        if (ok != 0) {
            CarParams_Build(&c->spec, &c->params);
            CarParams_GetPerformance(&c->params, &c->perf);
            EngineCurve_Resample((unsigned short)c->perf.pointCount, c->perf.rpm, c->perf.power, c->perf.torque, c->power,
                                 c->torque);
            g->ready[i] = 1;
        }
        if (++g->step == g->count) {
            done = 1;
        }
    }
    return done;
}

// The highest rpm, power and torque of the candidates computed.
void PowerGraph_GetMax(PowerGraph *g, int *out)
{
    int maxRpm = -1;
    int maxPower = maxRpm;
    int maxTorque = maxRpm;
    int i;

    for (i = 0;; i++) {
        PowerGraphCand *c;

        if (i >= g->count) {
            break;
        }
        c = &g->cands[i];
        if (g->ready[i] != 0) {
            EnginePerf *perf = &c->perf;
            int v = perf->rpm[perf->pointCount - 1];

            if (maxRpm < v) {
                maxRpm = v;
            }
            v = c->perf.maxPower;
            if (maxPower < v) {
                maxPower = v;
            }
            v = (unsigned short)perf->maxTorque;
            if (maxTorque < v) {
                maxTorque = v;
            }
        }
    }
    out[0] = maxRpm;
    out[1] = maxPower;
    out[2] = maxTorque;
}

short *PowerGraph_GetPowerCurve(PowerGraph *g, int i)
{
    short *curve = 0;

    if (g->ready[i] != 0) {
        curve = g->cands[i].power;
    }
    return curve;
}

short *PowerGraph_GetTorqueCurve(PowerGraph *g, int i)
{
    short *curve = 0;

    if (g->ready[i] != 0) {
        curve = g->cands[i].torque;
    }
    return curve;
}

void PowerGraph_GetPeak(PowerGraph *g, unsigned short *out, int i)
{
    if (g->ready[i] != 0) {
        out[6] = g->cands[i].perf.maxPower;
        out[7] = g->cands[i].perf.maxPowerRpm;
        out[8] = g->cands[i].perf.maxTorque;
        out[9] = g->cands[i].perf.maxTorqueRpm;
    }
}

int PowerGraph_GetPower(PowerGraph *g, int i)
{
    return g->ready[i] != 0 ? g->cands[i].perf.maxPower : -1;
}
}
