// OVL4 80019bf0-80019c60 (ghostfile): The ghost replay files of the events. OVL1 builds
// it too.
#include "common.h"

extern "C" {
extern GhostEventFile (&g_GhostEventFiles)[];

// The replay file of event eventCode's ghost, or that of IA dirt (no name).
VolBootId Event_GetGhostFileId(char *eventCode)
{
    int i;

    for (i = 0; g_GhostEventFiles[i].eventCode; i++) {
        if (strcmp(eventCode, g_GhostEventFiles[i].eventCode) == 0)
            return g_GhostEventFiles[i].fileId;
    }
    return VOLID_DIRT_IA_NO_NAME_DIRT;
}
}
