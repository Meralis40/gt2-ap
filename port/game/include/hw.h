// The PlayStation's I/O registers. The game addresses them through pointers, so
// HW_REG gives it the address of a register in a host copy of the I/O page; the
// runtime (port/hw) acts on what's written there.
#ifndef HW_H
#define HW_H

#ifdef __cplusplus
extern "C" {
#endif
extern unsigned char g_HwRegs[0x1000]; // 0x1f801000-0x1f801fff
#ifdef __cplusplus
}
#endif

#define HW_REG(offset) ((unsigned int)g_HwRegs + (offset))

#endif
