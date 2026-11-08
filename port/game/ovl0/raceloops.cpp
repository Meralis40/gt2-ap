// OVL0 800157a8-800163b8 (raceloops): the RaceViewLoop and RaceMenuLoop screen
// loops. Their vtables and type names (.rodata, after RaceMenuLoop::Run's jump
// table), their __tf functions and typeinfo (.bss, which build.py's CXX_NAMES
// places) are emitted here.
#include "common.h"
#include "gamestate.h"

// g_RaceSetup, as the game reaches it: g_GameState plus 0xbf7c, added at run time.
static inline RaceSetup *RaceSetup_Get(void)
{
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;

    return (RaceSetup *)(base + offset);
}

extern "C" {
extern RaceMem (&g_Race_IsDemo);
extern game_pointer<RaceViewLoop> (&g_RaceView);
extern unsigned int (&g_RandomSeed);
extern DrawLayer (&g_Race3DLayer);         // RaceView_DrawFrame's 3D layer
extern unsigned char (&g_Race_EndRequest);
extern int (&g_TT_GhostResync);

}

// A draw layer's priority and OT size.
static inline void DrawLayer_Set(DrawLayer *l, int priority, int otSize)
{
    l->priority = priority;
    l->otSize = otSize;
}

RaceViewLoop::RaceViewLoop()
{
    vptr.address = 0x8002ef98;  // RaceViewLoop's vtable
    int vsyncs;
    unsigned int seed;

    worldReady = 0;
    vsyncs = Irq_WaitVSyncs(0);
    seed = g_RandomSeed;
    unk2e7 = 0;
    randomSeed = vsyncs + seed;
}

RaceViewLoop::~RaceViewLoop()
{
}

// Runs the race (mode 0) or its replay (1) as g_RaceView.
void RaceViewLoop::Start(int mode)
{
    g_RaceView = this;
    replay = mode;
    Execute();
}

// The draw layers, the race world (once), the course and the cars, the intro
// length, the cameras, the HUD and the pads; two players split the screen.
// The game's code but for one andi: the game zero-extends the intro length again
// before it stores introTimer.
void RaceViewLoop::Enter()
{
    RaceMem *race = &g_Race_IsDemo;
    RaceWorld *world;
    RaceSetup *setup;
    char *base;
    int offset;
    unsigned short otSize;
    int intro;
    DrawLayer *l0;
    DrawLayer *l1;
    DrawLayer *l2;
    DrawLayer *l3;
    RaceCam *cam0;
    RaceCam *cam1;

    psxViewLoop::Enter();
    SetVideoMode(1);
    RaceMem_InitDrawBuffers(race, this);
    otSize = 0x10a0;
    layer0.priority = 0x100;
    layer0.otSize = 1;
    l0 = &layers[0];
    DrawLayer_Set(l0, 0x200, 3);
    base = (char *)&g_GameState;
    offset = 0xbf7c;
    setup = (RaceSetup *)(base + offset);
    world = &race->world;
    if (setup->raceType == 0) {
        otSize = 1;
    }
    l1 = &layers[1];
    DrawLayer_Set(l1, 0x300, otSize);
    l2 = &layers[2];
    DrawLayer_Set(l2, 0x400, 0x10a0);
    l3 = &layers[3];
    DrawLayer_Set(l3, 0x500, 2);
    ViewDrawContext_AddLayer(&draw, l0);
    ViewDrawContext_AddLayer(&draw, l1);
    ViewDrawContext_AddLayer(&draw, l2);
    ViewDrawContext_AddLayer(&draw, l3);
    world->isReplay = replay != 0;
    if (worldReady == 0) {
        worldReady = 1;
        RaceWorld_Init(world, replay != 0);
    }
    Race_SetCourseIndex(race);
    RaceLoad_InstallVSync();
    intro = 0xb4;
    if (setup->raceType == 3) {
        intro = 0x78;
    }
    if (setup->showIntro == 0) {
        intro = 0;
    }
    world->introFrames = intro;
    world->finishFrames = 0;
    world->introTimer = world->introFrames;
    RaceMusic_Init(this);
    world->frameScale = vsyncInterval * 0x1540000 / 60;
    RaceWorld_SetupCars(world);
    RaceMem_LoadCars(race);
    Race_LoadCourseOnce(race);
    Particles_Init(&race->world.particles);
    RaceWorld_Start(world);
    cam0 = &cams[0];
    RaceCam_Init(cam0, 0);
    cam1 = &cams[1];
    RaceCam_Init(cam1, 1);
    PauseMenu_Reset(this);
    Hud_Init(this);
    RaceLoad_RemoveVSync();
    frame = 0;
    pausedBy = 0;
    RacePad_Open(&race->world.pads[0], 0);
    RacePad_Open(&race->world.pads[1], 1);
    cam0->pad = &race->world.pads[0];
    cam1->pad = &race->world.pads[1];
    race->world.drawList = 0;
    world->showGhost = 1;
    g_Race_EndRequest = 0;
    vsyncInterval = setup->frameStep;
    RaceCam_Update(cam0);
    RaceCam_Update(cam1);
    twoPlayer = 0;
    drawFront = 0;
    if (setup->raceType == 0) {
        twoPlayer = 1;
        drawFront = 1;
        cam0->splitScreen = 1;
        cam1->splitScreen = 1;
    }
}

