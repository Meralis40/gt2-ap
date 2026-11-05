// SPDX-License-Identifier: GPL-2.0-or-later
#include "memmap.h"
#include "rt.h"

#include <csignal>
#include <cstdio>
#include <cstdlib>

#if defined(_WIN32)
#include <windows.h>
#else
#include <csignal>
#include <sys/mman.h>
#include <ucontext.h>
#include <unistd.h>
#endif

namespace rt {

namespace {

// Frame pointers are on: walks them and prints the return addresses, for llvm-symbolizer.
void printBacktrace(std::uintptr_t frame)
{
    for (int i = 0; i < 24 && frame > 0x10000; i++) {
        auto *words = reinterpret_cast<std::uintptr_t *>(frame);
        std::fprintf(stderr, "  %08lx\n", static_cast<unsigned long>(words[1]));
        if (words[0] <= frame) {
            break;
        }
        frame = words[0];
    }
}

void onAbort(int)
{
    std::fputs("fatal: abort\n", stderr);
    printBacktrace(reinterpret_cast<std::uintptr_t>(__builtin_frame_address(0)));
    std::_Exit(4);
}

#if defined(_WIN32)
bool mapFixed(std::uintptr_t address, std::size_t reserve, std::size_t commit)
{
    void *want = reinterpret_cast<void *>(address);

    return VirtualAlloc(want, reserve, MEM_RESERVE, PAGE_NOACCESS) == want &&
           VirtualAlloc(want, commit, MEM_COMMIT, PAGE_READWRITE) == want;
}

LONG CALLBACK onFault(EXCEPTION_POINTERS *info)
{
    if (info->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    // Raw output: a wild write may have hit the C library's own state.
    char line[160];
    DWORD written;
    int size = wsprintfA(line, "fatal: fault at %08lx, %s %08lx, ebp %08lx esp %08lx\r\n", info->ContextRecord->Eip,
                         info->ExceptionRecord->ExceptionInformation[0] == 1 ? "writing" : "reading",
                         info->ExceptionRecord->ExceptionInformation[1], info->ContextRecord->Ebp,
                         info->ContextRecord->Esp);
    WriteFile(GetStdHandle(STD_ERROR_HANDLE), line, size, &written, nullptr);
    auto frame = static_cast<std::uintptr_t>(info->ContextRecord->Ebp);
    for (int i = 0; i < 24 && frame > 0x10000 && !IsBadReadPtr(reinterpret_cast<void *>(frame), 8); i++) {
        auto *words = reinterpret_cast<std::uintptr_t *>(frame);
        size = wsprintfA(line, "  %08lx\r\n", static_cast<unsigned long>(words[1]));
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), line, size, &written, nullptr);
        if (words[0] <= frame) {
            break;
        }
        frame = words[0];
    }
    TerminateProcess(GetCurrentProcess(), 3);
    return EXCEPTION_CONTINUE_SEARCH;
}
#else
bool mapFixed(std::uintptr_t address, std::size_t reserve, std::size_t commit)
{
    void *want = reinterpret_cast<void *>(address);

    return mmap(want, reserve, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0) == want &&
           mprotect(want, commit, PROT_READ | PROT_WRITE) == 0;
}

void onFault(int, siginfo_t *info, void *context)
{
    auto eip = static_cast<ucontext_t *>(context)->uc_mcontext.gregs[REG_EIP];

    std::fprintf(stderr, "fatal: fault at %08lx, accessing %08lx\n", static_cast<unsigned long>(eip),
                 reinterpret_cast<unsigned long>(info->si_addr));
    printBacktrace(static_cast<ucontext_t *>(context)->uc_mcontext.gregs[REG_EBP]);
    _exit(3);
}
#endif

}  // namespace

void mapGuestMemory()
{
    if (!mapFixed(kGuestBase, kGuestReserve, kGuestRam)) {
        Rt_Fatal("cannot map guest RAM at %08x", kGuestBase);
    }
    if (!mapFixed(kScratchBase, kScratchSize, kScratchSize)) {
        Rt_Fatal("cannot map the scratchpad at %08x", kScratchBase);
    }
}

void installFaultHandler()
{
    std::signal(SIGABRT, onAbort);
#if defined(_WIN32)
    AddVectoredExceptionHandler(1, onFault);
#else
    struct sigaction action {};
    action.sa_sigaction = onFault;
    action.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &action, nullptr);
#endif
}

}  // namespace rt
