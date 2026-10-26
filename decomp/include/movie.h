// The movie player of OVL5 (docs/arcade.md): CD stream, MDEC output and the
// psxMovieLoop screen loop.
#ifndef MOVIE_H
#define MOVIE_H

struct RECT {
    short x, y, w, h;
};

// Ring of frames read from the CD (0x1c bytes).
struct MovieStream {
    unsigned int flags;        // 1: kept on start, 2: dropping a frame, 4: ended, 8: sector deferred
    short readIdx;
    short writeIdx;
    short queued;
    short ringSize;
    unsigned char **ring;
    unsigned char *dst;
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
    unsigned long *slice[2];
};

struct MoviePlayer {
    void (*savedGpuDmaHook)();
    void (*onFrameDone)();
    MovieStream stream;
    MdecOut mdec;
    short dstX;
    short dstY;
    short frameW;
    short frameH;
    void *vlcNext;
    void *vlcOther;
    void *vlcReady;
};

// Resident screen loops (EXE, still assembly). ScreenViewLoop holds only the
// vtable pointer; Execute runs Enter, then Step until it returns 0, then
// Leave. psxViewLoop (0x6c bytes) adds the frame, draw and display steps. The
// virtual functions are declared in their vtable order.
class ScreenViewLoop {
public:
    ScreenViewLoop();
    virtual ~ScreenViewLoop();
    virtual int Run();
    virtual void Enter();
    virtual int Step();
    virtual void Flip();
    virtual void Leave();
    int Execute();
};

class psxViewLoop : public ScreenViewLoop {
public:
    unsigned char pad04[0x14];
    int vsyncInterval;
    unsigned char videoModeDelay;
    unsigned char bufferIndex;
    unsigned char bufferCount;
    unsigned char videoModePending;
    unsigned char pad20[0x4c];

    psxViewLoop();
    virtual ~psxViewLoop();
    virtual void Enter();
    virtual int Step();
    virtual void Flip();
    virtual void Leave();
    virtual void BeginFrame();
    virtual int Update();
    virtual void BeginDraw();
    virtual void Draw();
    virtual void EndDraw();
    virtual void DrawFrame();
    virtual void SwapDisplay();
    virtual void ApplyVideoMode();
    virtual void UpdateDisplay();
};

struct PadState {
    unsigned int held;
    unsigned int pressed;
    unsigned int released;
    unsigned int repeat;
};

struct PadDevice {
    unsigned char pad00[0xc];
    unsigned char buttons[0x3c];
    unsigned char pad48[0x1c];
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
    short *dispStarts;         // display buffer positions (x, y)
    int dispOffX;
    int dispOffY;

