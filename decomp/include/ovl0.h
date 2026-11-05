// Types of the OVL0 code (the race), exported from the Ghidra project
// (DataTypeWriter) and adapted for cc1plus as resident.h is. Only the types that
// the decompiled OVL0 files use. See docs/decomp.md.
#ifndef OVL0_H
#define OVL0_H

#include "resident.h"
#include "viewloop.h"
#include "ovl2.h" // MenuPage

typedef struct CarPhys CarPhys, *PCarPhys;
typedef struct CarPhysWheel CarPhysWheel, *PCarPhysWheel;
typedef struct TyreAxle TyreAxle, *PTyreAxle;
typedef struct SuspAxle SuspAxle, *PSuspAxle;

struct SuspAxle { /* Suspension of one axle (2 at +0x12c of CarPhys). */
    short bumpStart; /* height below which the bump rubber works */
    short travelMin;
    short travelMax;
    unsigned char unk6[2];
    short rideHeight;
    unsigned char unka[2];
    int springRate;
    int bumpDead; /* damper, compressing: deadband, base, gain (sqrt of the speed) */
    int bumpBase;
    int bumpGain;
    int reboundDead; /* damper, extending */
    int reboundBase;
    int reboundGain;
    int antiRoll;
    int bumpRate;
    unsigned char unk30[2];
    short preload;
};

struct TyreAxle { /* Tyre curves of one axle (2 at +0x194 of CarPhys). */
    unsigned char angleCurve[0x2c]; /* TyreCurve_EvalAngle */
    unsigned char slipCurve[0x3c]; /* Tyres_LookupSlipCurve */
    short slipLimitUp; /* Gearbox_SelectGear: wheel slip that blocks a shift up */
    short slipLimitDown; /* and a shift down */
    unsigned char slipCurve2[0x24];
    unsigned char loadCurve[0x2c]; /* an EngineCurve_TorqueAt curve (Tyres_ComputeGripLimit) */
    unsigned char camberCurve[0x1c];
};

struct CarPhysWheel { /* Per-wheel state of CarPhys (4 at +0x460: front left, front right, rear left, rear right; index >> 1 = axle). */
    int groundY; /* ground height under the wheel */
    int groundY16; /* groundY << 4 */
    int suspLength; /* axle travel / 2 + static offset (CarPhys_PlaceOnTrack) */
    ushort heading; /* wheel heading (4096 = turn), used for the wheel velocity */
    unsigned char unke[2];
    short suspPos; /* suspension height, clamped to its travel (Wheel_ComputeSpringForce) */
    short suspSpeed;
    byte surface; /* surface type from the track collision */
    byte surface2;
    short unk16; /* last acceleration of the wheel or axle (RaceCars_UpdateDrivetrain) */
    int rollSpeed; /* wheel rolling speed */
    byte fxSmoke; /* tyre smoke (Body_UpdateTyreFx) */
    byte fxSqueal;
    byte fxTimer; /* frames between smoke puffs, 0xff = idle */
    byte fxGravel;
    ushort spinAngle; /* wheel rotation angle (0..0xfff) */
    byte dirt; /* grows on dirt surface type 2 hits; cleared by CarPhys_ServiceTyres */
    unsigned char unk23;
    short offX; /* wheel position relative to the body */
    short offY;
    short offZ;
    short slipGrip; /* 0x1000, lower at high wheel speed (CarPhys_UpdateLocalVelocity) */
    int velFwd; /* wheel velocity, forward */
    int velLat; /* wheel velocity, lateral */
    int gripLimit; /* Tyres_ComputeGripLimit */
    short tyreGrip; /* 0x1000 = new tyre; lowered by wear when g_TyreWearEnabled */
    short slipAbs; /* |slip ratio| (Tyres_LookupSlipCurve) */
    short lateralAbs; /* |cornering force before the grip limit| (CarPhys_UpdateDrivetrain) */
    byte wearRate; /* tyre wear of this frame, at most 0xff */
    signed char wearLevel; /* -0x80..0x7f (Tyres_ComputeGripLimit) */
    short bumpFade; /* 0..0x1000 with the wheel speed (RaceCars_PrepareWheelProbes) */
    short bump; /* surface bumps under the wheel (RaceCars_ApplyGroundProbes) */
    short unk44; /* slip ratio (Tyre_ComputeSlipRatio) */
    short unk46; /* second output of the slip curve (Tyres_LookupSlipCurve) */
    int spring; /* spring force (Wheel_ComputeSpringForce) */
    int damper;
    short unk50; /* slip angle (Tyres_ComputeSlipAngle) */
    short slipFade; /* 0..0x1000, fades the slip angle in with speed */
    int lateral; /* cornering force (CarPhys_UpdateDrivetrain) */
    int unk58;
    int unk5c;
    short brake; /* brake input of this wheel (0..0x1000) */
    unsigned char unk62;
    byte unk63; /* 0: wheel speed follows the road speed (CarPhys_UpdateWheelRotation) */
    int tyreWear; /* reset to g_TyreWearInit by CarPhys_ServiceTyres */
};