// Stops the music, shuts the race world down and closes both pads.
void RaceViewLoop::Leave()
{
    RaceMem *race = &g_Race_IsDemo;

    psxViewLoopDoubleBuffer::Leave();
    Music_Stop();
    RaceWorld_Shutdown(&race->world);
    PadDevice_Close(&race->world.pads[0].dev);
    PadDevice_Close(&race->world.pads[1].dev);
}

// One race frame: the pause menu while paused (Start or a pad unplugged opens it;
// the demo leaves instead), Select toggles the ghost; else the intro and finish
// counters, the particles, the race step, the cameras and the HUD, which returns
// 1 to go on. Then the music and the car sounds. Returns 0 to leave.
// The game's code but for one instruction: the game tests the first pad's
// connection with sltiu where GCC reuses the register that holds player 1.
int RaceViewLoop::Update()
{
    RaceMem *race = &g_Race_IsDemo;
    RacePad *pad0 = &race->world.pads[0];
    RacePad *pad1 = &race->world.pads[1];
    RaceWorld *world;
    PadState *raw0;
    PadState *raw1;
    RaceSetup *setup;
    int open;
    int player;
    int split;
    int step;
    int t;
    int done;
    int index;
    RaceMem *ghostRace;

    RacePad_Update(pad0);
    RacePad_Update(pad1);
    frame++;
    world = &race->world;
    if (pausedBy == 0) {
        open = 0;
        player = 1;
        raw0 = &race->world.pads[0].raw;
        raw1 = &race->world.pads[1].raw;
        split = RaceSetup_Get()->raceType == 0;
        if (world->isReplay == 0) {
            open = pad0->dev.connected == 0;
            if (split && pad1->dev.connected == 0) {
                open = 1;
                player = 2;
            }
        }
        if (raw0->pressed & PAD_SELECT) {
            world->showGhost = world->showGhost == 0;
        }
        if (!open) {
            if (raw0->pressed & PAD_START) {
                open = 1;
            }
            if (split && (raw1->pressed & PAD_START)) {
                open = 1;
                player = 2;
            }
        }
        if (open) {
            if (race->isDemo) {
                return 0;
            }
            if (g_Race_IsDemo.world.resultAnim[0] < 0) {
                pausedBy = player;
            }
        }
        if (pausedBy) {
            RaceWorld_MuteSounds(world);
        }
        setup = RaceSetup_Get();
        t = world->introTimer;
        step = setup->frameStep;
        t -= step;
        if (t < 0) {
            t = 0;
        }
        world->introTimer = t;
        t = world->finishFrames;
        if (t != 0) {
            t += step;
        }
        world->finishFrames = t;
        Particles_Update(&world->particles);
        ghostRace = &g_Race_IsDemo;
        if (g_TT_GhostResync) {
            g_TT_GhostResync = 0;
            GhostPlay_Start(&ghostRace->world.vehicles[0].phys);
        }
        Race_StepWorld();
        RaceCam_Update(&cams[0]);
        if (setup->raceType == 0) {
            RaceCam_Update(&cams[1]);
        }
        done = Hud_Update(this, raw0) != 1;
        if (done) {
            return 0;
        }
    } else {
        index = pausedBy - 1;
        done = PauseMenu_Update(this, &world->pads[index]) != 1;
        if (done) {
            RaceWorld_UnmuteSounds(world);
            pausedBy = 0;
        }
    }
    if (g_Race_EndRequest) {
        return 0;
    }
    RaceMusic_Update(this);
    if (pausedBy) {
        return 1;
    }
    if (twoPlayer == 0) {
        RaceWorld_UpdateSounds(world, cams);
    } else {
        RaceWorld_UpdateSounds2P(world, cams);
    }
    return 1;
}

