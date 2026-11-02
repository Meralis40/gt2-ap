// OVL4 80021a40-80021c38 (gtracedata): Race data events and wheels.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "ovl4.h"

extern "C" {
void *GameData_GetEntryIn(void *data, int table, int index);
char *GameData_GetTableString(void *data, int table, unsigned int index);
int GameData_BinarySearch(void *data, int table, unsigned int carCode);
extern void *g_GtModeRaceData;
extern void *g_GtModeData;

// Entry index of table 0 of g_GtModeRaceData.
unsigned short *GtRaceData_GetEvent(int index)
{
    return (unsigned short *)GameData_GetEntryIn(g_GtModeRaceData, 0, index);
}

// The event of the race data whose code is eventCode, or 0.
unsigned short *GtRaceData_FindEvent(char *eventCode)
{
    void *data = g_GtModeRaceData;
    int count = *(unsigned short *)((char *)data + 0xe);
    int i;
    unsigned short *event;

    for (i = 0; i < count; i++) {
        event = GtRaceData_GetEvent(i);
        if (strcmp(eventCode, GameData_GetTableString(data, 0, *event)) == 0)
            return event;
    }
    return 0;
}

// The index of the wheel wheelCode in table 0x1d.
int Wheel_FindIndex(int wheelCode)
{
    return GameData_BinarySearch(g_GtModeData, 0x1d, wheelCode);
}

// Fits the wheels of table 0x1d's entry wheelIndex; the car's own wheels
// (code 0) take their other variant on a racing modified car.
void CarSetup_SetWheels(CarSetup *setup, int wheelIndex)
{
    void *data = g_GtModeData;
    WheelEntry *wheel = (WheelEntry *)GameData_GetEntryIn(data, 0x1d, wheelIndex);
    unsigned int code = wheel->code;
    unsigned int variant = wheel->variant;

    if (code == 0) {
        unsigned char *rm = (unsigned char *)GameData_GetEntryIn(data, 5, (unsigned short)setup->idx_t05);

        if (rm[0xe] != 0)
            variant = wheel->altVariant;
    }
    setup->wheel = code | (variant & 0x1f) << 8;
    setup->idx_t1d = wheelIndex;
}

// The variant of the wheel wheelCode, or its other one if altVariant.
unsigned char Wheel_GetVariant(int wheelCode, int altVariant)
{
    WheelEntry *wheel = (WheelEntry *)GameData_GetEntryIn(g_GtModeData, 0x1d, Wheel_FindIndex(wheelCode));
    unsigned char variant = wheel->variant;

    if (altVariant != 0)
        variant = wheel->altVariant;
    return variant;
}
}
