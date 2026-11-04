// Resident code 8007f924-8007fe8c (pad): the controller driver's vblank work, which reads
// each port's raw buffer into its PadDevice, and the GPU's reset.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"

extern "C" {

static inline void PadDevice_ClearVibration(short *v)
{
    v[0] = 0;
    v[1] = 0;
    v[2] = 0;
    v[3] = 0;
}

// Adds the driver to the vblank list and starts reading the controllers.
#ifdef NON_MATCHING
// The address in $a0 first (matching-quirks.md, not solved).
void Pad_Install()
{
    PadSystem *sys = &g_PadSystem;

    sys->vsync.func = Pad_VSync;
    sys->vsync.priority = 0x10000;
    sys->devices[0] = 0;
    sys->devices[1] = 0;
    VSyncList_Add(&g_GfxIrq.vsyncCallbacks, &sys->vsync);
    PadStartCom();
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/pad", Pad_Install);
#endif

#ifdef NON_MATCHING
// The address in $a0 first.
void Pad_VSync(void)
{
    PadSystem *sys = &g_PadSystem;

    Pad_PollState(sys, 1);
    Pad_UpdateDevice(sys->devices[1]);
    Pad_PollState(sys, 0);
    Pad_UpdateDevice(sys->devices[0]);
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/pad", Pad_VSync);
#endif

// Follows the controller's state: the actuators need aligning again when the number
// PadInfoAct gives changes, or when the controller has gone.
#ifdef NON_MATCHING
// Registers only: the game keeps the id and the old mode in $s3 and $s2, the current mode in $s0.
void Pad_PollState(PadSystem *pads, int port)
{
    PadPort *p = &pads->ports[port];
    int id = port << 4;
    int cur = p->mode;
    int state;
    int old;
    int changed;

    state = PadGetState(id);
    old = cur;

    switch (state) {
    case 0:
    case 1:
        old = -1;
        cur = 0;
        break;
    case 2:
        cur = 1;
        break;
    case 6:
        cur = PadInfoAct(id, -1, 0);
        break;
    }
    changed = (old ^ cur) != 0;
    p->mode = cur;
    p->align |= changed;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/pad", Pad_PollState);
#endif

// Sets the device of a port and returns the one it replaces.
PadDevice *PadSystem_SetDevice(PadSystem *pads, unsigned int port, PadDevice *dev)
{
    PadDevice **slot = &pads->devices[port];
    PadDevice *old;

    if (port >= 2) return 0;
    old = *slot;
    *slot = dev;
    return old;
}

void PadDevice_Open(PadDevice *pad, int port, PadHandler *handlers)
{
    PadButtons_Init(&pad->buttons);
    PadDevice_ClearVibration(&pad->force);
    pad->handlers = handlers;
    pad->port = port;
    pad->type = 0;
    pad->connected = 1;
    pad->prevDevice = PadSystem_SetDevice(&g_PadSystem, port, pad);
}

void PadDevice_Close(PadDevice *dev)
{
    PadHandler *h;
    int leave;

    PadDevice_ClearVibration(&dev->force);
    leave = Sys_CriticalSection(0);
    h = PadDevice_FindHandler(dev);
    if (h != 0 && h->post != 0) {
        h->post(dev);
    }
    PadSystem_SetDevice(&g_PadSystem, dev->port, dev->prevDevice);
    Sys_CriticalSection(leave);
}

// The handler of the controller type in the port's raw buffer (type 0 for no controller).
// The table ends with type 0.
#ifdef NON_MATCHING
// One word: the game shifts typeLen with sra, GCC knows it is zero-extended and uses srl.
PadHandler *PadDevice_FindHandler(PadDevice *dev)
{
    PadHandler *h = dev->handlers;
    PadSystem *sys = &g_PadSystem;
    volatile PadRawBuffer *raw = &sys->raw[dev->port];
    int type = raw->typeLen;

    type >>= 4;
    if (raw->status != 0) type = 0;
    if (h != 0) {
        int t;

        do {
            t = h->type;
            if (type == t) return h;
            h++;
        } while (t != 0);
    }
    return 0;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/pad", PadDevice_FindHandler);
#endif

// Reads the port's buttons and axes by controller type, and calls the type's handlers.
#ifdef NON_MATCHING
// Needs its jump table in .rodata, and the registers and the address of the raw buffer differ.
void Pad_UpdateDevice(PadDevice *dev)
{
    if (dev != 0) {
        PadSystem *sys = &g_PadSystem;
        volatile PadRawBuffer *raw = &sys->raw[dev->port];
        volatile unsigned char *data = raw->buttons;
        volatile signed char *sdata = (volatile signed char *)data;
        int type = raw->typeLen;
        int status;
        int buttons;
        unsigned int zeroed[4];
        short *analog = (short *)zeroed;
        void (*update)(PadDevice *, ushort, ushort *, int);
        void (*post)(PadDevice *);
        int sameType;
        PadHandler *h;

        type >>= 4;
        status = raw->status;
        if (status != 0) type = 0;
        buttons = 0;
        zeroed[0] = 0;
        zeroed[1] = 0;
        zeroed[2] = 0;
        zeroed[3] = 0;
        if (status == 0) {
            buttons = (data[0] | data[1] << 8) ^ 0xffff;
        }
        switch (type) {
        case 1: // mouse: two signed axes
            analog[0] = sdata[2];
            analog[1] = sdata[3];
            break;
        case 6: // gun: two 16-bit coordinates
            analog[0] = sdata[3] << 8 | data[2];
            analog[1] = sdata[5] << 8 | data[4];
            break;
        case 2:
        case 5:
        case 7: // four 8-bit axes
            analog[0] = data[2];
            analog[1] = data[3];
            analog[2] = data[4];
            analog[3] = data[5];
            break;
        case 0xe:
            analog[0] = data[2] | data[3] << 8;
            analog[1] = data[4];
            break;
        case 3:
        case 4:
            break;
        default:
            type = 0;
            buttons = 0;
            break;
        }
        post = 0;
        sameType = dev->type;
        dev->connected = type != 0;
        dev->type = type;
        sameType = (sameType ^ type) == 0;
        h = PadDevice_FindHandler(dev);
        update = 0;
        if (h != 0) {
            update = h->update;
            post = h->post;
        }
        if (update == 0) {
            PadButtons_Update(&dev->buttons, 0);
            dev->analogMask = 0;
        } else {
            update(dev, buttons, (ushort *)analog, sameType);
        }
        if (post != 0) {
            post(dev);
        }
        if (--dev->vibrationFrames == -1) {
            PadDevice_ClearVibration(&dev->force);
        }
    }
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/pad", Pad_UpdateDevice);
#endif

// Stops the GPU's DMA, enables its channel and resets the GPU, with interrupts masked.
void Gpu_Reset(void)
{
    volatile unsigned int *dma;
    volatile unsigned int *gpu;
    int mask;

    GPU_cw(0);
    mask = SetIntrMask(0);
    dma = (volatile unsigned int *)0x1f801080;
    gpu = (volatile unsigned int *)0x1f801810;
    dma[0x28 / 4] = 0x401;
    dma[0x70 / 4] |= 0x800;
    gpu[1] = 0;
    SetIntrMask(mask);
}

}
