/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 07 of 2021, at 17:45 BRT
 * Last edited on February 19 of 2021 at 14:03 BRT */

#include <simd.hxx>

namespace CHicago {

Void CopyMemory(Void *Buffer, const Void *Source, UIntPtr Length) {
    if (Buffer == Null || Source == Null || Buffer == Source || !Length) {
        return;
    }

    auto dst = static_cast<UInt8*>(Buffer);
    auto src = static_cast<const UInt8*>(Source);

#ifdef NO_256_SIMD
    /* On some cases (like x86 without AVX2), we can't use 256-bit SIMD operations (or we can/GCC may let us do it, but
     * it will be slower), instead, let's use 128-bit SIMD operations. */

    while (Length >= 32) {
        SIMD::StoreUnaligned(dst, SIMD::LoadUnalignedI16(src));
        SIMD::StoreUnaligned(dst + 16, SIMD::LoadUnalignedI16(src + 16));
        Length -= 32;
        dst += 32;
        src += 32;
    }

    while (Length >= 16) {
        SIMD::StoreUnaligned(dst, SIMD::LoadUnalignedI16(src));
        Length -= 16;
        dst += 16;
        src += 16;
    }
#else
    /* And yeah, here we just copy twice as much per loop. */

    while (Length >= 64) {
        SIMD::StoreUnaligned(dst, SIMD::LoadUnalignedI32(src));
        SIMD::StoreUnaligned(dst + 32, SIMD::LoadUnalignedI32(src + 32));
        Length -= 64;
        dst += 64;
        src += 64;
    }

    while (Length >= 32) {
        SIMD::StoreUnaligned(dst, SIMD::LoadUnalignedI32(src));
        Length -= 32;
        dst += 32;
        src += 32;
    }

    while (Length >= 16) {
        SIMD::StoreUnaligned(dst, SIMD::LoadUnalignedI16(src));
        Length -= 16;
        dst += 16;
        src += 16;
    }
#endif

    while (Length--) {
        *dst++ = *src++;
    }
}

Void SetMemory(Void *Buffer, UInt8 Value, UIntPtr Length) {
    if (Buffer == Null || !Length) {
        return;
    }

    /* This is like the CopyMemory function, but we don't need any reads here, just writes. */

    auto dst = static_cast<UInt8*>(Buffer);
    Int64x2 val = UInt8x16 { Value, Value, Value, Value, Value, Value, Value, Value,
                             Value, Value, Value, Value, Value, Value, Value, Value };

#ifdef NO_256_SIMD
    /* Also, we can pre-init one variable containing Int64x2 (or x4 on 256-bits) of our value. */

    while (Length >= 32) {
        SIMD::StoreUnaligned(dst, val);
        SIMD::StoreUnaligned(dst + 16, val);
        Length -= 32;
        dst += 32;
    }

    while (Length >= 16) {
        SIMD::StoreUnaligned(dst, val);
        Length -= 16;
        dst += 16;
    }
#else
    Int64x4 val2 = UInt8x32 { Value, Value, Value, Value, Value, Value, Value, Value,
                              Value, Value, Value, Value, Value, Value, Value, Value,
                              Value, Value, Value, Value, Value, Value, Value, Value,
                              Value, Value, Value, Value, Value, Value, Value, Value };

    while (Length >= 64) {
        SIMD::StoreUnaligned(dst, val2);
        SIMD::StoreUnaligned(dst + 32, val2);
        Length -= 64;
        dst += 64;
    }

    while (Length >= 32) {
        SIMD::StoreUnaligned(dst, val2);
        Length -= 32;
        dst += 32;
    }

    while (Length >= 16) {
        SIMD::StoreUnaligned(dst, val);
        Length -= 16;
        dst += 16;
    }
#endif

    while (Length--) {
        *dst++ = Value;
    }
}

Void SetMemory32(Void *Buffer, UInt32 Value, UIntPtr Length) {
    /* This is like the CopyMemory function, but we don't need any reads here, just writes. */

    auto dst = static_cast<UInt32*>(Buffer);

    /* And this is like SetMemory(), but now we know that the size is 4-bytes aligned, and the value is also a 32-bits
     * one (instead of 8-bits). */

    Int64x2 val = UInt32x4 { Value, Value, Value, Value };

#ifdef NO_256_SIMD
    while (Length >= 8) {
        SIMD::StoreUnaligned(dst, val);
        SIMD::StoreUnaligned(dst + 4, val);
        Length -= 8;
        dst += 8;
    }

    while (Length >= 4) {
        SIMD::StoreUnaligned(dst, val);
        Length -= 4;
        dst += 4;
    }
#else
    Int64x4 val2 = UInt32x8 { Value, Value, Value, Value, Value, Value, Value, Value };

    while (Length >= 16) {
        SIMD::StoreUnaligned(dst, val2);
        SIMD::StoreUnaligned(dst + 8, val2);
        Length -= 16;
        dst += 16;
    }

    while (Length >= 8) {
        SIMD::StoreUnaligned(dst, val2);
        Length -= 8;
        dst += 8;
    }

    while (Length >= 4) {
        SIMD::StoreUnaligned(dst, val);
        Length -= 4;
        dst += 4;
    }
#endif

    while (Length--) {
        *dst++ = Value;
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
        auto dst = &static_cast<UInt8*>(Buffer)[Length - 1];
        auto src = &static_cast<const UInt8*>(Source)[Length - 1];

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

    for (auto m1 = static_cast<const UInt8*>(Left), m2 = static_cast<const UInt8*>(Right); Length--;) {
        if (*m1++ != *m2++) {
            return False;
        }
    }

    return True;
}

}
