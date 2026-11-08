// The movie player of OVL5 (docs/arcade.md): CD stream, MDEC output and the
// psxMovieLoop screen loop.
#ifndef MOVIE_H
#define MOVIE_H

#include "gptr.h"
#include "vtable.h"

#include "resident.h"

// Ring of frames read from the CD (0x1c bytes).
struct MovieStream {
    unsigned int flags;        // 1: kept on start, 2: dropping a frame, 4: ended, 8: sector deferred
    short readIdx;
    short writeIdx;
    short queued;
    short ringSize;
    game_pointer<game_pointer<unsigned char>> ring;
    game_pointer<unsigned char> dst;
    unsigned int frameNo;
    unsigned int sectorIdx;
};

// MDEC output, one 16-pixel column at a time (0x18 bytes).
struct MdecOut {
    RECT rect;
    unsigned char state;       // 0 busy, 1 last column out, 2 idle, 3 stopped
    unsigned char rgb24;
    unsigned short sliceWords;
    short sliceCount;
    short slicesLeft;
    game_pointer<unsigned int> slice[2];
};

struct MoviePlayer {
    game_function<void()> savedGpuDmaHook;
    game_function<void()> onFrameDone;
    MovieStream stream;
    MdecOut mdec;
    short dstX;
    short dstY;
    short frameW;
    short frameH;
    game_pointer<void> vlcNext;
    game_pointer<void> vlcOther;
    game_pointer<void> vlcReady;
};

// The movie screen loop of OVL5 (movieloop.cpp, which emits its vtable).
class psxMovieLoop : public psxViewLoop {
public:
    PadDevice pad;
    PadState padState;
    int noSkip;
    int rgb24;
    MoviePlayer player;
    int movieIndex;
    int startDelay;            // frames to queue before the first decode
    int framesReady;           // decoded frames waiting for display
    int decodeBuf;
    int vsyncTick;
    game_pointer<short> dispStarts;         // display buffer positions (x, y)
    int dispOffX;
    int dispOffY;

    psxMovieLoop(int movie, int triple);
    ~psxMovieLoop();
    void Enter();
    void Flip();
    void Leave();
    int Update();
    void BeginDraw();
    void Draw();
    void EndDraw();
    void UpdateDisplay();
    void Virtual15();
    int RestartRead();
    void OnFrameDecoded();
    // Calls through the vtable word (rt/vtable.h).
    void v_Virtual15() { return vptr.slot<void(ScreenViewLoop *)>(14)(this); }
    int v_RestartRead() { return vptr.slot<int(ScreenViewLoop *)>(15)(this); }
    void v_OnFrameDecoded() { return vptr.slot<void(ScreenViewLoop *)>(16)(this); }
};

// Header of a movie sector (the first 8 words, read before the 0x1f8 words of
// frame data).
struct MovieSectorHeader {
    unsigned short status;      // 0x160
    unsigned short type;        // 0x5349
    unsigned short sector;      // sector of the frame
    unsigned short sectorCount; // sectors in the frame
    unsigned int frameNo;
    unsigned int pad0c;
    unsigned int lastFrame;     // low 30 bits
    unsigned int pad14[3];
};

// OVL5's .bss (at 0x800A8D5C, tools/build.py): g++ puts zero-initialized
// globals in .data, so the section is given explicitly.
#define OVL5_BSS __attribute__((section(".bss")))

// Functions are extern "C" while their callers are assembly (common.h).
extern "C" {
// OVL5 data (g_Movie in movieset.cpp, the buffers in movieloop.cpp, the
// tables in .rodata).
extern MoviePlayer *g_Movie;
extern unsigned char g_MovieFrames[32][0x46e0];
extern unsigned char g_MovieVlcBufA[];
extern unsigned char g_MovieVlcBufB[];
extern unsigned char g_MovieSliceBufA[];
extern unsigned char g_MovieSliceBufB[];
extern game_pointer<unsigned char> g_MovieRing[32];
extern short (&D_800120CC)[];  // display buffers, double buffered
extern short (&D_800120D4)[];  // display buffers, triple buffered
// VLC table, 0x100 bytes before its start (0x800115CC): one row of 64 words
// per leading-zero count from 1.
extern unsigned int (&D_800114CC)[];

// Resident data.
extern CdAudio (&g_CdAudio);
extern int (&g_MovieSectors)[];
extern int (&g_StreamLba);
extern PadHandler (&g_PadHandlers)[];

}

// Installs the GPU DMA end hook and returns the previous one.
static inline void (*Gfx_SetGpuDmaHook(void (*hook)()))()
{
    void (*old)() = g_GfxIrq.gpuDmaHook;

    g_GfxIrq.gpuDmaHook = hook;
    return old;
}

// Sets the XA file and channel of a CD read request.
static inline void CdReq_SetFilter(int file, int channel, CdReq *r)
{
    r->file = file;
    r->channel = channel;
}

#endif
