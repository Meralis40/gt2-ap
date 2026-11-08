// OVL0 800292a0-800299d8 (raceview): Race view.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "gptr.h"
#include "common.h"
#include "gamestate.h"

// g_RaceSetup, as the game reaches it: g_GameState plus 0xbf7c, added at run time.
static inline RaceSetup *RaceSetup_Get(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;

    return (RaceSetup *)(base + offset);
}

static inline void Translate(MATRIX *m, int x, int y, int z)
{
    VECTOR v;

    v.vx = x;
    v.vy = y;
    v.vz = z;
    Matrix_Translate(m, &v);
}

static inline void Camera_SetViewport(Camera *cam, int x, int y, int w, int h)
{
    cam->vpX = x;
    cam->vpY = y;
    cam->width = w;
    cam->height = h;
}

// The two half-screen areas, copied whole.
struct SplitRects {
    RECT r[2];
};

struct Int3 {
    int v[3];
};

extern "C" {
extern RaceMem (&g_Race_IsDemo);
extern SplitRects (&g_SplitViewRects);
void PauseMenu_Draw(RaceViewLoop *view);
void Hud_DrawReplayLabel(RaceViewLoop *view, int car);
void Hud_DrawFull(RaceViewLoop *view, RaceVehicle *car, RaceCam *cam);
void Hud_DrawSplit(RaceViewLoop *view, RaceVehicle *car, RaceCam *cam);
void RaceWorld_DrawCars(RaceWorld *world, RaceCam *cam, int mirror);
void Sky_Draw(void *ot, Camera *cam, void *sky, int mirror);
unsigned int Course_Draw(Camera *cam, CourseData *course, int mirror);
void CourseObjects_DrawGroups(Camera *cam, CourseData *course, unsigned int mask);
void RaceView_DrawSplit(RaceViewLoop *view, int skip);
void RaceView_SetSplitArea(RaceViewLoop *view, int idx);
void RaceView_UpdateCamera(RaceViewLoop *view, RaceCam *cam, int mirror);
void RaceView_DrawWorld(RaceViewLoop *view, RaceCam *cam, int mirror);

void RaceView_DrawFrame(RaceViewLoop *view)
{
    int wasTwo = view->twoPlayer != 0;
    int two;
    int front;

    if (wasTwo == 0) {
        if (RaceSetup_Get()->raceType == 0) {
            int split = view->cams[0].splitScreen != 0 || view->cams[1].splitScreen != 0;

            if (split) {
                RaceCam *cam0 = &view->cams[0];
                RaceCam *cam1;

                cam0->targetCar = 0;
                cam0->splitScreen = 1;
                cam0->replayMode = 0;
                cam1 = &view->cams[1];
                cam1->targetCar = 1;
                cam1->splitScreen = 1;
                cam1->replayMode = 0;
                view->twoPlayer = 1;
            }
        }
    } else {
        int full = view->cams[0].splitScreen == 0 || view->cams[1].splitScreen == 0;

        if (full) {
            RaceCam *cam0 = &view->cams[0];
            RaceCam *cam1;

            cam0->splitScreen = 0;
            cam0->replayMode = 2;
            cam1 = &view->cams[1];
            cam1->splitScreen = 0;
            cam1->replayMode = 2;
            view->twoPlayer = 0;
        }
    }
    two = view->twoPlayer != 0;
    front = two;
    if (wasTwo != 0) {
        RaceView_DrawSplit(view, (two == 0) << 1);
    }
    if (two == 0) {
        RaceView_DrawSingle(view);
    }
    view->drawFront = front;
}

void RaceView_DrawHud(RaceViewLoop *view)
{
    if (view->pausedBy != 0) {
        PauseMenu_Draw(view);
    }
    if (RaceSetup_Get()->raceType != 0) {
        RaceCam *cam = &view->cams[0];

        Hud_DrawReplayLabel(view, cam->targetCar);
        if (cam->targetCar == 0) {
            RaceMem *race = &g_Race_IsDemo;

            Hud_DrawFull(view, &race->world.vehicles[0], cam);
        }
    } else if (view->twoPlayer != 0) {
        RaceMem *race = &g_Race_IsDemo;

        Hud_DrawSplit(view, &race->world.vehicles[0], &view->cams[0]);
        Hud_DrawSplit(view, &race->world.vehicles[1], &view->cams[1]);
    } else {
        RaceCam *cam = &view->cams[0];
        RaceMem *race = &g_Race_IsDemo;

        Hud_DrawFull(view, &race->world.vehicles[cam->targetCar], cam);
    }
}

void RaceView_RenderView(RaceViewLoop *view, int idx)
{
    RaceCam *cam = &view->cams[idx];
    int mirror = 0;

    if (cam->splitScreen == 0 && cam->rearMirror != 0) {
        RaceSetup *setup = RaceSetup_Get();

        if (setup->raceType != 0) {
            int step = setup->frameStep;

            mirror = step >= 2;
        }
    }
    if (mirror != 0) {
        RaceCam copy = *cam;
        MATRIX *m = &copy.view;
        RaceMem *race;
        int w;
        int h;

        Translate(m, 0, 0x3333, 0x8000);
        Matrix_RotEuler(m, 0, -0x20, 0);
        Matrix_Scale(m, 0x1000, 0x1000, -0x1000);
        w = 0x78;
        h = 0x20;
        Camera_SetViewport(&copy.cam, 0, 0, w, h);
        Camera_SetFrustum(&copy.cam, -0x3c, 0x3c, 0x10, -0x10, w, 0x7fff);
        {
            RECT r;

            r.x = 100;
            r.y = 20;
            r.w = w;
            r.h = h;
            DrawLayer_SetArea(&view->layers[1], &r);
            r.x = r.y = 0;
            Prim_AddRectOutline(view->layers[1].ot, 0, &r);
            r.x = r.y = 1;
            r.w -= 2;
            r.h -= 2;
            Prim_AddRectOutline(view->layers[1].ot, 0, &r);
        }
        RaceView_UpdateCamera(view, &copy, 1);
        race = &g_Race_IsDemo;
        RaceWorld_DrawCars(&race->world, &copy, 1);
        RaceView_DrawWorld(view, &copy, 1);
    }
    RaceView_UpdateCamera(view, cam, 0);
    {
        RaceMem *race = &g_Race_IsDemo;

        RaceWorld_DrawCars(&race->world, cam, 0);
    }
    RaceView_DrawWorld(view, cam, 0);
}

// RaceView_DrawHud, then RaceView_RenderView(0).
void RaceView_DrawSingle(RaceViewLoop *view)
{
    RaceView_DrawHud(view);
    RaceView_RenderView(view, 0);
}

void RaceView_SetSplitArea(RaceViewLoop *view, int idx)
{
    SplitRects rects = g_SplitViewRects;
    RECT *area = (RECT *)rt::from_guest((idx * 8 + (int)rt::to_guest(&rects)));

    DrawLayer_SetArea(&view->layers[2], area);
    DrawLayer_SetArea(&view->layers[3], area);
}

void RaceView_DrawSplit(RaceViewLoop *view, int skip)
{
    if ((skip & 1) == 0) {
        RaceView_DrawHud(view);
        RaceView_SetSplitArea(view, 0);
        RaceView_RenderView(view, 0);
        view->v_EndDraw();
        Gpu_DrawSync();
        view->drawFront = 0;
        view->v_BeginDraw();
        view->drawFront = 1;
    }
    if ((skip & 2) == 0) {
        RaceMem *race = &g_Race_IsDemo;
        RaceVehicle *car = &race->world.vehicles[0];
        Int3 relPos = *(Int3 *)race->world.vehicles[0].relPos;
        int level = car->soundLevel;

        RaceView_SetSplitArea(view, 1);
        RaceView_RenderView(view, 1);
        *(Int3 *)race->world.vehicles[0].relPos = relPos;
        car->soundLevel = level;
    }
}

void RaceView_UpdateCamera(RaceViewLoop *view, RaceCam *cam, int mirror)
{
    MATRIX *m = &cam->view;
    void *ot;

    if (mirror == 0) {
        ot = view->layers[2].ot;
    } else {
        ot = view->layers[1].ot;
    }
    Camera_Update(&cam->cam, m, ot, 0x10);
}

void RaceView_DrawWorld(RaceViewLoop *view, RaceCam *cam, int mirror)
{
    RaceMem *race = &g_Race_IsDemo;
    void *sky = (char *)race + 0x5d34;
    void *ot;
    unsigned int mask;

    if (mirror == 0) {
        ot = view->layers[3].ot;
    } else {
        ot = (char *)view->layers[1].ot + 0x4178;
    }
    Sky_Draw(ot, &cam->cam, sky, mirror);
    {
        char *base = (char *)&g_Race_IsDemo;
        int offset = 0xb534;
        CourseData *course = (CourseData *)(base + offset);

        mask = Course_Draw(&cam->cam, course, mirror);
        if (mirror == 0) {
            CourseObjects_DrawGroups(&cam->cam, course, mask);
        }
    }
}
}
