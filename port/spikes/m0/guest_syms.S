/* What port/gen/guest_syms.S will look like: names bound to guest addresses. */
#if defined(_WIN32)
#define SYM(n) _##n
#else
#define SYM(n) n
#endif
    .globl SYM(g_GuestWord)
    .set   SYM(g_GuestWord), 0x801c98e0
    .globl SYM(g_ScratchWord)
    .set   SYM(g_ScratchWord), 0x1f800000
#if defined(__linux__)
    .section .note.GNU-stack,"",@progbits
#endif
