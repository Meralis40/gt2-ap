// M0 spike: see README.md.
#include <csetjmp>
#include <cstdint>
#include <cstdio>
#include <cstring>

#if defined(_WIN32)
#include <windows.h>
#else
#include <csignal>
#include <sys/mman.h>
#include <ucontext.h>
#include <unistd.h>
#endif

static_assert(sizeof(void *) == 4, "the port is a 32-bit build");

extern "C" {
extern volatile int g_GuestWord;
extern volatile int g_ScratchWord;
}

namespace {

constexpr uintptr_t kGuestBase = 0x80000000u;
constexpr size_t kGuestReserve = 16u << 20;
constexpr size_t kGuestRam = 2u << 20;
constexpr uintptr_t kScratchBase = 0x1f800000u;
constexpr size_t kScratchSize = 0x10000;

int g_failures;

void check(bool ok, const char *what)
{
    std::printf("%-44s %s\n", what, ok ? "ok" : "FAILED");
    if (!ok) {
        g_failures++;
    }
}

#if defined(_WIN32)
bool map_fixed(uintptr_t addr, size_t reserve, size_t commit)
{
    void *want = reinterpret_cast<void *>(addr);
    if (VirtualAlloc(want, reserve, MEM_RESERVE, PAGE_NOACCESS) != want) {
        return false;
    }
    return VirtualAlloc(want, commit, MEM_COMMIT, PAGE_READWRITE) == want;
}
#else
bool map_fixed(uintptr_t addr, size_t reserve, size_t commit)
{
    void *want = reinterpret_cast<void *>(addr);
    void *got = mmap(want, reserve, PROT_NONE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    return got == want && mprotect(want, commit, PROT_READ | PROT_WRITE) == 0;
}
#endif

// A missed code pointer: control lands in guest RAM, which isn't executable.
volatile uintptr_t g_faultAddress;
jmp_buf g_faultJump;

#if defined(_WIN32)
LONG CALLBACK on_fault(EXCEPTION_POINTERS *info)
{
    if (info->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    g_faultAddress = info->ContextRecord->Eip;
    longjmp(g_faultJump, 1);
}
void install_fault_handler()
{
    AddVectoredExceptionHandler(1, on_fault);
}
#else
void on_fault(int, siginfo_t *, void *context)
{
    g_faultAddress = static_cast<ucontext_t *>(context)->uc_mcontext.gregs[REG_EIP];
    siglongjmp(*reinterpret_cast<sigjmp_buf *>(&g_faultJump), 1);
}
void install_fault_handler()
{
    struct sigaction action {};
    action.sa_sigaction = on_fault;
    action.sa_flags = SA_SIGINFO | SA_NODEFER;
    sigaction(SIGSEGV, &action, nullptr);
}
#endif

// The overlay dispatch: setjmp lives in one frame that stays alive, and the
// longjmp value carries the entry point, as Overlay_Dispatch's int does.
jmp_buf g_dispatchJump;
int g_entriesRun;

void entry_b()
{
    g_entriesRun |= 2;
    longjmp(g_dispatchJump, 0);  // setjmp turns this into 1: stop
}

void entry_a()
{
    g_entriesRun |= 1;
    longjmp(g_dispatchJump, reinterpret_cast<int>(&entry_b));
}

void dispatch()
{
    volatile int next = reinterpret_cast<int>(&entry_a);
    for (;;) {
        int value = setjmp(g_dispatchJump);
        if (value == 1) {
            return;
        }
        if (value != 0) {
            next = value;
        }
        reinterpret_cast<void (*)()>(next)();
    }
}

}  // namespace

int main()
{
    bool ram = map_fixed(kGuestBase, kGuestReserve, kGuestRam);
    check(ram, "reserve guest RAM at 0x80000000");
    bool scratch = map_fixed(kScratchBase, kScratchSize, kScratchSize);
    check(scratch, "reserve scratchpad at 0x1F800000");
    if (!ram || !scratch) {
        return 1;
    }

    auto *word = reinterpret_cast<volatile int *>(0x801c98e0u);
    g_GuestWord = 0x12345678;
    check(*word == 0x12345678, "absolute symbol binds to guest RAM");
    check(reinterpret_cast<uintptr_t>(&g_GuestWord) == 0x801c98e0u,
          "absolute symbol address is exact");
    g_ScratchWord = 42;
    check(*reinterpret_cast<volatile int *>(0x1f800000u) == 42,
          "absolute symbol binds to scratchpad");

    // A two-packet OT: tag = length << 24 | next, terminated by 0xffffff.
    auto *packets = reinterpret_cast<uint32_t *>(0x80100000u);
    packets[0] = (1u << 24) | 0x100010u;
    packets[1] = 0xaaaaaaaau;
    packets[4] = (1u << 24) | 0xffffffu;
    packets[5] = 0xbbbbbbbbu;
    uint32_t tag = reinterpret_cast<uintptr_t>(packets) & 0xffffffu;
    uint32_t sum = 0;
    while ((tag & 0xffffffu) != 0xffffffu) {
        auto *packet = reinterpret_cast<uint32_t *>((tag & 0xffffffu) | 0x80000000u);
        sum ^= packet[1];
        tag = packet[0];
    }
    check(sum == (0xaaaaaaaau ^ 0xbbbbbbbbu), "24-bit OT links resolve");

    dispatch();
    check(g_entriesRun == 3, "setjmp trampoline runs both entries");

    install_fault_handler();
    if (setjmp(g_faultJump) == 0) {
        reinterpret_cast<void (*)()>(0x8001dd98u)();
        check(false, "jump into guest RAM faults");
    } else {
        check(g_faultAddress == 0x8001dd98u, "jump into guest RAM reports its address");
    }

    std::printf("failures: %d\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
