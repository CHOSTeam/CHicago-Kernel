/* File author is Ítalo Lima Marconato Matias
 *
 * Created on April 10 of 2021, at 12:41 BRT
 * Last edited on April 10 of 2021, at 13:10 BRT */

#pragma once

#include <base/iterator.hxx>

/* BitOp works on different integer sizes depending on the host arch, and as such, it uses different builtin functions,
 * and we use a macro to avoid repetition (of determining which one to use). */

#ifdef _LP64
#define POPCNT(x) __builtin_popcountll(x)
#define BSWP(x) __builtin_bswap64(x)
#define CLZ(x) __builtin_clzll(x)
#define CTZ(x) __builtin_ctzll(x)
#define SIZE 64
#else
#define POPCNT(x) __builtin_popcount(x)
#define BSWP(x) __builtin_bswap32(x)
#define CLZ(x) __builtin_clz(x)
#define CTZ(x) __builtin_ctz(x)
#define SIZE 32
#endif

namespace CHicago {

class BitOp {
public:
    /* Custom iterator, so that we can use a ranged for loop on the set bits (this uses ctz, so it's more optimized
     * than a dummy/naive loop and test). */

    class Iterator {
    public:
        using Tag = CHicago::Iterator::Forward;
        using Val = const Int32;
        using Ptr = const Int32*;
        using Ref = const Int32&;

        inline constexpr Iterator(UIntPtr State) : State(State), Bit(ScanForward(State)) {}

        inline constexpr Boolean operator ==(Iterator &Other) const { return State == Other.State; }
        inline constexpr Boolean operator !=(Iterator &Other) const { return State != Other.State; }

        inline constexpr Ref operator *() const { return Bit; }
        inline constexpr Iterator &operator ++() { return *this = State ^ (State & -State); }
        inline constexpr const Iterator operator ++(Int32) { Iterator it = *this; ++*this; return it; }
    private:
        UIntPtr State;
        Int32 Bit;
    };

    /* Iterator wrapper (for ::begin and ::end). */

    class IteratorWrapper {
    public:
        constexpr IteratorWrapper(UIntPtr State) : State(State) {}
        inline constexpr Iterator begin() const { return State; }
        inline constexpr Iterator end() const { return 0; }
    private:
        UIntPtr State;
    };

    /* Utility functions. */

    static inline constexpr UIntPtr GetBit(UInt8 Bit) { return (UIntPtr)1 << Bit; }
    static inline constexpr UIntPtr GetMask(UInt8 To) { return ((UIntPtr)2 << To) - 1; }
    static inline constexpr UIntPtr GetMask(UInt8 From, UInt8 To) { return GetMask(To - From) << From; }
    static inline constexpr Int32 Count(UIntPtr State) { return POPCNT(State); }
    static inline constexpr UIntPtr Swap(UIntPtr State) { return BSWP(State); }
    static inline constexpr Int32 GetLeadingZeroes(UIntPtr State) { return !State ? SIZE : CLZ(State); }
    static inline constexpr Int32 ScanForward(UIntPtr State) { return !State ? SIZE : CTZ(State); }
    static inline constexpr Int32 ScanReverse(UIntPtr State) { return !State ? SIZE : CLZ(State) ^ (SIZE - 1); }

    template<typename... T> static inline constexpr Void Set(UIntPtr &State, T... Args) {
        Int32 bits[] { Args... }; for (Int32 i : bits) State |= GetBit(i);
    }

    template<typename... T> static inline constexpr Void Unset(UIntPtr &State, T... Args) {
        Int32 bits[] { Args... }; for (Int32 i : bits) State &= ~GetBit(i);
    }

    template<typename... T> static inline constexpr Void Toggle(UIntPtr &State, T... Args) {
        Int32 bits[] { Args... }; for (Int32 i : bits) State ^= GetBit(i);
    }

    template<typename... T> static inline constexpr Boolean Test(UIntPtr State, T... Args) {
        Int32 bits[] { Args... }; for (Int32 i : bits) if (!(State & GetBit(i))) return False;
        return True;
    }

    /* Same as above, but for ranges. */

    static inline constexpr UIntPtr GetBits(UIntPtr State, UInt8 From, UInt8 To) {
        return (State >> From) & GetMask(To - From);
    }

    static inline constexpr Void SetRange(UIntPtr &State, UInt8 From, UInt8 To) { State |= GetMask(From, To); }
    static inline constexpr Void UnsetRange(UIntPtr &State, UInt8 From, UInt8 To) { State &= ~GetMask(From, To); }
    static inline constexpr Void ToggleRange(UIntPtr &State, UInt8 From, UInt8 To) { State ^= GetMask(From, To); }

    static inline constexpr Boolean TestRange(UIntPtr State, UInt8 From, UInt8 To) {
        return (State & GetMask(From, To)) == GetMask(From, To);
    }
};

/* Don't let the temp macros escape this file's scope. */

#undef SIZE
#undef CTZ
#undef CLZ
#undef BSWP
#undef POPCNT

}
