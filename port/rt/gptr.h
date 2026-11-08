// SPDX-License-Identifier: GPL-2.0-or-later
// Pointers as guest data holds them: 4 bytes, the PlayStation address (docs/port.md).
//
// Guest memory is a 4 GB window at rt::base, so a guest address a is at host address
// base + a: RAM at base + 0x80000000, the scratchpad at base + 0x1f800000. On i686 the
// window is the address space itself and base is 0. Address 0 is the null pointer.
//
// game_pointer<T> is a guest word that holds a pointer to T. It converts to and from T*,
// so code reads like it did with T *; (int)p gives the PlayStation address, as the console
// did, and rt::to_guest and rt::from_guest convert a host pointer and an address.
// game_function<F> is a guest word that holds a function's PlayStation address, which
// rt::callTarget turns into the host function (rt/functions.cpp).
//
// Game code is built without the C++ library's headers (-nostdlibinc), so this header
// uses compiler builtins only.
#pragma once

namespace rt {

using u32 = __UINT32_TYPE__;
using uptr = __UINTPTR_TYPE__;
using iptr = __PTRDIFF_TYPE__;

namespace detail {
template <bool B, class T = void>
struct enable_if {};
template <class T>
struct enable_if<true, T> {
    using type = T;
};
template <bool B, class T = void>
using enable_if_t = typename enable_if<B, T>::type;
}  // namespace detail

#if __SIZEOF_POINTER__ == 4
inline constexpr uptr base = 0;
#else
extern uptr base;  // set by mapGuestMemory
#endif

[[noreturn]] void notInWindow(const volatile void *pointer);

// The host address of guest address a. 0 isn't special here: callers pass real addresses.
inline void *guest(u32 address)
{
    return reinterpret_cast<void *>(base + address);
}

template <class T>
inline T *guest(u32 address)
{
    return reinterpret_cast<T *>(base + address);
}

// The guest address of p, 0 for null. p must point into the window.
inline u32 to_guest(const volatile void *pointer)
{
    if (!pointer) {
        return 0;
    }
    uptr offset = reinterpret_cast<uptr>(pointer) - base;
#if __SIZEOF_POINTER__ != 4
    if (offset >> 32) {
        notInWindow(pointer);
    }
#endif
    return static_cast<u32>(offset);
}

// The host address of guest address a, null for 0: (T *)a as the console read it.
inline void *from_guest(u32 address)
{
    return address ? reinterpret_cast<void *>(base + address) : nullptr;
}

// Guest memory for data the port adds, that guest pointers reach (rt/memmap.cpp): size
// zeroed bytes, never freed; a copy of s there.
void *guestAlloc(__SIZE_TYPE__ size);
char *guestString(const char *s);

// The host function at guest address a, for the overlay loaded now (rt/functions.cpp).
// A function the PlayStation didn't have gets an address when first stored.
void *callTarget(u32 address);
u32 functionAddress(void *function);

}  // namespace rt

template <class T>
class game_pointer {
    static_assert(!__is_function(T), "a function pointer in guest data is a game_function");

public:
    game_pointer() = default;  // trivial, like the raw pointer it replaces
    game_pointer(T *pointer) : address_(rt::to_guest(pointer)) {}
    template <class U, class = rt::detail::enable_if_t<__is_convertible_to(U *, T *) && !__is_same(U, T)>>
    game_pointer(game_pointer<U> other) : game_pointer(static_cast<T *>(other.get()))
    {
    }

    static game_pointer fromGuest(rt::u32 address)
    {
        game_pointer p;
        p.address_ = address;
        return p;
    }

    T *get() const { return address_ ? reinterpret_cast<T *>(rt::base + address_) : nullptr; }
    rt::u32 raw() const { return address_; }

    operator T *() const { return get(); }
    // (int)p: the PlayStation address, as the console gave it.
    template <class I, class = rt::detail::enable_if_t<__is_integral(I)>>
    explicit operator I() const
    {
        return static_cast<I>(address_);
    }
    template <class U, class = rt::detail::enable_if_t<!__is_same(U, T)>>
    explicit operator U *() const
    {
        return (U *)get();
    }

    template <class U = T, class = rt::detail::enable_if_t<!__is_void(U)>>
    U *operator->() const
    {
        return get();
    }
    template <class U = T, class = rt::detail::enable_if_t<!__is_void(U)>>
    U &operator*() const
    {
        return *get();
    }
    template <class U = T, class = rt::detail::enable_if_t<!__is_void(U)>>
    U &operator[](rt::iptr i) const
    {
        return get()[i];
    }

    // Arithmetic wraps at 32 bits, as the console's did.
    template <class U = T, class = rt::detail::enable_if_t<!__is_void(U)>>
    game_pointer &operator+=(rt::iptr n)
    {
        address_ += static_cast<rt::u32>(n * static_cast<rt::iptr>(sizeof(U)));
        return *this;
    }
    template <class U = T, class = rt::detail::enable_if_t<!__is_void(U)>>
    game_pointer &operator-=(rt::iptr n)
    {
        return *this += -n;
    }
    template <class U = T, class = rt::detail::enable_if_t<!__is_void(U)>>
    game_pointer &operator++()
    {
        return *this += 1;
    }
    template <class U = T, class = rt::detail::enable_if_t<!__is_void(U)>>
    game_pointer &operator--()
    {
        return *this += -1;
    }
    template <class U = T, class = rt::detail::enable_if_t<!__is_void(U)>>
    game_pointer operator++(int)
    {
        game_pointer old = *this;
        *this += 1;
        return old;
    }
    template <class U = T, class = rt::detail::enable_if_t<!__is_void(U)>>
    game_pointer operator--(int)
    {
        game_pointer old = *this;
        *this += -1;
        return old;
    }

private:
    rt::u32 address_;
};

static_assert(sizeof(game_pointer<int>) == 4 && alignof(game_pointer<int>) == 4);
static_assert(__is_trivially_copyable(game_pointer<int>) && __is_standard_layout(game_pointer<int>));

template <class F>
class game_function;

template <class R, class... A>
class game_function<R(A...)> {
public:
    using Function = R (*)(A...);

    game_function() = default;
    game_function(Function f) : address_(f ? rt::functionAddress(reinterpret_cast<void *>(f)) : 0) {}

    static game_function fromGuest(rt::u32 address)
    {
        game_function f;
        f.address_ = address;
        return f;
    }

    Function get() const { return address_ ? reinterpret_cast<Function>(rt::callTarget(address_)) : nullptr; }
    rt::u32 raw() const { return address_; }
    operator Function() const { return get(); }
    R operator()(A... args) const { return get()(args...); }

    template <class G, class = rt::detail::enable_if_t<__is_function(G)>>
    explicit operator G *() const  // a cast to another signature, as decomp code does
    {
        return reinterpret_cast<G *>(get());
    }

private:
    rt::u32 address_;
};

static_assert(sizeof(game_function<void()>) == 4 && __is_trivially_copyable(game_function<void()>));
