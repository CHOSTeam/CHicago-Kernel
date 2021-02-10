/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 07 of 2021, at 17:45 BRT
 * Last edited on February 10 of 2021 at 11:18 BRT */

#include <types.hxx>

Void CopyMemory(Void *Buffer, const Void *Source, UIntPtr Length) {
    if (Buffer == Null || Source == Null || Buffer == Source || !Length) {
        return;
    }

    UInt8 *dst = reinterpret_cast<UInt8*>(Buffer);
    const UInt8 *src = reinterpret_cast<const UInt8*>(Source);

    while (Length--) {
        *dst++ = *src++;
    }
}

Void SetMemory(Void *Buffer, UInt8 Value, UIntPtr Length) {
    if (Buffer == Null || !Length) { 
        return;
    }

    for (UInt8 *buf = reinterpret_cast<UInt8*>(Buffer); Length--;) {
        *buf++ = Value;
    }
}

Void MoveMemory(Void *Buffer, const Void *Source, UIntPtr Length) {
    /* While copy doesn't handle intersecting regions, move should handle them, there are two ways of doing that:
     * Allocating a temp buffer, copying the source data into it and copying the data from the temp buffer into the
     * destination, or checking if the source buffer overlaps with the destination when copying forward, and, if that's
     * the case, copy backwards. We're going with the second way, as it's (probably) a good idea to make this work even
     * without the memory allocator. */

    UIntPtr buf = reinterpret_cast<UIntPtr>(Buffer), src = reinterpret_cast<UIntPtr>(Source);

    if (Buffer == Null || Source == Null || Buffer == Source || !Length) {
        return;
    } else if (buf > src && src + Length >= buf) {
        UInt8 *dst = &reinterpret_cast<UInt8*>(Buffer)[Length - 1];
        const UInt8 *src = &reinterpret_cast<const UInt8*>(Source)[Length - 1];

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

    for (const UInt8 *m1 = reinterpret_cast<const UInt8*>(Left),
                     *m2 = reinterpret_cast<const UInt8*>(Right); Length--;) {
        if (*m1++ != *m2++) {
            return False;
        }
    }

    return True;
}
