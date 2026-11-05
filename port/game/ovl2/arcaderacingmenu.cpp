// OVL2 80014708-80015670 (arcaderacingmenu): ArcadeRacingMenu, the arcade race
// loading screen, with its car placement and body drawing. Its vtable and type
// name (.rodata), its __tf function and typeinfo (.bss, which build.py's
// CXX_NAMES places) are emitted here.
#include "common.h"

extern "C" {
extern ArcadeLoadInfo g_ArcadeLoadInfo;
extern char *g_PacketPtr;
extern char *g_PacketEnd;
extern char g_OVL2_Strings[];           // the arcade strings, inflated by ArcadeMode_Run
extern unsigned char g_OVL2_FileBuf[];
extern unsigned char g_ArcadeT002nModel[];
extern unsigned short D_800454D4[];     // [1] clut y and [4] texture page of the car views
extern unsigned char D_800274D4[];      // the car views' models
extern unsigned char D_800F8800[];
extern unsigned short D_8019C1E0[];     // the car texture, uploaded to 0x140, 0
extern Font D_80093130;                 // the resident font of the car list
extern Font D_80129820;
}

// A rounded box for Prim_AddRoundedBox: its colour and code, then its rectangle.
struct RoundedBox {
    unsigned int rgb;
    RECT rect;
};

// Draws only the body of inst (CarBody_DrawClipped after Clip_TestBox) with cam,
// through the scratchpad as CarModel_Draw does; no wheels or shadow. Used for the
// t002n model of the loading screen.
void ArcadeRacingMenu_DrawBody(Camera *cam, CarModelInst *inst)
{
    CarScratch *sp = (CarScratch *)0x1f800000;
    int light[6];               // the rotation part of a MATRIX: Matrix_Mul writes no translation
    MATRIX *world;
    CarModelHeader *model;
    LodTable *table;
    int detail;
    unsigned int brightness;
    unsigned int rgb;
    CarMesh *mesh;
    unsigned int unk02;
    short unk06;
    unsigned short tpage;

    *(CameraView *)sp = *(CameraView *)&cam->view;
    model = inst->model;
    detail = inst->detail;
    unk02 = inst->unk02;
    brightness = inst->brightness;
    unk06 = inst->unk06;
    tpage = inst->tpage;
    table = (LodTable *)&model->lodCount;
    sp->mirror = 0;
    sp->detail = detail << 24;
    sp->clutSelect = unk02 << 16;
    rgb = brightness | brightness << 8 | brightness << 16;
    sp->envFT3 = rgb | 0x26000000;
    sp->envFT4 = rgb | 0x2e000000;
    sp->envClut = unk06;
    sp->envTpage = tpage | 0x20;
    if (brightness) {
        sp->lit = -1;
    } else {
        sp->lit = 0;
    }
    world = &inst->world;
    Matrix_Comp(&sp->view, world);
    Matrix_Mul((MATRIX *)light, (MATRIX *)cam->invRot, world);
    mesh = table->lods[0].mesh;
    Gte_LoadTransform((ViewScratch *)sp, mesh->unk4e, mesh->unk4c, 0xc);
    sp->verts = mesh->verts;
    sp->normals = mesh->normals;
    sp->packetPtr = g_PacketPtr;
    sp->packetLimit = g_PacketEnd - 0x2000;
    if ((Clip_TestBox((ViewScratch *)sp, mesh->bboxMin, mesh->bboxMax) & 0x1f) == 0) {
        CarBody_DrawClipped(mesh, (MATRIX *)light);
    }
    g_PacketPtr = sp->packetPtr;
}

ArcadeRacingMenu::ArcadeRacingMenu()
{
}

ArcadeRacingMenu::~ArcadeRacingMenu()
{
}

