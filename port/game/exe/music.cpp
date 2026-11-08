// Resident code 80080eec-80081084 (music): plays the tracks of MUSIC.DAT as streamed XA.
#include "gptr.h"
#include "common.h"
#include "gamestate.h"

// An entry of the music track table: the track's name and its sector range in MUSIC.DAT.
struct MusicTrack {
    game_pointer<void> name;
    unsigned int start;
    unsigned int end;
};

extern "C" {
extern game_pointer<MusicTrack> (&D_80095ABC); // Disc_LocateDataFiles' table
extern unsigned int (&g_MusicLba); // MUSIC.DAT's sector, 0 if the disc has none

void Music_Init(void *tracks, unsigned int lba)
{
    D_80095ABC = (MusicTrack *)tracks;
    g_MusicLba = lba;
}

void *func_80080F00(int track)
{
    return D_80095ABC[track].name;
}

// Starts streaming a track at the music volume of the options. Returns CdAudio_RunScript's
// result, 0 without music data.
int Music_PlayTrack(int track, int loop)
{
    unsigned int lba = g_MusicLba;
    int result;

    if (lba != 0) {
        MusicTrack *t = &D_80095ABC[track];
        unsigned int start = t->start + lba;
        unsigned int end = t->end + lba;
        CdAudio *cd;
        unsigned char *req;
        int one;
        int mode;
        long long vol;

        CdAudio_ResetRead();
        vol = (long long)((int)(g_GameState.options2[5] << 15) / 0xff) * 0x8cc;
        mode = 0x40;
        cd = &g_CdAudio;
        one = 1;
        req = &cd->reqFlag;
        cd->reqMode = mode;
        req[2] = one;
        req[3] = track + one;
        cd->reqStart = start;
        cd->reqEnd = end;
        cd->reqFlag = loop != 0;
        cd->onSector = 0;
        cd->masterVol = vol >> 12;
        result = CdAudio_RunScript(5);
    } else {
        result = 0;
    }
    return result;
}

// The track's length: its sectors at 75 a second, the 0x4a rounding it up.
unsigned int Music_GetTrackSeconds(int track)
{
    MusicTrack *t = &D_80095ABC[track];
    unsigned int start = t->start;

    return (t->end - start + 0x4a) / 75;
}

void func_8008107C(void) {
}

}
