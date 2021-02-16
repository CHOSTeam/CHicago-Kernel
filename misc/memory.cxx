/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 07 of 2021, at 17:45 BRT
 * Last edited on February 15 of 2021 at 10:29 BRT */

#include <types.hxx>

namespace CHicago {

Void CopyMemory(Void *Buffer, const Void *Source, UIntPtr Length) {
    if (Buffer == Null || Source == Null || Buffer == Source || !Length) {
        return;
    }

    auto dst = reinterpret_cast<UInt8*>(Buffer);
    auto src = reinterpret_cast<const UInt8*>(Source);

    while (Length--) {
        *dst++ = *src++;
    }
}

Void SetMemory(Void *Buffer, UInt8 Value, UIntPtr Length) {
    if (Buffer == Null || !Length) {
        return;
    }

    for (auto buf = reinterpret_cast<UInt8*>(Buffer); Length--;) {
        *buf++ = Value;
    }
}

Void SetMemory32(Void *Buffer, UInt32 Value, UIntPtr Length) {
    /* We kind of depend on this function for properly filling lines/the whole screen with the same color... */

    if (Buffer == Null || !Length) {
        return;
    }

    for (auto buf = reinterpret_cast<UInt32*>(Buffer); Length--;) {
        *buf++ = Value;
    }
}

Void MoveMemory(Void *Buffer, const Void *Source, UIntPtr Length) {
    /* While copy doesn't handle intersecting regions, move should handle them, there are two ways of doing that:
     * Allocating a temp buffer, copying the source data into it and copying the data from the temp buffer into the
     * destination, or checking if the source buffer overlaps with the destination when copying forward, and, if that's
     * the case, copy backwards. We're going with the second way, as it's (probably) a good idea to make this work even
     * without the memory allocator. */

    auto buf = reinterpret_cast<UIntPtr>(Buffer), src = reinterpret_cast<UIntPtr>(Source);

    if (Buffer == Null || Source == Null || Buffer == Source || !Length) {
        return;
    } else if (buf > src && src + Length >= buf) {
        auto dst = &reinterpret_cast<UInt8*>(Buffer)[Length - 1];
        auto src = &reinterpret_cast<const UInt8*>(Source)[Length - 1];

        while (Length--) {
            *dst-- = *src--;
        }
    } else {
        CopyMemory(Buffer, Source, Length);
    }
}

Boolean CompareMemory(const Void *const Left, const Void *const Right, UIntPtr Length) {
    if (Left == Null || Right == Null || Left == Right || !Length) {
        return False;
    }

    for (auto m1 = reinterpret_cast<const UInt8*>(Left), m2 = reinterpret_cast<const UInt8*>(Right); Length--;) {
        if (*m1++ != *m2++) {
            return False;
        }
    }

    return True;
}

}