struct CarPhys { /* Vehicle dynamics state of one race car (0x798 bytes, built from CarParams by CarPhys_Init, placed by CarPhys_Setup). Sits at +0x2c of each 0xb40-byte race car object. Also holds the CPU driver state (ai*) and pit state. */
    int lenFront; /* footprint: front extent from the centre (CarColl_ComputeCorners, Body_ComputeFootprint) */
    int lenRear; /* footprint: rear extent */
    short width; /* footprint: full width */
    unsigned char unka[2];
    short distFrontAxle; /* CG to front axle (wheelbase split by weight distribution) */
    short distRearAxle; /* CG to rear axle */
    short wheelbase;
    unsigned char unk12[2];
    short rearWeight; /* 0x1000 - frontWeight */
    short frontWeight; /* front weight fraction (0x1000 = 100%) */
    short halfTrackF; /* front track width / 2 */
    short halfTrackR; /* rear track width / 2 */
    byte unk1c; /* CarPhys_SetUnk1c */
    byte driveClass; /* Drivetrain_GetClass of the layout; index into g_AiDriveParams */
    unsigned char unk1e[10];
    short mass; /* CarPhys_EffectiveMass(weight) * 4 */
    short invMass;
    unsigned char unk2c[4];
    int cornerGrip; /* cornering speed scale of the car (AiLine_CornerSpeed) */
    unsigned char unk34[2];
    short brakeDecel; /* braking distance (Ai_SpeedControl) */
    unsigned char unk38[40];
    short maxSteer; /* max steering angle */
    short steerSpeed; /* steering rate per frame (0x5c7 on dirt courses) */
    unsigned char unk64[24];
    struct EngineCurve engine; /* torque curve (EngineCurve_Build); +0x8c redline rpm, +0x8e idle rpm, +0xac turbo */
    struct SuspAxle susp[2]; /* front, rear */
    struct TyreAxle tyres[2]; /* front, rear */
    int topSpeed; /* CarPhys_ComputeTopSpeed */
    short surfaceGrip[8]; /* by CarPhysWheel.surface (Tyres_ComputeGripLimit) */
    short surfaceBrake[8]; /* by surface (Ai_SpeedControl) */
    short camber[2]; /* front, rear */
    unsigned char unk36c[4];
    byte driveLayout; /* start of the drivetrain block (Drivetrain_Init) */
    unsigned char unk371;
    byte gearCount; /* Disputed: used as the gear count by the gearbox code, but CarPhys_UpdateDrivetrain / RaceCars_UpdateDrivetrain branch on it being 1 or 2 like a transmission mode. */
    unsigned char unk373;
    short centerSplit; /* front share of the wheel speed, layout 6 */
    unsigned char unk376[30];
    ushort launchLimit; /* rpm above which the clutch starts slipping (CarPhys_UpdateDrivetrain) */
    ushort launchRpm; /* drivetrain +0x26: min redline rpm (x100 from CarParams) */
    unsigned char unk398[12];
    int gearRatio[8]; /* Drivetrain_EngineSpeedFromWheels */
    int gearSpeed[8]; /* speed per rpm of each gear */
    short tyreRadius[2]; /* front, rear */
    short invTyreRadius[2]; /* 0x1000000 / tyreRadius */
    unsigned char unk3ec[52];
    int brakeMin; /* least brake force (Ai_SpeedControl) */
    unsigned char unk424[56];
    byte carIndex;
    byte control; /* 0 player, 2 CPU (CarPhys_SetControl) */
    byte ghost; /* non-zero: no car-to-car contact */
    unsigned char unk45f;
    struct CarPhysWheel wheels[4]; /* FL, FR, RL, RR */
    int section; /* track collision section */
    int trackPos; /* distance along the course */
    short lap;
    short clutch;
    short steer; /* current steering */
    unsigned char unk60e[2];
    short throttle; /* 0..0x1000 */
    short brake; /* 0..0x1000 */
    short unk614; /* cleared by CarAi_Drive */
    unsigned char unk616[2];
    byte gear;
    byte clutchIn;
    byte diffLocked[2]; /* front, rear limited slip locked up (RaceCars_UpdateDifferentials) */
    byte startDelay;
    byte revLimiter; /* set at redline, cleared 500 rpm below */
    short engineLoad; /* throttle after the turbo */
    short turboSpool[2]; /* two turbo stages */
    int engineSpeed; /* rpm * 0x1000 / 60 */
    int vel[3]; /* world velocity x, y, z */
    int axleSpeed[2]; /* front, rear */
    unsigned char unk63c;
    byte wallContact;
    char wallSteerKick; /* CarPhys_WallDeflect */
    byte wallSide;
    short impact; /* wall impact strength, decays each frame */
    byte unk642; /* CarPhys_SetUnk642 (player cars only) */
    unsigned char unk643;
    short pitch;
    short roll;
    short yaw; /* 4096 = turn */
    unsigned char unk64a[2];
    int yawRate;
    short unk650;
    short unk652;
    short wallNormalX;
    short wallNormalZ;
    uint dirt; /* in the race scale: RaceSetup.dirt x600000 >> 12 */
    int pos[3]; /* world position x, y, z */
    short fwdDir[3]; /* body forward direction (0x1000 = unit), from the matrix build Matrix_FromAngles(+0x668, +0x670, +0x678) */
    unsigned char unk66e[2];
    short rightDir[3]; /* body right direction */
    unsigned char unk676[46];
    int speedFwd; /* forward speed (body frame) */
    int speedLat; /* lateral speed */
    short rpm;
    ushort speedKmh;
    byte checkpoint; /* next checkpoint index (g_Checkpoints) */
    byte airborne; /* all wheels unloaded (RaceCars_UpdateSuspension) */
    byte pitBox; /* pit lane segment of this car (AiLine_GetPitBox) */
    byte bufIdx; /* double-buffered contact data index */
    int footprint[16]; /* 2 buffers (bufIdx) x 4 corners x {x, z} (Body_ComputeFootprint) */
    unsigned char unk6f4[4];
    ushort maxSpeedKmh;
    byte hitFeedback; /* set once a hard hit sent feedback */
    unsigned char unk6fb;
    byte aiStarted; /* Disputed: also set when a license test ends, after which no more lap times are recorded. */
    unsigned char unk6fd;
    short timeStep; /* frame step of this car, scaled by catchUp (Body_ApplyTimeScale) */
    unsigned char unk700[4];
    int rateScale; /* frame rate over catchUp (Body_ApplyTimeScale) */
    short throttleTarget;
    unsigned char unk70a[6];
    int engineFriction;
    int engineBraking;
    unsigned char unk718;
    byte wheelContact; /* bits 0-3: wheel below its rest height; bits 4-7 previous pass (RaceCars_UpdateSuspension) */
    unsigned char unk71a[30];
    ushort unk738;
    short chaseSway[2]; /* taken off the chase camera's angles (RaceCam_SetChase) */
    short unk73e;
    short wallHit;
    short boost;
    short blowOffState;
    short boostDisplay;
    unsigned char unk748[8];
    byte racePos; /* place in the race, from 1 (RaceOrder_Update) */
    byte licenseState; /* 1 passed, 2 failed (LicenseTest_Succeed / LicenseTest_Fail) */
    unsigned char unk752[4];
    byte licenseResult; /* 1 cleared; why it failed: 3 overran, 4 hit or moved, 5 hit (LicenseTest_CheckEnd) */
    byte unk757;
    byte unk758;
    byte blowOffVolume; /* turbo blow-off sound volume */
    unsigned char unk75a[2];
    uint licenseTime;
    unsigned char unk760[2];
    byte unk762;
    byte unk763;
    byte aiMessage; /* CarAi_SetMessage */
    byte aiMessageHold;
    short catchUp; /* catch-up time factor, 4.12 (CatchUp_ApplyAI / CatchUp_Apply2P) */
    unsigned char unk768;
    byte launchHold;
    short followSlowdown; /* car ahead: 0x400/0x800/0x1000 */
    byte proximity[8]; /* contact zones set by CarPhys_SetProximityFlags (1 near, 2 contact) */
    short pushed; /* pushed from behind (0..0x1000) Disputed: CarPhys_UpdateAero reads it as the slipstream build-up rate. */
    unsigned char unk776[2];
    int pushVec[2]; /* car to car separation push x, z (CarColl_SeparateOverlaps) */
    uint raceTime; /* the car's race clock (Result_DrawTimeRow, GhostPlay_RestartLap) */
    byte aiPitLimit;
    byte aiBlockMask;
    byte aiMode; /* Ai_SetMode: 0 race, 2 pit entry, 3 pit stop, 4 pit exit, 5/6 stop, 7 stopped */
    unsigned char unk787;
    byte aiPassState;
    byte aiLine; /* racing line index in g_AiLines (3 pit entry, 4 pit lane) */
    byte aiSegment;
    byte aiCornerPrev;
    byte aiCornerNext; /* 0 straight, 1/2 corner, 3/4 braking */
    byte flags;
    ushort pitTimer; /* pit stop countdown (5 s) */
    int aiTargetSpeedPrev;
    int aiTargetSpeedNext;
};

