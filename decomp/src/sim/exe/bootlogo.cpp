// The boot splash screens (boot code, overwritten by the first overlay):
// VendorBootLogo shows one TIM for a number of frames, fading in and out, and
// Boot_Init runs the startup sequence. VendorBootLogo's vtable and type name
// (.rodata, after Boot_Init's string), __tf function and typeinfo are emitted
// here.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {
extern SplashScreen g_SplashScreens[];
extern RECT D_80033DB8;         // the tile under the image
extern RECT D_80033DC0;         // CLUT position
extern RECT D_80033DC8;         // image position
extern unsigned char g_VolDir[];
extern unsigned char g_MenuContext[];

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
// Not matching: loop strength reduction gives the byte fields their own
// pointer (s + 7), so the high half of D_80033DC0 isn't kept in a register as
// in the original. The buf local (g_VolDir in a register for PacketBuffer_Init
// and Tim_LoadClutAndImage, the symbol for Gzip_Inflate) and the local at the
// top of the loop (it keeps the test at the top) are right.
#ifdef NON_MATCHING
extern "C" void Boot_ShowSplashScreens()
{
    SplashScreen *s;
    unsigned char *buf = g_VolDir;

    Boot_ClearVram();
    VendorBootLogo logo;
    logo.SetVideoMode(0x27);
    PacketBuffer_Init(&logo.packets, buf, 0xc0000);
    ViewDrawContext_Init(&logo.draw, g_MenuContext, 0x100);
    logo.InitLayer();
    RECT *image = &D_80033DC8;
    int width = 0xa0;

    for (s = g_SplashScreens; s->timGz != 0; s++) {
        int shift = s->hiRes;

        image->w = width << shift;
        Gzip_Inflate(s->timGz, g_VolDir);
        Tim_LoadClutAndImage(buf, &D_80033DC0, image);
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
