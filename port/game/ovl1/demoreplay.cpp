// OVL1 80020dcc-80020f2c (demoreplay): The demo replays, a replay file of the
// language that MainMenu_Run plays after 15 s idle.
#include "common.h"
#include "gamestate.h"

extern "C" {
extern VolBootId g_DemoReplay_Files[];

// A memory card sector of a replay file.
struct ReplaySector {
    unsigned char data[0x80];
};

// Reads the demo replay file of the current language into buf (a ReplayDir and its
// sectors); returns the number of replays in it.
int DemoReplay_Count(void *buf)
{
    unsigned int lang = g_GameState.language;

    VolId_ReadFileAsync(g_DemoReplay_Files[lang], buf);
    return ((ReplayDir *)buf)->count;
}

// Gathers replay index of file (DemoReplay_Count's buffer) from its sectors into dst,
// following the sector map, then unpacks it (Replay_Unpack).
void DemoReplay_Load(void *file, void *dst, int index)
{
    ReplaySector *out = (ReplaySector *)dst;
    ReplayDir *dir = (ReplayDir *)file;
    SectorMap *map = &dir->sectorMap;
    ReplaySector *sectors = (ReplaySector *)(dir + 1);
    ReplayDirEntry *e = &dir->entries[index];
    int count = e->sectorCount;
    int s = e->firstSector;
    int i;

    i = 0;
    while (1) {
        if (i >= count) {
            break;
        }
        *out++ = sectors[s];
        s = map->next[s];
        i++;
    }
    Replay_Unpack((char *)dst, 0);
}
}