typedef struct CarSound CarSound, *PCarSound;
typedef union EngineVoice_u4 EngineVoice_u4, *PEngineVoice_u4;

union EngineVoice_u4 {
    short vol[2];
    int volPair;
};

struct CarSound {
    uchar mode;
    uchar carIndex;
    short doppler;
    short rpm;
    short pitchLoop0;
    short pitchLoop2;
    short pitchLoop3;
    short volume;
    short unk0e;
    short pan;
    short unk12;
    short unk14;
    short volEngine;
    short volEngine2;
    short volLoop[4];
    short volOneShot;
    struct EngineSound engine;
    struct EngineSound engine2;
    struct SfxLoop loops[4];
    uchar unk94;
    uchar unk95;
    uchar pad96[2];
};

typedef struct CatchUpParams CatchUpParams, *PCatchUpParams;

struct CatchUpParams {
    int aheadPct;
    int aheadStart;
    int aheadEnd;
    int behindPct;
    int behindStart;
    int behindEnd;
    int scale;
};

typedef struct CourseData CourseData, *PCourseData;
typedef struct CourseSectionTable CourseSectionTable, *PCourseSectionTable;

typedef struct CourseSection CourseSection, *PCourseSection;
typedef struct CourseMesh CourseMesh, *PCourseMesh;

