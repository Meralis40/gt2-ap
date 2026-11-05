// OVL0 80016a3c-80016b78 (racemem): Race memory.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "common.h"

extern "C" {
extern RaceSetup g_RaceSetup;
extern RaceWorld g_RaceWorld;
void Race_LoadEngineBanks(void);
void RaceCars_LoadModels(RaceWorld *world);
// Clears the three flags at mem + 0x11d700 (0x801c6c00..02), so the course and car
// assets are reloaded.
void RaceMem_ResetFlags(void *mem)
{
    *(unsigned char *)((int)mem + 0x11d700) = 0;
    *(unsigned char *)((int)mem + 0x11d701) = 0;
    *(unsigned char *)((int)mem + 0x11d702) = 0;
}

// Packet pool (mem + 0xe5700, 0x38000 bytes) and OT buffer (mem + 0xd4cd0, 0x10a30
// bytes) of the race view.
void RaceMem_InitDrawBuffers(void *mem, psxViewLoop *view)
{
    PacketBuffer_Init(&view->packets, (void *)((int)mem + 0xe5700), 0x38000);
    ViewDrawContext_Init(&view->draw, (void *)((int)mem + 0xd4cd0), 0x10a30);
}

// Loads the engine banks and car models once, then starts each car's model.
void RaceMem_LoadCars(RaceMem *race)
{
    int i;

    if (*((unsigned char *)race + 0x11d701) == 0) {
        *((unsigned char *)race + 0x11d701) = 1;
        Race_LoadEngineBanks();
        RaceCars_LoadModels(&race->world);
    }
    for (i = 0; i < g_RaceSetup.carCount; i++) {
        CarModel_InitStub((CarModelHeader *)((char *)race + 0xb6394 + i * 0x5000), g_RaceWorld.isReplay != 0);
    }
}
}
