// OVL0 800299d8-80029d58 (racemusic): Race music.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "common.h"
#include "gamestate.h"

extern "C" {
extern RaceSetup g_RaceSetup;
extern RaceWorld g_RaceWorld;
extern RaceMem g_Race_IsDemo;
extern CdAudio g_CdAudio;
extern GameState g_GameState;
extern char D_8002F20C[];
int strcmp(const char *a, const char *b);

// g_RaceSetup, as the game reaches it: g_GameState plus 0xbf7c, added at run time.
static inline RaceSetup *RaceSetup_Get(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;

    return (RaceSetup *)(base + offset);
}

// Picks the race track and what plays before it, which stretches the intro to its length.
void RaceMusic_Init(RaceViewLoop *view)
{
    int none = -1;
    RaceMem *race;
    RaceWorld *world;
    int kind;

    view->musicPlaying = none;
    view->musicTrack = none;
    race = &g_Race_IsDemo;
    world = &race->world;
    view->musicLoop = 0;
    kind = Rand_Next(&view->randomSeed);
    view->musicNext = none;
    view->musicKind = 0;
    view->raceTrack = (unsigned int)kind % 6;
    switch (RaceSetup_Get()->raceType) {
    case 3:
        view->musicKind = 2;
        break;
    case 7:
        view->musicKind = 1;
        break;
    default:
        if (world->introTimer != 0) {
            int track;

            RaceSetup *setup = RaceSetup_Get();

            if (setup->worldLeague != 0) {
                view->musicKind = 4;
                track = 0xb;
            } else {
                int special = 0;

                if (setup->game == 1) {
                    special = strcmp(setup->eventName, D_8002F20C) == 0;
                }
                track = special ? 0xa : 0xc;
            }
            view->musicTrack = track;
        }
        break;
    }
    if (view->musicTrack >= 0) {
        int frames = Music_GetTrackSeconds(view->musicTrack) * 60;

        if (world->introTimer < frames) {
            world->introTimer = frames;
            world->introFrames = frames;
        }
    }
    if (g_RaceWorld.isReplay != 0) {
        view->musicTrack = -1;
    }
}

// Starts or stops the requested track, moves on to the next one when it has ended, and
// pauses the CD while the pause menu is open.
void RaceMusic_Update(RaceViewLoop *view)
{
    int playing = view->musicPlaying;
    int track = view->musicTrack;
    int next = view->musicNext;

    if (view->pausedBy == 0) {
        if (playing != track) {
            view->musicPlaying = track;
            if (track >= 0) {
                Music_PlayTrack(track, view->musicLoop != 0);
            } else {
                Music_Stop();
            }
        } else {
            int ended = 1;

            if (playing >= 0) {
                unsigned char *cd = (unsigned char *)&g_CdAudio;

                if (cd[0x164] == 0) {
                    if (cd[0x166] == 0) {
                        CdAudio_RunScript(6);
                    }
                    ended = 0;
                }
            }
            if (ended != 0 && next >= 0) {
                view->musicNext = -1;
                view->musicTrack = next;
                view->musicLoop = 1;
            }
        }
    } else if (((unsigned char *)&g_CdAudio)[0x163] != 0) {
        CdAudio_RunScript(2);
    }
}

void RaceMusic_Request(void *view, int track, int loop)
{
    *(char *)((int)view + 0x2ee) = (char)track;
    *(char *)((int)view + 0x2ed) = (char)loop;
}

// Requests the random race track (+0x2ef), looped. Called at the start signal.
void RaceMusic_PlayRaceTrack(void *view)
{
    RaceMusic_Request(view, (unsigned int)*(unsigned char *)((int)view + 0x2ef), 1);
}

// At the finish, plays the track for what was raced.
void RaceMusic_PlayFinish(RaceViewLoop *view)
{
    RaceMem *race = &g_Race_IsDemo;
    RaceWorld *world = &race->world;

    if (world->finishFrames == 0) {
        int track = -1;
        int loop = 0;

        world->finishFrames = RaceSetup_Get()->frameStep;
        if (world->isReplay == 0) {
            switch (view->musicKind) {
            case 1:
                track = 0x10;
                break;
            case 3:
                track = 0xd;
                break;
            case 4:
                track = 0xe;
                break;
            default:
                track = 0xe;
                break;
            }
            int type = RaceSetup_Get()->raceType;

            if (type != 0 && type != 3) {
                view->musicNext = 8;
            }
        }
        if (track >= 0 || loop != 0) {
            RaceMusic_Request(view, track, 0);
        }
    }
}
}