struct CourseSectionTable {
    uint unk00;
    ushort count;
    ushort pad06;
    void *spriteTex;
    struct CourseSection *sections[1];
};

struct CourseMesh {
    void *verts;
    void *prims[8];
    int *sprites;
    int *flares;
    ushort vertCount;
    ushort pad2e;
    ushort primCount[8];
    ushort spriteCount;
    ushort flareCount;
};

struct CourseSection {
    ushort nextIdx;
    ushort prevIdx;
    struct CourseSection *next;
    struct CourseSection *prev;
    uint groupMask;
    uchar unk10[8];
    int planeOrigin[3];
    short planeNormal[3];
    short pad2a;
    int radius;
    int center[3];
    uchar unk3c[8];
    int bbox[10];
    int bboxFar[10];
    struct CourseMesh *mirrorMesh;
    int *walls;
    void *groundGrid;
    ushort *visList;
    struct CourseMesh mesh;
};

struct CourseData {
    uchar unk00[16];
    struct CourseSectionTable *sections;
    uint *modelLib;
    uint *objModels;
    ushort *table1c;
    void *data20;
    int checkpointCount;
    int checkpoints[60]; /* course distance of each sector line */
    uint *objGroups[33];
};

typedef struct InfoBar InfoBar, *PInfoBar;

