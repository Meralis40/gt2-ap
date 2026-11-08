// psxMovieLoop, the movie screen loop: its members, the three movies and the
// overlay entries. Its vtable (.rodata), __tf function and typeinfo (.bss) are
// emitted here.
#include "gptr.h"
#include "common.h"
#include "rt.h"

// The overlay's buffers, in its .bss after g_Movie, then the typeinfo of
// psxMovieLoop.
unsigned char g_MovieFrames[32][0x46e0] OVL5_BSS;
unsigned char g_MovieVlcBufA[0x20000] OVL5_BSS;
unsigned char g_MovieVlcBufB[0x20000] OVL5_BSS;
unsigned char g_MovieSliceBufA[0x3000] OVL5_BSS;
unsigned char g_MovieSliceBufB[0x3000] OVL5_BSS;
game_pointer<unsigned char> g_MovieRing[32] OVL5_BSS;

// triple: triple buffered display (and two frames queued before the first
// decode).
psxMovieLoop::psxMovieLoop(int movie, int triple)
{
    vptr.address = 0x8001156c;  // psxMovieLoop's vtable
    PadDevice_Open(&pad, 0, g_PadHandlers);
    noSkip = 0;
    rgb24 = 1;
    movieIndex = movie;
    if (triple != 0) {
        startDelay = 2;
    } else {
        startDelay = 0;
    }
    g_LastBufferIndex = 0;
    bufferCount = triple != 0 ? 3 : 2;
    vsyncTick = 0;
    dispStarts = triple != 0 ? D_800120D4 : D_800120CC;
    dispOffX = 0;
    dispOffY = 0;
}

psxMovieLoop::~psxMovieLoop()
{
    PadDevice_Close(&pad);
}

void psxMovieLoop::Enter()
{
    MoviePlayer *mp;

    psxViewLoop::Enter();
    framesReady = 0;
    videoModePending = 1;
    bufferIndex = 0;
    decodeBuf = 0;
    Movie_InitRing();
    mp = &player;
    Movie_Init(mp, rgb24);
    mp->onFrameDone = Movie_OnFrameDone;
    Movie_SetRing(mp, 0x20, g_MovieRing);
    Movie_SetVlcBuffers(mp, g_MovieVlcBufA, g_MovieVlcBufB);
    Movie_SetSliceBuffers(mp, (unsigned int *)g_MovieSliceBufA, (unsigned int *)(g_MovieSliceBufA + 0x3000));
    Movie_Open(mp, movieIndex);
    Movie_Start(mp);
}

// One step of the screen loop: decodes the next frame when the display has a
// free buffer. Returns 0 to leave (START pressed, the stream ended, or the
// read can't restart), 1 after a frame was started.
int psxMovieLoop::Update()
{
    int start = Irq_WaitVSyncs(0);
    int now;
    int status;
    int busy;

    busy = 1;
    for (;;) {
        now = Irq_WaitVSyncs(0);
        PadButtons_Read(&pad.buttons, &padState);
        if (noSkip == 0 && (padState.pressed & PAD_START)) {
            while (Movie_GetStatus(&player) == busy) {
            }
            return 0;
        }
        status = Movie_GetStatus(&player);
        // Waiting for 900 vsyncs (15 seconds at 60 Hz): treat it as a
        // stopped CD.
        if (status == busy && now - start > 899) {
            start = now;
            status = 3;
        }
        if (status == 0) {
            if (startDelay != 0) {
                // The CD callback changes queued: read it as volatile.
                if (((volatile MovieStream *)&player.stream)->queued < startDelay) {
                    continue;
                }
                startDelay = 0;
            }
            if (framesReady < bufferCount - 1) {
                break;
            }
        }
        if (status == 2) {
            return 0;
        }
        // RestartRead returns 1 to keep playing. The original tests the
        // result with xori, which only `^ 1` gives.
        if (status == 3 && (v_RestartRead() ^ 1) != 0) {
            return 0;
        }
    }
    {
        short *pos = (short *)rt::from_guest((decodeBuf * 4 + (int)dispStarts));
        Movie_Step(&player, pos[0] + dispOffX, pos[1] + dispOffY);
    }
    if (++decodeBuf >= bufferCount) {
        decodeBuf = 0;
    }
    return 1;
}