// Two players draw into the displayed buffer (drawFront); then copies the 3D
// layer to g_Race3DLayer.
void RaceViewLoop::BeginDraw()
{
    if (drawFront) {
        bufferIndex ^= 1;
    }
    psxViewLoopDoubleBuffer::BeginDraw();
    if (drawFront) {
        bufferIndex ^= 1;
    }
    g_Race3DLayer = layers[0];
}

// RaceView_DrawFrame draws the race.
void RaceViewLoop::Draw()
{
    RaceView_DrawFrame(this);
}

RaceMenuLoop::RaceMenuLoop(RaceViewLoop *view, int arg)
{
    vptr.address = 0x8002ef58;  // RaceMenuLoop's vtable
    this->view = view;
    this->arg = arg;
}

RaceMenuLoop::~RaceMenuLoop()
{
}

// Runs the state machine: Init, Run, Finish.
void RaceMenuLoop::Execute()
{
    v_Init();
    v_Run();
    v_Finish();
}

// A state's result: 0 repeats the last direction, 2 (back) and 3 (forward)
// become it, the others (1 leave, a state) are returned as they are.
int RaceMenuLoop::ResolveState(int ret)
{
    switch (ret) {
    case 0:
        return state;
    case 2:
    case 3:
        state = ret;
        return ret;
    }
    return ret;
}

// Starts forward.
void RaceMenuLoop::Init()
{
    state = 3;
}

// Each state goes back or forward to its neighbours: 4 load, 5, then 8 before
// the race, 9 the race, 10, or with arg (the replay only) 11, 12 the replay, 13;
// both go on to 6 the menus and 7, then leave.
void RaceMenuLoop::Run()
{
    void *back;
    void *next;
    int ret;

state4:
    ret = v_State4();
    back = &&leave;
    next = &&state5;
    goto resolve;
state5:
    ret = v_State5();
    back = &&state4;
    if (arg == 0) {
        next = &&state8;
    } else {
        next = &&state11;
    }
    goto resolve;
state6:
    ret = v_State6();
    if (arg == 0) {
        back = &&state10;
    } else {
        back = &&state13;
    }
    next = &&state7;
    goto resolve;
state7:
    ret = v_State7();
    back = &&state6;
    next = &&leave;
    goto resolve;
state8:
    ret = v_State8();
    back = &&state5;
    next = &&state9;
    goto resolve;
state9:
    ret = v_State9();
    back = &&state8;
    next = &&state10;
    goto resolve;
state10:
    ret = v_State10();
    back = &&state8;
    next = &&state6;
    goto resolve;
state11:
    ret = v_State11();
    back = &&state5;
    next = &&state12;
    goto resolve;
state12:
    ret = v_State12();
    back = &&state11;
    next = &&state13;
    goto resolve;
state13:
    ret = v_State13();
    back = &&state11;
    next = &&state6;
resolve:
    switch (ResolveState(ret)) {
    case 1:
        goto leave;
    case 2:
        goto *back;
    case 3:
        goto *next;
    case 4:
        goto state4;
    case 5:
        goto state5;
    case 6:
        goto state6;
    case 7:
        goto state7;
    case 8:
        goto state8;
    case 9:
        goto state9;
    case 10:
        goto state10;
    case 11:
        goto state11;
    case 12:
        goto state12;
    case 13:
        goto state13;
    }
leave:
    ;
}

void RaceMenuLoop::Finish()
{
}

int RaceMenuLoop::State4()
{
    return 0;
}

int RaceMenuLoop::State5()
{
    return 0;
}

int RaceMenuLoop::State6()
{
    return 0;
}

int RaceMenuLoop::State7()
{
    return 0;
}

int RaceMenuLoop::State8()
{
    return 0;
}

// Runs the race.
int RaceMenuLoop::State9()
{
    view->v_Start(0);
    return 0;
}

int RaceMenuLoop::State10()
{
    return 0;
}

int RaceMenuLoop::State11()
{
    return 0;
}

// Runs the replay.
int RaceMenuLoop::State12()
{
    view->v_Start(1);
    return 0;
}

int RaceMenuLoop::State13()
{
    return 0;
}
