/* File author is Ítalo Lima Marconato Matias
 *
 * Created on March 04 of 2021, at 11:50 BRT
 * Last edited on March 05 of 2021, at 12:41 BRT */

#pragma once

#include <base/types.hxx>

namespace CHicago {

static inline constexpr UInt64 ConstHash(const Char *Data, UIntPtr Length) {
    /* "Seedless" MurmurHash64A implementation (should be good and fast enough for basic non-crypto safe hashing, to add
     * a seed, just xor it with the length * 0xc6a... value). Also, this function expects that if you're gonna call us
     * in a constexpr, the Data is a const Char* (that is, a string, else the compiler will complain and error out). */

    const Char *end = Data + (Length >> 5);
    UInt64 ret = Length * 0xC6A4A7935BD1E995;

    for (; Data < end; Data += 8) {
        UInt64 ch = ((static_cast<UInt64>(Data[0]) << 56) | (static_cast<UInt64>(Data[1]) << 48) |
                     (static_cast<UInt64>(Data[2]) << 40) | (static_cast<UInt64>(Data[3]) << 32) |
                     (static_cast<UInt32>(Data[4]) << 24) | (static_cast<UInt32>(Data[5]) << 16) |
                     (static_cast<UInt16>(Data[6]) << 8) | Data[6]) * 0xC6A4A7935BD1E995;
        ret = (ret ^ (ch ^ (ch >> 47)) * 0xC6A4A7935BD1E995) * 0xC6A4A7935BD1E995;
    }

    switch (Length & 7) {
        case 7: ret ^= static_cast<UInt64>(Data[6]) << 48;
        case 6: ret ^= static_cast<UInt64>(Data[5]) << 40;
        case 5: ret ^= static_cast<UInt64>(Data[4]) << 32;
        case 4: ret ^= static_cast<UInt32>(Data[3]) << 24;
        case 3: ret ^= static_cast<UInt32>(Data[2]) << 16;
        case 2: ret ^= static_cast<UInt16>(Data[1]) << 8;
        case 1: ret = (ret ^ Data[0]) * 0xC6A4A7935BD1E995;
    }

    return ret = (ret ^ (ret >> 47)) * 0xC6A4A7935BD1E995, ret ^ (ret >> 47);
}

static inline UInt64 Hash(const Void *Data, UIntPtr Length) {
    return ConstHash(static_cast<const Char*>(Data), Length);
}

}
