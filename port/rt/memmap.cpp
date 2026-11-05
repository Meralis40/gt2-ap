// SPDX-License-Identifier: GPL-2.0-or-later
#include "memmap.h"
#include "rt.h"

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
    std::fprintf(stderr, "fatal: fault at %08lx, %s %08lx\n", static_cast<unsigned long>(info->ContextRecord->Eip),
                 info->ExceptionRecord->ExceptionInformation[0] == 1 ? "writing" : "reading",
                 static_cast<unsigned long>(info->ExceptionRecord->ExceptionInformation[1]));
    std::_Exit(3);
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
