// GTE (geometry coprocessor) access for the C++ versions of the game's handwritten GTE
// code. Those functions don't match (they were never compiler output) and are built only
// with NON_MATCHING, so this header exists to make them readable and portable.
//
// In the decomp build every operation is inline assembly: the register moves are mtc2,
// mfc2, ctc2, cfc2, lwc2 and swc2, and the GTE commands use the instruction macros of
// include/gte_macros.inc, which include_asm.h already gives every file. The two nops
// before each command cover the GTE's two-instruction delay after a register write.
//
// A port defines GTE_C and supplies the gte_* functions declared at the end instead
// (a GTE emulation, or a translation of each command).
#ifndef GTE_INCLUDE_H
#define GTE_INCLUDE_H

// Data registers (mtc2/mfc2).
enum {
    GTE_VXY0, GTE_VZ0, GTE_VXY1, GTE_VZ1, GTE_VXY2, GTE_VZ2, GTE_RGBC, GTE_OTZ,
    GTE_IR0, GTE_IR1, GTE_IR2, GTE_IR3, GTE_SXY0, GTE_SXY1, GTE_SXY2, GTE_SXYP,
    GTE_SZ0, GTE_SZ1, GTE_SZ2, GTE_SZ3, GTE_RGB0, GTE_RGB1, GTE_RGB2, GTE_RES1,
    GTE_MAC0, GTE_MAC1, GTE_MAC2, GTE_MAC3, GTE_IRGB, GTE_ORGB, GTE_LZCS, GTE_LZCR
};

// Control registers (ctc2/cfc2).
enum {
    GTE_R11R12, GTE_R13R21, GTE_R22R23, GTE_R31R32, GTE_R33, GTE_TRX, GTE_TRY, GTE_TRZ,
    GTE_L11L12, GTE_L13L21, GTE_L22L23, GTE_L31L32, GTE_L33, GTE_RBK, GTE_GBK, GTE_BBK,
    GTE_LR1LR2, GTE_LR3LG1, GTE_LG2LG3, GTE_LB1LB2, GTE_LB3, GTE_RFC, GTE_GFC, GTE_BFC,
    GTE_OFX, GTE_OFY, GTE_H, GTE_DQA, GTE_DQB, GTE_ZSF3, GTE_ZSF4, GTE_FLAG
};

// mvmva's arguments: the matrix, the vector and the translation vector.
#define GTE_MX_RT 0     // rotation
#define GTE_MX_LL 1     // light
#define GTE_MX_LC 2     // light colour
#define GTE_V_V0 0
#define GTE_V_V1 1
#define GTE_V_V2 2
#define GTE_V_IR 3      // IR1-IR3
#define GTE_CV_TR 0     // translation
#define GTE_CV_BK 1     // background colour
#define GTE_CV_FC 2     // far colour
#define GTE_CV_NONE 3

#ifndef GTE_C

#define gte_SetDataReg(reg, value) \
    __asm__ volatile("mtc2 %0, $%1" : : "r"(value), "i"(reg))
#define gte_GetDataReg(reg) \
    ({ int _gte_v; __asm__ volatile("mfc2 %0, $%1" : "=r"(_gte_v) : "i"(reg)); _gte_v; })
#define gte_SetCtrlReg(reg, value) \
    __asm__ volatile("ctc2 %0, $%1" : : "r"(value), "i"(reg))
#define gte_GetCtrlReg(reg) \
    ({ int _gte_v; __asm__ volatile("cfc2 %0, $%1" : "=r"(_gte_v) : "i"(reg)); _gte_v; })
// A data register from or to memory (a 32-bit word).
#define gte_LoadDataReg(reg, ptr) \
    __asm__ volatile("lwc2 $%1, 0(%0)" : : "r"(ptr), "i"(reg))
#define gte_StoreDataReg(reg, ptr) \
    __asm__ volatile("swc2 $%1, 0(%0)" : : "r"(ptr), "i"(reg) : "memory")

#define GTE_COMMAND(text) __asm__ volatile("nop\n\tnop\n\t" text)
#define gte_rtps() GTE_COMMAND("rtps")
#define gte_rtpt() GTE_COMMAND("rtpt")
#define gte_nclip() GTE_COMMAND("nclip")
#define gte_intpl() GTE_COMMAND("intpl")
#define GTE_STR(x) #x
// The arguments must be literal constants (or the GTE_* defines above).
#define gte_mvmva(sf, mx, v, cv, lm) \
    GTE_COMMAND("mvmva " GTE_STR(sf) ", " GTE_STR(mx) ", " GTE_STR(v) ", " GTE_STR(cv) ", " GTE_STR(lm))
// sf is 1 to shift the results right by 12 bits, 0 not to.
#define gte_sqr(sf) GTE_COMMAND("sqr " GTE_STR(sf))
#define gte_gpf(sf) GTE_COMMAND("gpf " GTE_STR(sf))
#define gte_gpl(sf) GTE_COMMAND("gpl " GTE_STR(sf))

#else

extern "C" {
void gte_SetDataReg(int reg, int value);
int gte_GetDataReg(int reg);
void gte_SetCtrlReg(int reg, int value);
int gte_GetCtrlReg(int reg);
void gte_LoadDataReg(int reg, const void *ptr);
void gte_StoreDataReg(int reg, void *ptr);
void gte_rtps(void);
void gte_rtpt(void);
void gte_nclip(void);
void gte_intpl(void);
void gte_mvmva(int sf, int mx, int v, int cv, int lm);
void gte_sqr(int sf);
void gte_gpf(int sf);
void gte_gpl(int sf);
}

#endif

// The rotation matrix from a MATRIX's nine shorts (five words).
#define gte_SetRotMatrixWords(m) \
    do { \
        int *_gte_m = (int *)(m); \
        gte_SetCtrlReg(GTE_R11R12, _gte_m[0]); \
        gte_SetCtrlReg(GTE_R13R21, _gte_m[1]); \
        gte_SetCtrlReg(GTE_R22R23, _gte_m[2]); \
        gte_SetCtrlReg(GTE_R31R32, _gte_m[3]); \
        gte_SetCtrlReg(GTE_R33, _gte_m[4]); \
    } while (0)

#endif