void psxMovieLoop::Flip()
{
}

// Every vsyncInterval vblanks, if a decoded frame is waiting, shows the next
// display buffer, and turns the display on after the first frame.
void psxMovieLoop::UpdateDisplay()
{
    if (++vsyncTick >= vsyncInterval) {
        vsyncTick = 0;
        if (framesReady > 0) {
            // idx is set twice and added to the address as an integer, which
            // gives the original instruction order and registers.
            int idx = bufferIndex;
            short *pos;

            framesReady--;
            idx <<= 2;
            pos = (short *)rt::from_guest((idx + (int)dispStarts));
            Gpu_SetDisplayStart((pos[0] * 3) >> 1, pos[1]);
            if (++bufferIndex >= bufferCount) {
                bufferIndex = 0;
            }
            if (videoModePending != 0) {
                videoModePending = 0;
                EnableDisplay(0);
            }
        }
    }
}

void psxMovieLoop::Leave()
{
    Movie_Stop(&player);
    Movie_Shutdown(&player);
    psxViewLoop::Leave();
}

void psxMovieLoop::BeginDraw()
{
}

void psxMovieLoop::Draw()
{
}

void psxMovieLoop::EndDraw()
{
}

void psxMovieLoop::Virtual15()
{
}

int psxMovieLoop::RestartRead()
{
    Movie_RestartRead(&player);
    return 1;
}

void psxMovieLoop::OnFrameDecoded()
{
    framesReady++;
}

extern "C" {
void Movie_Play18()
{
    psxMovieLoop loop(0x18, 1);
    psxMovieLoop *p;

    Movie_ClearVram();
    Irq_WaitVSyncs(-1);
    Gpu_SetDisplayMode(0x31);
    Irq_WaitVSyncs(2);
    p = &loop;
    p->dispOffY = 0x18;
    p->dispOffX = 0;
    p->vsyncInterval = 2;
    p->Execute();
    Movie_DisplayOn();
}

void Movie_Play1A()
{
    psxMovieLoop loop(0x1a, 0);
    psxMovieLoop *p;

    Movie_ClearVram();
    Irq_WaitVSyncs(-1);
    Gpu_SetDisplayMode(0x13);
    Irq_WaitVSyncs(2);
    p = &loop;
    p->dispOffY = 8;
    p->vsyncInterval = 4;
    p->dispOffX = 0;
    p->noSkip = 1;
    p->Execute();
    Movie_DisplayOn();
}

void Movie_Play19()
{
    psxMovieLoop loop(0x19, 0);
    psxMovieLoop *p;

    Movie_ClearVram();
    Irq_WaitVSyncs(-1);
    Gpu_SetDisplayMode(0x33);
    Irq_WaitVSyncs(2);
    p = &loop;
    p->dispOffY = 0xc;
    p->vsyncInterval = 4;
    p->dispOffX = 0;
    p->noSkip = 1;
    p->Execute();
    Movie_DisplayOn();
}

// MDEC isn't written yet, so a movie leaves the screen as one would have and goes on.
static void Movie_Skip(int movie)
{
    Rt_Log("skipping movie %#x: MDEC isn't implemented", movie);
    Movie_ClearVram();
    Movie_DisplayOn();
}

void Movie_DefaultEntry()
{
    Movie_Skip(0x18);
    Overlay_Start(1);
}

void Movie_ArcadeEntry(int which)
{
    Movie_Skip(which == 0 ? 0x1a : 0x19);
    Overlay_Start(1);
}
}