// Packets in OVL2's file buffer, 4 layers, the race font, the t002n model, the
// car texture and a CLUT; race type 6 uploads its TIM. Then each car view's
// camera from its seed, and the 16-frame counter.
void ArcadeRacingMenu::Enter()
{
    RECT r;
    RECT *rp = &r;
    unsigned short clut[256];
    void *tim;
    TimBlock *b;

    psxViewLoop::Enter();
    SetVideoMode(1);
    PacketBuffer_Init(&packets, g_OVL2_FileBuf, 0x66000);
    ViewDrawContext_Init(&draw, D_800F8800, 0x2b000);
    layers[2].priority = 1;
    layers[2].otSize = 4;
    layers[0].priority = 2;
    layers[0].otSize = 0x10a0;
    layers[1].priority = 3;
    layers[1].otSize = 0x10a0;
    layers[3].priority = 4;
    layers[3].otSize = 1;
    ViewDrawContext_Reset(&draw);
    ViewDrawContext_AddLayer(&draw, &layers[2]);
    ViewDrawContext_AddLayer(&draw, &layers[0]);
    ViewDrawContext_AddLayer(&draw, &layers[1]);
    ViewDrawContext_AddLayer(&draw, &layers[3]);
    RaceFont_Load(0x1c);
    CarModel_Init((CarModelHeader *)g_ArcadeT002nModel, 5);
    r.x = 0x140;
    r.w = 0x40;
    r.y = 0;
    r.h = 0xe0;
    Gpu_TransferImage(rp, (unsigned int *)D_8019C1E0, 0);
    Gpu_DrawSync();
    Clut_Blend(clut, D_8019C1E0 - 0x21c0, 0, 0);
    Clut_UploadToSlot(clut, 5, 0);
    if (g_ArcadeLoadInfo.raceType == 6) {
        tim = g_ArcadeLoadInfo.tim;
        b = (TimBlock *)((char *)tim + 8);
        r.x = 0x1c0;
        r.y = 0x100;
        r.w = b->w;
        r.h = b->h;
        Tim_LoadClut(tim, rp);
        b = (TimBlock *)((char *)b + ((TimBlock *)((char *)tim + 8))->len);
        r.x = 0x1c0;
        r.y = 0x101;
        r.w = b->w;
        r.h = b->h;
        Tim_LoadImage(tim, rp);
    }
    yaws[0] = g_ArcadeLoadInfo.seeds[0] & 0xfff;
    rolls[0] = (g_ArcadeLoadInfo.seeds[0] >> 4 & 0x1ff) - 0x100;
    presets[0] = g_ArcadeLoadInfo.seeds[0] & 3;
    yaws[1] = g_ArcadeLoadInfo.seeds[1] & 0xfff;
    rolls[1] = (g_ArcadeLoadInfo.seeds[1] >> 4 & 0x1ff) - 0x100;
    presets[1] = g_ArcadeLoadInfo.seeds[1] & 3;
    frames = 16;
    textDelay = 2;
}

void ArcadeRacingMenu::Leave()
{
    psxViewLoopDoubleBuffer::Leave();
}

// Counts down; returns 0 (leave) once the counter is below 0 on buffer 1.
int ArcadeRacingMenu::Update()
{
    psxViewLoop::Update();
    textDelay = 3;
    if (--frames < 0 && bufferIndex == 1) {
        return 0;
    }
    return 1;
}