    psxMovieLoop(int movie, int triple);
    virtual ~psxMovieLoop();
    virtual void Enter();
    virtual void Flip();
    virtual void Leave();
    virtual int Update();
    virtual void BeginDraw();
    virtual void Draw();
    virtual void EndDraw();
    virtual void UpdateDisplay();
    virtual void Virtual15();
    virtual int RestartRead();
    virtual void OnFrameDecoded();
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

// A CD read request: sectors start..end, mode, XA file and channel.
struct CdReq {
    unsigned char flag;
    unsigned char mode;
    unsigned char file;
    unsigned char channel;
    unsigned int start;
    unsigned int end;
};

struct CdAudio {
    unsigned char pad00[0x18];
    unsigned short masterVol;
    unsigned char pad1a[6];
    CdReq req;
    int (*onSector)(void *, void *);
    unsigned char pad30[0x136];
    volatile unsigned char state;  // changed by the CD interrupt
};

struct GfxIrq {
    unsigned char pad00[0xc];
    void (*gpuDmaHook)();
};

#define PAD_START 0x10000

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
extern unsigned char *g_MovieRing[32];
extern short D_800120CC[];  // display buffers, double buffered
extern short D_800120D4[];  // display buffers, triple buffered
// VLC table, 0x100 bytes before its start (0x800115CC): one row of 64 words
// per leading-zero count from 1.
extern unsigned int D_800114CC[];

// Resident data.
extern CdAudio g_CdAudio;
extern GfxIrq g_GfxIrq;
extern int g_MovieSectors[];
extern int g_StreamLba;
extern unsigned char g_LastBufferIndex;
extern psxViewLoop *g_CurrentViewLoop;
extern void *g_PadHandlers[];

// Resident code.
int Sys_CriticalSection(int enter);
void CdAudio_ResetRead();
unsigned char *Lz_Decompress(unsigned char *dst, unsigned char *src, int size);
void func_8007CFD4(void *dst, int words);  // jumps to CdGetSector
int CdAudio_RunScript(int script);
void DecDCTReset(int mode);
void DecDCTin(unsigned long *buf, int mode);
void DecDCTout(unsigned long *buf, int size);
void DecDCToutCallback(void (*cb)());
void Gpu_TransferImage(RECT *rect, void *src, int arg);
void Gpu_DrawSync();
void Gpu_SetDisplayMode(int mode);
void Gpu_SetDisplayStart(int x, int y);
void EnableDisplay(int on);
int Irq_WaitVSyncs(int n);
void Music_Stop();
void PadDevice_Open(PadDevice *pad, int port, void **handlers);
void PadDevice_Close(PadDevice *pad);
void PadButtons_Read(void *buttons, PadState *state);
void Overlay_Start(int ovl);

// OVL5.
void Movie_Init(MoviePlayer *mp, int rgb24);
void Movie_Shutdown(MoviePlayer *mp);
void Movie_SetRing(MoviePlayer *mp, short count, unsigned char **ring);
void Movie_SetVlcBuffers(MoviePlayer *mp, void *a, void *b);
void Movie_SetSliceBuffers(MoviePlayer *mp, unsigned long *a, unsigned long *b);
void Movie_Open(MoviePlayer *mp, int movie);
void Movie_SetCdRange(MoviePlayer *mp, unsigned int lba, int sectors);
void Movie_Start(MoviePlayer *mp);
int Movie_GetStatus(MoviePlayer *mp);
void Movie_Stop(MoviePlayer *mp);
void Movie_RestartRead(MoviePlayer *mp);
int Movie_StartMdec(MoviePlayer *mp);
void *Movie_DecodeNext(MoviePlayer *mp, int x, int y);
void Movie_Step(MoviePlayer *mp, int x, int y);
void Movie_DecodeThenStart(MoviePlayer *mp, int x, int y);
int Movie_CdSectorCb(void *a, void *b);
int Movie_OnSector(MoviePlayer *mp, void *a, void *b);
void Movie_MdecOutCb();
void Movie_OnMdecSlice(MoviePlayer *mp);
void Movie_GpuDmaIrq();
void Movie_OnGpuDma(MoviePlayer *mp);
void MovieStream_Clear(MovieStream *s);
int MovieStream_Start(MovieStream *s);
void MovieStream_Stop(MovieStream *s);
int MovieStream_ReadSector(MovieStream *s);
unsigned char *MovieStream_PopFrame(MovieStream *s);
void MovieStream_ReleaseFrame(MovieStream *s);
void MdecOut_Init(MdecOut *m, int rgb24);
void MdecOut_SetSize(MdecOut *m, int w, int h);
void MdecOut_Start(MdecOut *m, int x, short y, unsigned long *vlc);
void MdecOut_Stop(MdecOut *m);
void Movie_DecodeVlc(unsigned short *frame, unsigned short *out);
void Movie_InitRing();
void Movie_DisplayOn();
void Movie_ClearVram();
void Movie_OnFrameDone();
void Movie_Play18();
void Movie_Play1A();
void Movie_Play19();
void Movie_DefaultEntry();
void Movie_ArcadeEntry(int which);
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