struct InfoBar {
    struct WipeBar bar;
    struct TextItem label;
    uint value;
    uint valueRgb;
    short anim;
    ushort kind;
};

typedef struct InputLog InputLog, *PInputLog;

struct InputLog { /* Header of a run-length coded input recording (InputLog_*): the replay of every player car, and the time-trial lap ghosts. */
    int framesWritten; /* frames recorded */
    int framesLeft; /* playback: frames left (copied from framesWritten on reset) */
    int runLength; /* frames the current values repeat; -1 = nothing pending */
    ushort ended; /* 1 when full (record) or at the end (playback) */
    ushort pos; /* byte offset of the next packet in data */
    ushort length; /* bytes written */
    ushort capacity; /* data size; 0x4400 for a race, 0x1000 for a time-trial lap */
    byte header; /* last packet header: bits 0-2 run length low bits, 3-6 which values follow, 7 run length follows */
    byte flags; /* RaceInput flags byte */
    byte buttons; /* action bits 0-7 */
    byte steer; /* analog steer */
    byte pedals; /* throttle index | brake index << 4 */
    byte data[3]; /* run-length packed input packets (capacity bytes) */
};

typedef struct QuickMenu QuickMenu, *PQuickMenu;
typedef struct QuickMenuPage QuickMenuPage, *PQuickMenuPage;

struct QuickMenuPage {
    struct MenuPage page;
    short timer;
    short sel;
    short mode;
    short arg;
    uchar pad1c[8];
};

struct QuickMenu {
    struct psxViewLoopDoubleBuffer base;
    signed char hookActive;
    uchar pad71[3];
    uchar (*hook)(void *, int, int);
    void *hookArg;
    uchar unk7c;
    uchar pad7d[3];
    struct DrawLayer layers[6];
    struct PadDevice pads[2];
    struct PadState padState[2];
    struct QuickMenuPage *activePage;
    struct QuickMenuPage *leavingPage;
    struct QuickMenuPage *pageStack[16];
    signed char pageDepth;
    signed char transitionTimer;
    signed char transitionKind;
    uchar pad213;
    struct TextCtx text;
    uint drawStamp;
    int frameCount;
    int updateCount;

    QuickMenu();                // QuickMenu__QuickMenu (quickmenu.cpp)
    ~QuickMenu();
};

typedef struct RaceCam RaceCam, *PRaceCam;
typedef struct RacePad RacePad, *PRacePad;

typedef struct SVECTOR SVECTOR, *PSVECTOR;

struct SVECTOR {
    short vx;
    short vy;
    short vz;
    short pad;
};

struct RacePad { /* A race controller (RacePad_Open / RacePad_Update): pad device plus the key-config mapped action bits. */
    struct PadDevice dev;
    struct PortConfig *config; /* &g_GameState.ports[port] (button maps) */
    struct PadState raw; /* PadButtons_Read output */
    byte analogCopy[16]; /* copy of dev.analogMask and dev.analog */
    short analogCopy2; /* copy of dev +0x58 */
    unsigned char unk8a[2];
    uint actHeld; /* race actions (bit n = action n of the key config) held */
    uint actPressed;
    uint actReleased;
    uint actRepeat;
    ushort analogActs; /* bit n: action n is mapped to an analog axis */
    ushort analogValue[9]; /* per action: axis value (0..0xff, pedal axes folded) */
};

