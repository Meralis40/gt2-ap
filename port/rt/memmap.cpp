// SPDX-License-Identifier: GPL-2.0-or-later
#include "memmap.h"
#include "gptr.h"
#include "guest_data.h"
#include "rt.h"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#include <sys/mman.h>
#include <ucontext.h>
#include <unistd.h>
#endif

#if UINTPTR_MAX != 0xffffffffu
#define RT_WINDOW 1  // guest memory is a window at a base the system picks
#endif

namespace rt {

#if RT_WINDOW
std::uintptr_t base;
#endif

namespace {

// A host address, and its guest address when it's in the window.
void describe(char *out, std::size_t size, std::uintptr_t address)
{
#if RT_WINDOW
    if (address - base < (std::uintptr_t{1} << 32)) {
        std::snprintf(out, size, "%08x (guest)", static_cast<unsigned>(address - base));
        return;
    }
#endif
#if defined(_WIN64)
    // In the executable: as llvm-symbolizer reads it, at its preferred base.
    auto image = reinterpret_cast<std::uintptr_t>(GetModuleHandleA(nullptr));
    if (address - image < 0x10000000) {
        std::snprintf(out, size, "%llx (exe)", static_cast<unsigned long long>(address - image + 0x140000000));
        return;
    }
#endif
#if !defined(_WIN32)
    // In a module, the executable among them: its offset there, for llvm-symbolizer.
    Dl_info info;
    if (dladdr(reinterpret_cast<void *>(address), &info) && info.dli_fname) {
        const char *name = std::strrchr(info.dli_fname, '/');
        std::snprintf(out, size, "%s+%#lx", name ? name + 1 : info.dli_fname,
                      static_cast<unsigned long>(address - reinterpret_cast<std::uintptr_t>(info.dli_fbase)));
        return;
    }
#endif
    std::snprintf(out, size, "%p", reinterpret_cast<void *>(address));
}

// Frame pointers are on: walks them and prints the return addresses, for llvm-symbolizer.
void printBacktrace(std::uintptr_t frame)
{
    char at[80];

    for (int i = 0; i < 24 && frame > 0x10000; i++) {
        auto *words = reinterpret_cast<std::uintptr_t *>(frame);
        describe(at, sizeof at, words[1]);
        std::fprintf(stderr, "  %s\n", at);
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
#if RT_WINDOW
#define REG_IP Rip
#define REG_FP Rbp
#define REG_SP Rsp
#else
#define REG_IP Eip
#define REG_FP Ebp
#define REG_SP Esp
#endif

// Reserves size bytes at address, or anywhere for 0, and commits the first commit bytes.
void *reserve(std::uintptr_t address, std::size_t size)
{
    return VirtualAlloc(reinterpret_cast<void *>(address), size, MEM_RESERVE, PAGE_NOACCESS);
}

bool commit(std::uintptr_t address, std::size_t size)
{
    return VirtualAlloc(reinterpret_cast<void *>(address), size, MEM_COMMIT, PAGE_READWRITE) != nullptr;
}

LONG CALLBACK onFault(EXCEPTION_POINTERS *info)
{
    if (info->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    // Raw output: a wild write may have hit the C library's own state.
    char at[80], accessed[80], line[240];
    DWORD written;
    describe(at, sizeof at, info->ContextRecord->REG_IP);
    describe(accessed, sizeof accessed, info->ExceptionRecord->ExceptionInformation[1]);
    int size = wsprintfA(line, "fatal: fault at %s, %s %s\r\n", at,
                         info->ExceptionRecord->ExceptionInformation[0] == 1 ? "writing" : "reading", accessed);
    WriteFile(GetStdHandle(STD_ERROR_HANDLE), line, size, &written, nullptr);
#if RT_WINDOW
    // x64 code has unwind tables, the system's too: walk with them, naming each module.
    CONTEXT walk = *info->ContextRecord;
    for (int i = 0; i < 24 && walk.Rip; i++) {
        DWORD64 imageBase;
        PRUNTIME_FUNCTION entry = RtlLookupFunctionEntry(walk.Rip, &imageBase, nullptr);
        char module[MAX_PATH] = "?";
        HMODULE handle;
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               reinterpret_cast<LPCSTR>(walk.Rip), &handle)) {
            GetModuleFileNameA(handle, module, sizeof module);
        }
        describe(at, sizeof at, walk.Rip);
        const char *name = std::strrchr(module, '\\');
        size = wsprintfA(line, "  %s +%lx %s\r\n", at, static_cast<unsigned long>(walk.Rip - imageBase),
                         name ? name + 1 : module);
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), line, size, &written, nullptr);
        if (!entry) {
            walk.Rip = *reinterpret_cast<DWORD64 *>(walk.Rsp);  // a leaf: the return address is on top
            walk.Rsp += 8;
            continue;
        }
        void *handlerData;
        DWORD64 establisher;
        RtlVirtualUnwind(UNW_FLAG_NHANDLER, imageBase, walk.Rip, entry, &walk, &handlerData, &establisher, nullptr);
    }
    TerminateProcess(GetCurrentProcess(), 3);
#endif
    auto frame = static_cast<std::uintptr_t>(info->ContextRecord->REG_FP);
    for (int i = 0; i < 24 && frame > 0x10000 && !IsBadReadPtr(reinterpret_cast<void *>(frame), 2 * sizeof frame);
         i++) {
        auto *words = reinterpret_cast<std::uintptr_t *>(frame);
        describe(at, sizeof at, words[1]);
        size = wsprintfA(line, "  %s\r\n", at);
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
void *reserve(std::uintptr_t address, std::size_t size)
{
    int flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | (address ? MAP_FIXED_NOREPLACE : 0);
    void *p = mmap(reinterpret_cast<void *>(address), size, PROT_NONE, flags, -1, 0);
    return p == MAP_FAILED ? nullptr : p;
}

bool commit(std::uintptr_t address, std::size_t size)
{
    return mprotect(reinterpret_cast<void *>(address), size, PROT_READ | PROT_WRITE) == 0;
}

void onFault(int, siginfo_t *info, void *context)
{
    auto &regs = static_cast<ucontext_t *>(context)->uc_mcontext.gregs;
#if RT_WINDOW
    auto ip = regs[REG_RIP], fp = regs[REG_RBP];
#else
    auto ip = regs[REG_EIP], fp = regs[REG_EBP];
#endif
    char at[80], accessed[80];
    describe(at, sizeof at, static_cast<std::uintptr_t>(ip));
    describe(accessed, sizeof accessed, reinterpret_cast<std::uintptr_t>(info->si_addr));
    std::fprintf(stderr, "fatal: fault at %s, accessing %s\n", at, accessed);
    printBacktrace(static_cast<std::uintptr_t>(fp));
    _exit(3);
}
#endif

}  // namespace

void mapGuestMemory()
{
#if RT_WINDOW
    void *window = reserve(0, std::size_t{1} << 32);
    if (!window) {
        Rt_Fatal("cannot reserve the 4 GB guest window");
    }
    base = reinterpret_cast<std::uintptr_t>(window);
#endif
    std::uintptr_t ram = reinterpret_cast<std::uintptr_t>(guest(kGuestBase));
    std::uintptr_t scratch = reinterpret_cast<std::uintptr_t>(guest(kScratchBase));
#if !RT_WINDOW
    if (!reserve(ram, kGuestReserve) || !reserve(scratch, kScratchSize)) {
        Rt_Fatal("cannot map guest memory at %08x and %08x", kGuestBase, kScratchBase);
    }
#endif
    if (!commit(ram, kGuestRam) || !commit(scratch, kScratchSize) ||
        !commit(reinterpret_cast<std::uintptr_t>(guest(kPortDataBase)), kPortDataSize)) {
        Rt_Fatal("cannot commit guest memory");
    }
#if RT_WINDOW
    if (!commit(reinterpret_cast<std::uintptr_t>(guest(kGameStackBase)), kGameStackSize)) {
        Rt_Fatal("cannot commit the game's stack");
    }
#endif
    bindGuestData();
}

void bindGuestData()
{
    for (const GuestData *d = kGuestData; d->slot; d++) {
        *d->slot = reinterpret_cast<std::uintptr_t>(guest(d->address));
    }
}

void *guestAlloc(__SIZE_TYPE__ size)
{
    static std::size_t used;
    std::size_t at = (used + 15) & ~std::size_t{15};

    if (size > kPortDataSize - at) {
        Rt_Fatal("out of port data in guest memory (%zu bytes asked)", size);
    }
    used = at + size;
    return guest(kPortDataBase + static_cast<std::uint32_t>(at));  // committed zeroed, never freed
}

char *guestString(const char *s)
{
    std::size_t n = std::strlen(s) + 1;
    return static_cast<char *>(std::memcpy(guestAlloc(n), s, n));
}

[[noreturn]] void notInWindow(const volatile void *pointer)
{
    std::fprintf(stderr, "fatal: host address %p stored as a guest pointer (the window is at %p), from:\n",
                 const_cast<const void *>(pointer), guest(0));
#if defined(_WIN32)
    std::fprintf(stderr, "  called from %p, the executable at %p\n", __builtin_return_address(0),
                 static_cast<void *>(GetModuleHandleA(nullptr)));
#endif
    printBacktrace(reinterpret_cast<std::uintptr_t>(__builtin_frame_address(0)));
    std::_Exit(1);
}

#if !RT_WINDOW
void runOnGameStack(void (*fn)())
{
    fn();
}
#elif defined(_WIN32)
extern "C" void Rt_CallOnStack(void *top, void (*fn)());  // rt/stack_win64.S

void runOnGameStack(void (*fn)())
{
    // The stack limits in the TEB are what stack probes and unwinding (longjmp) check.
    auto *tib = reinterpret_cast<NT_TIB *>(NtCurrentTeb());
    void *oldBase = tib->StackBase;
    void *oldLimit = tib->StackLimit;
    auto *bottom = static_cast<char *>(guest(kGameStackBase));
    tib->StackBase = bottom + kGameStackSize;
    tib->StackLimit = bottom;
    Rt_CallOnStack(bottom + kGameStackSize, fn);
    tib->StackBase = oldBase;
    tib->StackLimit = oldLimit;
}
#else
namespace {
ucontext_t g_HostContext;
ucontext_t g_GameContext;
}  // namespace

void runOnGameStack(void (*fn)())
{
    getcontext(&g_GameContext);
    g_GameContext.uc_stack.ss_sp = guest(kGameStackBase);
    g_GameContext.uc_stack.ss_size = kGameStackSize;
    g_GameContext.uc_link = &g_HostContext;
    makecontext(&g_GameContext, fn, 0);
    swapcontext(&g_HostContext, &g_GameContext);
}
#endif

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
