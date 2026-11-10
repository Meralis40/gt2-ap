// The PlayStation's I/O registers. The game addresses them through pointers, so
// HW_REG gives it the address of a register in a host copy of the I/O page; the
// runtime (port/hw) acts on what's written there.
#ifndef HW_H
#define HW_H

#ifdef __cplusplus
extern "C" {
#endif
extern unsigned char g_HwRegs[0x1000]; // 0x1f801000-0x1f801fff

void Hw_Gp0(unsigned int word);
void Hw_Gp1(unsigned int word);
unsigned int Hw_GpuStat(void);
unsigned int Hw_GpuRead(void);
void Hw_GpuDrawList(const void *list);
void Hw_SpuDma(void *guest, unsigned int bytes, int toGuest);
void Rt_Pump(void);    // a place where the game waits: lets interrupts and the host run
void Rt_DmaDone(int channel);
// The display settings the Options menu shows: which renderer draws, and the hardware one's
// resolution, 1 to 8 times the console's. Setting one applies it at once and saves it.
#ifndef HOST_DISPLAY_OPTIONS
#define HOST_DISPLAY_OPTIONS  // in both hw.h, the runtime's and the game's
enum { HOST_DISPLAY_RENDERER, HOST_DISPLAY_SCALE };
enum { HOST_RENDERER_HARDWARE, HOST_RENDERER_SOFTWARE };
int Host_DisplayOption(int option);
void Host_SetDisplayOption(int option, int value);
#endif
#ifdef __cplusplus
}
#endif

#define HW_REG(offset) ((__UINTPTR_TYPE__)g_HwRegs + (offset))

#endif