struct RaceCam { /* A race camera (RaceViewLoop.cams; RaceCam_*). */
    struct Camera cam;
    struct RacePad *pad; /* controller that drives this camera */
    int section; /* track section of the camera */
    struct MATRIX view; /* camera transform; view.t is the position */
    struct SVECTOR angles; /* Euler angles of view (Matrix_ToEuler) */
    int prevPos[3];
    int vel[3];
    struct SVECTOR velDir;
    short forward[4]; /* view column 2 */
    short right[4]; /* view column 0 */
    int speed;
    ushort tvFlags; /* flags of the current track-side camera entry */
    unsigned char unk102;
    byte splitScreen; /* half-height view (2 players); toggled in a 2P replay */
    unsigned char unk104;
    byte replayMode; /* replay: 0 track-side, 1 onboard, 2 track-side following the leader */
    byte onboardView; /* replay onboard view 0..8 (RaceCam_SetOnboard) */
    byte replayOpt; /* replay: cycled 0..2 by action 9 */
    byte insideCar; /* camera is inside the target car: it is not drawn */
    byte rearMirror; /* bumper view looking forward */
    byte innerView; /* car-mounted view (sound mix) */
    byte tvView; /* 1 unless the camera is attached to the target car */
    byte targetCar;
    unsigned char unk10d;
    byte chaseView; /* race: 0 bumper, 1 near, 2 far chase (action 8 cycles it) */
    byte lookBack; /* action 9 held */
};

// The race screen loop (0x2f4 bytes), derived from the resident
// psxViewLoopDoubleBuffer (viewloop.h); raceloops.cpp emits its vtable. Start(mode)
// runs it; Update steps the race once per frame. RaceDevelopment derives from it.
class RaceViewLoop : public psxViewLoopDoubleBuffer {
public:
    uint randomSeed; /* vsync count + g_RandomSeed at construction */
    struct DrawLayer layer0; /* not added to the draw context */
    struct DrawLayer layers[4]; /* priorities 0x200 (3D), 0x300, 0x400, 0x500 */
    struct RaceCam cams[2]; /* cams[1] only in 2-player races */
    ushort frame;
    byte worldReady; /* RaceWorld_Init done */
    unsigned char unk2e7;
    byte replay; /* Start() argument: 1 replay / demo */
    byte pausedBy; /* player (1/2) who opened the pause menu, 0 = running */
    byte twoPlayer;
    byte drawFront; /* 2P: draws into the displayed buffer (BeginDraw swaps bufferIndex) */
    signed char musicPlaying; /* track playing, -1 none */
    unsigned char musicLoop;
    signed char musicTrack; /* track requested (RaceMusic_Request) */
    unsigned char raceTrack; /* the random race track */
    signed char musicNext; /* played when the current one ends */
    unsigned char musicKind; /* picks the finish track */
    unsigned char pad2f2[2];

    RaceViewLoop();
    virtual ~RaceViewLoop();
    virtual void Enter();
    virtual void Leave();
    virtual int Update();
    virtual void BeginDraw();
    virtual void Draw();
    virtual void Start(int mode);
};

struct PreQuickMenuIO {
    struct psxViewLoopDoubleBuffer base;
    struct DrawLayer layer;
    short loader;
    short frames;
    signed char step;
    signed char fontTimUploaded;
    signed char menuTimUploaded;
    signed char mode;

    PreQuickMenuIO();           // PreQuickMenuIO__PreQuickMenuIO (quickmenu.cpp)
    ~PreQuickMenuIO();
};

// Base of the race flow state machines (0x10 bytes; raceloops.cpp emits its
// vtable). Execute runs Init, Run and Finish; Run steps through the states
// (State4..State13), each returning 1 to leave, a state, or a direction: 3
// forward, 2 back, 0 the last one.
class RaceMenuLoop {
public:
    int arg; /* Race_Entry argument: non-zero = replay only (attract demo, raceType 0xc) */
    RaceViewLoop *view;
    int state; /* last direction: 3 forward (initial), 2 back */

    RaceMenuLoop(RaceViewLoop *view, int arg);
    virtual ~RaceMenuLoop();
    virtual void Init();
    virtual void Run();
    virtual void Finish();
    virtual int State4();       // load
    virtual int State5();
    virtual int State6();       // menus
    virtual int State7();
    virtual int State8();       // before the race
    virtual int State9();       // the race
    virtual int State10();
    virtual int State11();
    virtual int State12();      // the replay
    virtual int State13();
    void Execute();
    int ResolveState(int ret);
};

