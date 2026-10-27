// The boot splash screens (boot code, overwritten by the first overlay):
// VendorBootLogo shows one TIM for a number of frames, fading in and out, and
// Boot_Init runs the startup sequence. VendorBootLogo's vtable and type name
// (.rodata, after Boot_Init's string), __tf function and typeinfo are emitted
// here.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "viewloop.h"

// A splash screen of g_SplashScreens, which ends with a null timGz.
struct SplashScreen {
    void *timGz;                // gzip-compressed TIM
    unsigned short frames;
    unsigned char fadeOut;
    unsigned char hiRes;        // 640 pixels wide
};

// The part of the textured quad Prim_AddTexQuad returns that is set here.
struct TexQuad {
    short x;
    short unk02[3];
    short texX;
    short y;
    unsigned char u;
    unsigned char v;
    unsigned short clut;
    short w;
    short h;
};

// One draw layer of a view (ViewDrawContext_AddLayer).
struct DrawLayer {
    unsigned short priority;
    unsigned short otSize;
    DrawLayer *next;
    void *ot;
    void *header;
};

class VendorBootLogo : public psxViewLoopDoubleBuffer {
public:
    DrawLayer layer;
    int frame;
    int duration;               // frames to show
    int fadeOut;                // fade out over the last 16 frames
    int yOffset;
    int hiRes;

    VendorBootLogo();
    virtual ~VendorBootLogo();
    virtual int Update();
    virtual void Draw();
    void InitLayer();
};

extern "C" {
extern SplashScreen g_SplashScreens[];
extern RECT D_80033DB8;         // the tile under the image
extern RECT D_80033DC0;         // CLUT position
extern RECT D_80033DC8;         // image position
extern unsigned char g_VolDir[];
extern unsigned char g_MenuContext[];

void *memset(void *dst, int c, unsigned int n);
TexQuad *Prim_AddTexQuad(void *ot, unsigned int rgb);
RECT *Prim_AddTile(void *ot, unsigned int rgb);
void ViewDrawContext_Init(ViewDrawContext *ctx, void *ot, int size);
void ViewDrawContext_Reset(ViewDrawContext *ctx);
void ViewDrawContext_AddLayer(ViewDrawContext *ctx, DrawLayer *layer);
void PacketBuffer_Init(PacketBuffer *buf, void *base, unsigned int size);
void Gpu_TransferImage(RECT *rect, void *src, int arg);
void Gpu_DrawSync();
void Gzip_Inflate(void *src, void *dst);
void Tim_LoadClutAndImage(void *tim, RECT *clut, RECT *image);
void Irq_Install();
void Sound_Init();
void Sound_SetTickRate(int rate);
void Seq_Init();
void Seq_SetTickRate(int rate);
void Spu_SetReverbType(int type);
void Spu_EnableReverb(int on);
void Spu_SetReverbDepth(int left, int right);
void Sound_InstallVSync();
void CdAudio_Init();
void Pad_Install();
void Card_Init();
void Font_Init();
void Disc_LocateDataFiles(char *dir);
void Vol_Init();
void Overlay_Init();
void Game_InitData();
void EnableDisplay(unsigned int arg);
void Boot_ShowSplashScreens();
}

VendorBootLogo::VendorBootLogo()
{
    yOffset = 16;
}

VendorBootLogo::~VendorBootLogo()
{
}

void VendorBootLogo::InitLayer()
{
    layer.priority = 0;
    layer.otSize = 1;
    ViewDrawContext_Reset(&draw);
    ViewDrawContext_AddLayer(&draw, &layer);
}

int VendorBootLogo::Update()
{
    return frame++ < duration;
}

// The image as six 256-pixel-wide quads (two rows of three), at a brightness
// that rises by 8 per frame up to 0x60 and falls the same way at the end.
void VendorBootLogo::Draw()
{
    int left = duration - frame - 2;
    int level = frame;
    int i;
    unsigned int rgb;
    int clut;
    int width;

    if (left < 0) {
        left = 0;
    }
    if (level < 15) {
        level *= 8;
    } else {
        level = 0x80;
        if (fadeOut != 0 && left < 15) {
            level = left * 8;
        }
    }
    if (level >= 0x60) {
        level = 0x60;
    }
    rgb = level | level << 8 | level << 16;
    i = 0;
    clut = 0x7ff0;
    width = 0x100;
    for (; i < 6; i++) {
        TexQuad *q = Prim_AddTexQuad(layer.ot, rgb);
        int texX = i << 8;
        short y = 0;
        int h = 0x100;
        int x = (i << hiRes) + 10;

        if (i >= 3) {
            x = ((i - 3) << hiRes) + 26;
            texX = (i - 3) << 8;
            y = 0x100;
            h = 0xff;
        }
        q->x = hiRes != 0 ? x | 0x80 : x;
        q->texX = texX;
        y -= yOffset;
        q->v = 0;
        q->u = 0;
        q->clut = clut;
        q->w = width;
        q->h = h;
        q->y = y;
    }
    *Prim_AddTile(layer.ot, 0) = D_80033DB8;
}

// Clears VRAM to black, 16 lines at a time.
extern "C" void Boot_ClearVram()
{
    RECT rect;
    unsigned int black[0x2000];
    int y;

    memset(black, 0, sizeof(black));
    rect.w = 0x400;
    rect.x = 0;
    rect.y = 0;
    rect.h = 16;
    for (y = 0; y < 0x200; y += 16) {
        int line = y;   // a local in the body: g++ then keeps the test at the top

        rect.y = line;
        Gpu_TransferImage(&rect, black, 0);
        Gpu_DrawSync();
    }
}

// A VendorBootLogo loop for each splash screen. The TIMs are inflated into
// g_VolDir, unused this early.
// Not matching: the original keeps the loop's test at the top and holds the
// high halves of g_VolDir and D_80033DC0 in registers across it; no source
// shape tried gives both.
#ifdef NON_MATCHING
extern "C" void Boot_ShowSplashScreens()
{
    SplashScreen *s;

    Boot_ClearVram();
    VendorBootLogo logo;
    logo.SetVideoMode(0x27);
    PacketBuffer_Init(&logo.packets, g_VolDir, 0xc0000);
    ViewDrawContext_Init(&logo.draw, g_MenuContext, 0x100);
    logo.InitLayer();
    for (s = g_SplashScreens; s->timGz != 0; s++) {
        D_80033DC8.w = 0xa0 << s->hiRes;
        Gzip_Inflate(s->timGz, g_VolDir);
        Tim_LoadClutAndImage(g_VolDir, &D_80033DC0, &D_80033DC8);
        logo.frame = 0;
        logo.duration = s->frames;
        logo.fadeOut = s->fadeOut;
        logo.hiRes = s->hiRes != 0;
        logo.Execute();
    }
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/bootlogo", Boot_ShowSplashScreens);
#endif

extern "C" void Boot_Init()
{
    Irq_Install();
    Sound_Init();
    Sound_SetTickRate(60);
    Seq_Init();
    Seq_SetTickRate(60);
    Spu_SetReverbType(4);
    Spu_EnableReverb(1);
    Sound_InstallVSync();
    CdAudio_Init();
    Pad_Install();
    Card_Init();
    Font_Init();
    Boot_ShowSplashScreens();
    Disc_LocateDataFiles("/");
    Vol_Init();
    Overlay_Init();
    Game_InitData();
    Spu_SetReverbDepth(0xfff, 0xfff);
    EnableDisplay(1);
}
