/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 07 of 2021, at 17:45 BRT
 * Last edited on February 19 of 2021 at 20:40 BRT */

#include <simd.hxx>

namespace CHicago {

/* Disable UBSan for the memory functions (so that the compiler will not generate a check every single store). */

disable_ubsan Void CopyMemory(Void *Buffer, const Void *Source, UIntPtr Length) {
    if (Buffer == Null || Source == Null || Buffer == Source || !Length) {
        return;
    }

    auto dst = static_cast<UInt8*>(Buffer);
    auto src = static_cast<const UInt8*>(Source);

#ifdef NO_256_SIMD
    /* On some cases (like x86 without AVX2), we can't use 256-bit SIMD operations (or we can/GCC may let us do it, but
     * it will be slower), instead, let's use 128-bit SIMD operations. But before actually starting the copy, if the
     * length is big enough, let's align the dest pointer to a 16-byte boundary (so that we can use aligned stores). */

    if (Length >= 16) {
        UIntPtr align = 0x10 - (reinterpret_cast<UIntPtr>(Buffer) & 0x0F);
        SIMD::StoreUnaligned(dst, SIMD::LoadUnalignedI16(src));
        Length -= align;
        dst += align;
        src += align;
    }

    while (Length >= 32) {
        Int64x2 a = SIMD::LoadUnalignedI16(src), b = SIMD::LoadUnalignedI16(src + 16);
        SIMD::StoreAligned(dst, a), SIMD::StoreAligned(dst + 16, b);
        Length -= 32;
        dst += 32;
        src += 32;
    }
#else
    /* And yeah, here we just copy twice as much per loop (and also the alignment this time is 32-bytes instead of
     * 16). */

    if (Length >= 32) {
        UIntPtr align = 0x20 - (reinterpret_cast<UIntPtr>(Buffer) & 0x1F);
        SIMD::StoreUnaligned(dst, SIMD::LoadUnalignedI32(src));
        Length -= align;
        dst += align;
        src += align;
    } else if (Length >= 16) {
        UIntPtr align = 0x10 - (reinterpret_cast<UIntPtr>(Buffer) & 0x0F);
        SIMD::StoreUnaligned(dst, SIMD::LoadUnalignedI16(src));
        Length -= align;
        dst += align;
        src += align;
    }

    while (Length >= 64) {
        Int64x4 a = SIMD::LoadUnalignedI32(src), b = SIMD::LoadUnalignedI32(src + 32);
        SIMD::StoreAligned(dst, a), SIMD::StoreAligned(dst + 32, b);
        Length -= 64;
        dst += 64;
        src += 64;
    }

    while (Length >= 32) {
        SIMD::StoreAligned(dst, SIMD::LoadUnalignedI32(src));
        Length -= 32;
        dst += 32;
        src += 32;
    }
#endif

    while (Length >= 16) {
        SIMD::StoreAligned(dst, SIMD::LoadUnalignedI16(src));
        Length -= 16;
        dst += 16;
        src += 16;
    }

    while (Length--) {
        *dst++ = *src++;
    }
}

disable_ubsan Void SetMemory(Void *Buffer, UInt8 Value, UIntPtr Length) {
    if (Buffer == Null || !Length) {
        return;
    }

    /* This is like the CopyMemory function, but we don't need any reads here, just writes. */

    auto dst = static_cast<UInt8*>(Buffer);
    Int64x2 val = UInt8x16 { Value, Value, Value, Value, Value, Value, Value, Value,
                             Value, Value, Value, Value, Value, Value, Value, Value };

#ifdef NO_256_SIMD
    /* Also, we can pre-init one variable containing Int64x2 (or x4 on 256-bits) of our value. */

    if (Length >= 16) {
        UIntPtr align = 0x10 - (reinterpret_cast<UIntPtr>(Buffer) & 0x0F);
        SIMD::StoreUnaligned(dst, val);
        Length -= align;
        dst += align;
    }

    while (Length >= 32) {
        SIMD::StoreAligned(dst, val), SIMD::StoreAligned(dst + 16, val);
        Length -= 32;
        dst += 32;
    }
#else
    Int64x4 val2 = UInt8x32 { Value, Value, Value, Value, Value, Value, Value, Value,
                              Value, Value, Value, Value, Value, Value, Value, Value,
                              Value, Value, Value, Value, Value, Value, Value, Value,
                              Value, Value, Value, Value, Value, Value, Value, Value };

    if (Length >= 32) {
        UIntPtr align = 0x20 - (reinterpret_cast<UIntPtr>(Buffer) & 0x1F);
        SIMD::StoreUnaligned(dst, val2);
        Length -= align;
        dst += align;
    } else if (Length >= 16) {
        UIntPtr align = 0x10 - (reinterpret_cast<UIntPtr>(Buffer) & 0x0F);
        SIMD::StoreUnaligned(dst, val);
        Length -= align;
        dst += align;
    }

    while (Length >= 64) {
        SIMD::StoreAligned(dst, val2), SIMD::StoreAligned(dst + 32, val2);
        Length -= 64;
        dst += 64;
    }

    while (Length >= 32) {
        SIMD::StoreAligned(dst, val2);
        Length -= 32;
        dst += 32;
    }
#endif

    while (Length >= 16) {
        SIMD::StoreAligned(dst, val);
        Length -= 16;
        dst += 16;
    }

    while (Length--) {
        *dst++ = Value;
    }
}

disable_ubsan Void SetMemory32(Void *Buffer, UInt32 Value, UIntPtr Length) {
    /* This is like the CopyMemory function, but we don't need any reads here, just writes. */

    auto dst = static_cast<UInt32*>(Buffer);

    /* And this is like SetMemory(), but now we know that the size is 4-bytes aligned, and the value is also a 32-bits
     * one (instead of 8-bits). We're also not going to try aligning the dest pointer here. */

    Int64x2 val = UInt32x4 { Value, Value, Value, Value };

#ifdef NO_256_SIMD
    while (Length >= 8) {
        SIMD::StoreUnaligned(dst, val), SIMD::StoreUnaligned(dst + 4, val);
        Length -= 8;
        dst += 8;
    }
#else
    Int64x4 val2 = UInt32x8 { Value, Value, Value, Value, Value, Value, Value, Value };

    while (Length >= 16) {
        SIMD::StoreUnaligned(dst, val2), SIMD::StoreUnaligned(dst + 8, val2);
        Length -= 16;
        dst += 16;
    }

    while (Length >= 8) {
        SIMD::StoreUnaligned(dst, val2);
        Length -= 8;
        dst += 8;
    }
#endif

    while (Length >= 4) {
        SIMD::StoreUnaligned(dst, val);
        Length -= 4;
        dst += 4;
    }

    while (Length--) {
        *dst++ = Value;
    }
}

disable_ubsan Void MoveMemory(Void *Buffer, const Void *Source, UIntPtr Length) {
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

disable_ubsan Boolean CompareMemory(const Void *const Left, const Void *const Right, UIntPtr Length) {
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