// The arcade race loop (0x5d8 bytes; arcaderace.cpp emits its vtable),
// constructed on ArcadeRace_Run's stack: the race view, the loading screen
// around the menus and the QuickMenu.
class ArcadeRaceLoop : public RaceMenuLoop {
public:
    RaceViewLoop raceView;
    PreQuickMenuIO menuIO; /* loading screen run before / after the menus (mode: 0 enter menus, 1 back to the race) */
    QuickMenu menu; /* menu.unk7c is the choice: 0 replay, 1 retry, 2 quit, 4 start / next race */
    unsigned char unk5cc[4];
    signed char firstEntry; /* the menus' first time after a race: shows the results */
    signed char ghostKept; /* time trial: a ghost was kept, car 1 races it */
    unsigned char unk5d2[6];

    ArcadeRaceLoop(int arg);
    virtual ~ArcadeRaceLoop();
    virtual int State4();
    virtual int State6();
    virtual int State8();
    virtual int State10();
    virtual int State13();
};

// GranTurismoRaceLoop's layout (0x5e0 bytes) until it is a class: its
// RaceMenuLoop, then the members ArcadeRaceLoop has too. RaceLoop_EnterMenus
// and RaceLoop_LeaveMenus take it.
struct RaceLoop {
    RaceMenuLoop base;
    RaceViewLoop view;
    struct PreQuickMenuIO menuIO; /* loading screen run before / after the menus (mode +0x87: 0 enter menus, 1 back to the race) */
    struct QuickMenu menu; /* QuickMenu; menu.unk7c is the choice: 0 replay, 1 retry, 2 quit, 4 start / next race */
    byte raceDoneA; /* GT: set after a race */
    byte raceDoneB;
    unsigned char unk5ce[2];
    signed char firstEntry;
    signed char resultsPending; /* race finished: show the result pages */
    short savedLapCount; /* RaceSetup.unk0f before the practice run */
    short savedCarCount;
    short savedIntro; /* RaceSetup.unk0d */
    short savedRaceType; /* GranTurismoRaceLoop only */
    unsigned char unk5da[6];
};

typedef struct RaceVehicle RaceVehicle, *PRaceVehicle;
typedef struct RaceWheelDraw RaceWheelDraw, *PRaceWheelDraw;

struct RaceWheelDraw { /* Drawn wheel position of a race car. */
    short pos[3]; /* drawn wheel position (y follows the suspension) */
    unsigned char unk6[2];
    short rest[3]; /* wheel position on the model (RaceVehicle_SetupWheels) */
    unsigned char unke[2];
};

struct RaceVehicle { /* One race car (0xb40 bytes; RaceWorld.vehicles[6] at 0x800a9688). Init: RaceVehicle_Init. */
    struct RaceVehicle *next; /* next car in RaceWorld.drawList (sorted by camera distance) */
    unsigned char unk4[8];
    short index; /* car index (RaceSetup.cars) */
    byte present; /* car takes part (0 for an unused ghost) */
    byte hidden; /* not drawn */
    byte hasTurbo; /* CarParams.turboBoost != 0; picks the CPU engine sound */
    unsigned char unk11[3];
    int section; /* copy of phys.section, read by the cameras */
    short controlType; /* 0 CPU, 1 recorded car (ghost), 2 player 1, 3 player 2 (from RaceCar.driverType) */
    unsigned char unk1a[2];
    struct InputLog *inputLog; /* recorded input: written for players, read for recorded cars and replays */
    char ghostLap; /* time trial replay: lap ghost shown */
    char ghostLapDelta; /* time trial replay: -1/+1 requested by the replay camera */
    unsigned char unk22[2];
    int finishTime; /* set when the car finishes; -1 when it retires */
    unsigned char unk28[4];
    struct CarPhys phys; /* vehicle dynamics (fork R4) */
    struct RaceWheelDraw wheelDraw[4];
    int camDist; /* approximate distance to the camera */
    int soundLevel; /* distance attenuation 0..0x4000, 0 when far */
    int relPos[3]; /* position relative to the camera */
    unsigned char unk818[4];
    struct MATRIX world; /* body transform (RaceVehicle_UpdateTransform) */
    struct MATRIX shadow; /* shadow transform */
    int prevPos[3];
    int vel[3]; /* world.t - prevPos */
    unsigned char unk874[4];
    struct CarModelHeader *model; /* RaceWorld.carModels[index] */
    int bodyLength; /* half length of the model x16 */
    unsigned char unk880[524];
    byte lapInvalid; /* lap does not count (set by the lap callbacks) */
    byte message; /* race message id (Race_ShowMessage) */
    short lapTimeTimer; /* frames the lap time stays shown (0x78) */
    short refTimeTimer; /* frames refTime stays shown (0x78) */
    short messageTimer;
    int lapTimeShown; /* last lap (or race) time shown on the HUD */
    void *lapLabel; /* HUD label for lapTimeShown */
    int refTime; /* time trial: course best lap; 2P: gap to the leader */
    int refTime2; /* time trial: lap time; 2P: lead over the other car */
    byte soundStart; /* car sound state starts here (0x98 bytes, fork R2) */
    unsigned char unkaa5[151];
    byte soundMuted;
    unsigned char unkb3d[3];
};