// Places a car view's car: one of 4 presets (distance and pitch) with the yaw
// and roll of its seed, then the frustum (Camera_SetFrustum).
void ArcadeRacingMenu_SetCamera(Camera *cam, MATRIX *world, int preset, int yaw, int roll, int halfW, int halfH)
{
    VECTOR v;
    int a;

    Matrix_Identity(world);
    halfH = halfH * 110 / 100;
    a = (yaw > 0x800 ? 0xc00 : 0x400) - yaw;
    roll = roll * abs(a) / 1024;
    if (yaw > 0x800) {
        a = (yaw - 0x800) * 0x6d4 / 2048 + 0x896;
    } else {
        a = yaw * 0x6d4 / 2048 + 0x96;
    }
    yaw = a;
    switch (preset) {
    case 0:
        Matrix_RotZ(world, roll);
        v.vx = 0;
        v.vy = 0;
        v.vz = -0x50000;
        Matrix_Translate(world, &v);
        Matrix_RotX(world, 0x96);
        Matrix_RotY(world, yaw);
        Camera_SetFrustum(cam, -halfW, halfW, halfH, -halfH, 300, 0x7fff);
        break;
    case 1:
        Matrix_RotZ(world, roll);
        v.vx = 0x28f;
        v.vy = -0x1999;
        v.vz = -0x60000;
        Matrix_Translate(world, &v);
        Matrix_RotX(world, 0x14);
        Matrix_RotY(world, yaw);
        Camera_SetFrustum(cam, -halfW, halfW, halfH, -halfH, 300, 0x7fff);
        break;
    case 2:
        Matrix_RotZ(world, roll);
        v.vx = -0x4f5c;
        v.vy = -0x7333;
        v.vz = -0x50000;
        Matrix_Translate(world, &v);
        Matrix_RotY(world, yaw);
        Camera_SetFrustum(cam, -halfW, halfW, halfH, -halfH, 300, 0x7fff);
        break;
    case 3:
        Matrix_RotZ(world, roll);
        v.vx = 0;
        v.vy = 0x3333;
        v.vz = -0x40000;
        Matrix_Translate(world, &v);
        Matrix_RotX(world, 0x96);
        Matrix_RotY(world, yaw);
        Camera_SetFrustum(cam, -halfW, halfW, halfH, -halfH, 200, 0x7fff);
        break;
    }
}

