/* SPDX-License-Identifier: GPL-2.0-or-later */
/* void Rt_CallOnStack(void *top, void (*fn)(void)): calls fn with the stack pointer at top
   (rt/memmap.cpp), and returns on the caller's stack. Microsoft x64 convention. */
    .text
    .globl Rt_CallOnStack
    .def Rt_CallOnStack
    .scl 2
    .type 32
    .endef
    .seh_proc Rt_CallOnStack
Rt_CallOnStack:
    pushq %rbp
    .seh_pushreg %rbp
    movq %rsp, %rbp
    .seh_setframe %rbp, 0
    .seh_endprologue
    andq $-16, %rcx
    movq %rcx, %rsp
    subq $32, %rsp          /* the callee's home space */
    callq *%rdx
    movq %rbp, %rsp
    popq %rbp
    retq
    .seh_endproc