typedef struct RaceWorld RaceWorld, *PRaceWorld;
typedef struct ParticlePool ParticlePool, *PParticlePool;

typedef struct Particle Particle, *PParticle;

struct Particle { /* One smoke particle (Particles_*). */
    short kind;
    short next; /* index of the next particle in its list, -1 = end */
    short size; /* grows by pool.growStep each frame */
    short life; /* decreases by fade; freed below 0 */
    ushort seed;
    short fade; /* life lost per frame */
    int pos[3]; /* world position; y rises by pool.rise per frame */
};

struct ParticlePool { /* Tyre smoke particles of the race (RaceWorld.particles). */
    int rise; /* 0x444 */
    int fadeKind0; /* 0x90 */
    int fadeKind1; /* 0x48 */
    int growStep; /* 8 */
    short spawnCount; /* cycles 0..15 through the fade table at 0x8002f4f8 */
    short activeHead;
    short freeHead;
    unsigned char unk16[2];
    struct Particle particles[256];
};

struct RaceWorld { /* The race state (g_RaceWorld, 0x800a9504; RaceWorld_Init). */
    struct CarModelHeader *carModels[6];
    byte isReplay; /* race view was started with Start(1): replay / demo playback */
    unsigned char unk19;
    ushort introFrames; /* length of the pre-race camera intro (0xb4, 0x78 license, 0 when RaceSetup.unk0d is 0) */
    ushort introTimer; /* counts down from introFrames */
    ushort finishFrames; /* counts up once the race is over; recording stops after 300 frames (60 license) */
    struct CourseRecord *courseRecord; /* g_GameState course record of this course */
    struct RacePad pads[2];
    struct RaceVehicle vehicles[6];
    struct RaceVehicle *drawList;
    struct ParticlePool particles;
    short countdown;
    short resultAnim[2];
    unsigned char unk5d26[2];
    int frameScale; /* vsyncInterval * 0x1540000 / 60 */
    byte course; /* course index (Course_Get) */
    byte carCount; /* cars in the race (2 in time trial: player and ghost) */
    byte showGhost; /* toggled by player 1 action 17 */
    unsigned char unk5d2f;
};

// The race's memory from g_Race_IsDemo on: the game addresses g_RaceWorld from it.
struct RaceMem {
    unsigned char isDemo;
    unsigned char pad1[3];
    RaceWorld world;
};

typedef struct ResultCar ResultCar, *PResultCar;

struct ResultCar {
    short unk00;
    short tpage;
    void *model;
    uchar enabled;
    uchar brightness;
    uchar pad0a[2];
    struct GroundDisc disc;
};

typedef struct TitleBanner TitleBanner, *PTitleBanner;

struct TitleBanner {
    struct WipeBar bar;
    struct TextItem text;
};

#endif