// From frame 12: race type 4's car list (number, name, colour), the title, and
// the one or two car views (race type 0 splits the screen), each car with the
// t002n body; the background tile.
// The game's code up to the register allocation: the game spills more of the list
// loop's positions and constants (a 0x378-byte frame for GCC's 0x370) and its
// function is 0x3c bytes longer.
void ArcadeRacingMenu::Draw()
{
    TextCtx ctx;
    TextCtx *t;
    RECT area;
    RoundedBox box;
    char num[64];
    GouraudQuad q;
    Camera cam;
    MATRIX view;
    CarModelInst insts[2];
    CarModelInst body;
    VECTOR v;
    void *ot;
    char *s;
    RECT *r;
    unsigned int width;
    unsigned int height;
    int scale;
    int count;
    int halfW;
    int halfH;
    int numX;
    int nameX;
    int y;
    int qx;
    int qy;
    int boxX;
    int boxY;
    int i;

    psxViewLoop::Draw();
    if (frames <= 12) {
        ot = layers[2].ot;
        textDelay--;
        Text_Init(&ctx, 0x1c);
        t = &ctx;
        t->ot = (unsigned int *)ot;
        if (textDelay == 0 && g_ArcadeLoadInfo.raceType == 4) {
            Text_SetFont(t, &D_80093130);
            t->rgb = 0x1e6fde;
            numX = 0x3c;
            nameX = 0x4c;
            y = 0x4f;
            qx = 0x28;
            qy = 0x40;
            boxX = 0xbc;
            boxY = 0x48;
            for (i = 0; i < 6; i++) {
                char *name;

                num[0] = i + '1';
                num[1] = 0;
                t->rgb = 0x2004670;
                *(unsigned int *)&t->clut &= 0xff9fffff;
                Text_DrawString(t, (signed char *)num, numX, y, -1);
                t->rgb = 0x2808080;
                *(unsigned int *)&t->clut &= 0xff9fffff;
                name = CarInfoA_GetName(g_ArcadeLoadInfo.cars[i].carId);
                Text_DrawString(t, (signed char *)name, nameX, y, -1);
                q.rect.x = qx;
                q.rect.y = qy;
                q.rect.w = 0x10;
                q.rect.h = 0x10;
                q.top = g_ArcadeLoadInfo.cars[i].rgb;
                Menu_DrawColorSwatch((unsigned int *)ot, &q, 0x80);
                Prim_AddDrawMode(ot, 0x200);
                box.rect.w = 0x130;
                box.rect.h = 0x16;
                box.rgb = 0x2bf622c;
                box.rect.x = boxX;
                box.rect.y = boxY;
                Prim_AddRoundedBox(ot, &box, 0x20);
                Prim_AddDrawMode(ot, 0);
                numX += 4;
                nameX += 4;
                y += 0x18;
                qx += 4;
                qy += 0x18;
                boxX += 4;
                boxY += 0x18;
            }
        }
        r = Prim_AddTile(ot, 0x80);
        r->y = 0x32;
        r->w = 0x140;
        r->x = 0;
        r->h = 2;
        Text_Init(&ctx, 0x1e);
        Text_SetFont(&ctx, &D_80129820);
        ctx.rgb = 0xa376f;
        s = g_OVL2_Strings + 0x205;
        switch (g_ArcadeLoadInfo.raceType) {
        case 4:
            break;
        case 0:
            s = g_OVL2_Strings + 0x7d;
            break;
        case 6:
            s = g_OVL2_Strings + 0x218;
            if (g_ArcadeLoadInfo.unk08 != 0) {
                s = g_OVL2_Strings + 0x229;
            }
            break;
        }
        Text_DrawString(&ctx, (signed char *)s, 0x14, 0x30, 1);
        r = Prim_AddTile(ot, 0x2000000);
        r->w = 0x140;
        r->x = 0;
        r->y = 0;
        r->h = 0xf0;
        Prim_AddDrawMode(ot, 0);
        scale = 0x80;
        height = 0xf0;
        if (g_ArcadeLoadInfo.raceType == 0) {
            area.x = 0;
            area.y = 0x32;
            area.w = 0x140;
            area.h = 0x5b;
            DrawLayer_SetArea(&layers[0], &area);
            area.x = 0;
            area.y = 0x8d;
            area.w = 0x140;
            area.h = 0x5b;
            DrawLayer_SetArea(&layers[1], &area);
            scale = 0xa0;
            count = 2;
            height = 0x78;
        } else {
            area.x = 0;
            area.y = 0x32;
            area.w = 0x140;
            area.h = height;
            DrawLayer_SetArea(&layers[0], &area);
            count = 1;
        }
        for (i = 0; i < 2; i++) {
            insts[i].lod = 1;
            insts[i].detail = 0;
            insts[i].unk02 = 0;
            insts[i].drawShadow = 1;
            insts[i].model = (CarModelHeader *)(D_800274D4 + i * 0x6000);
            insts[i].unk06 = *(unsigned int *)D_800454D4 >> 16;
            insts[i].brightness = 0x40;
            insts[i].tpage = D_800454D4[4];
        }
        width = 0x140;
        halfW = (width >> 1) * scale;
        body.detail = 0;
        body.unk02 = 0;
        body.drawShadow = 0;
        body.brightness = 0;
        halfH = (height >> 1) * scale;
        body.lod = 1;
        body.model = (CarModelHeader *)g_ArcadeT002nModel;
        body.unk06 = *(unsigned int *)D_800454D4 >> 16;
        body.tpage = D_800454D4[4];
        for (i = 0; i < count; i++) {
            void *carOt;

            cam.vpX = 0;
            cam.vpY = i * 0x78;
            cam.width = width;
            cam.height = height;
            ArcadeRacingMenu_SetCamera(&cam, &insts[i].world, presets[i], yaws[i], rolls[i], halfW / 128,
                                       halfH / 128);
            body.world = insts[i].world;
            carOt = layers[i].ot;
            Matrix_Identity(&view);
            Camera_Update(&cam, &view, carOt, 0x10);
            v.vx = 0;
            v.vy = CarModel_GetRideHeight(insts[i].model);
            v.vz = 0;
            Matrix_Translate(&insts[i].world, &v);
            CarInst_InitWheels(&insts[i]);
            insts[i].shadow = insts[i].world;
            v.vx = 0;
            v.vy = -CarModel_GetRideHeight(insts[i].model);
            v.vz = 0;
            Matrix_Translate(&insts[i].shadow, &v);
            CarModel_Draw(&cam, &insts[i], 0);
            Matrix_Identity(&view);
            Camera_Update(&cam, &view, (char *)carOt + 0x100, 0x10);
            ArcadeRacingMenu_DrawBody(&cam, &body);
        }
    }
    r = Prim_AddTile(layers[3].ot, 0x140a00);
    r->w = 0x140;
    r->x = 0;
    r->y = 0;
    r->h = 0xf0;
}
