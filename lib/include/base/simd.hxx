/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 18 of 2021, at 10:33 BRT
 * Last edited on March 04 of 2021 at 17:06 BRT */

#pragma once

#include <base/types.hxx>

namespace CHicago {

/* 128-bit vector types (x86/amd64 uses them for SSE ops). */

typedef Char Charx16 vector_size(16), Charx16U vector_size(16) aligned(1);

typedef Int8 Int8x16 vector_size(16), Int8x16U vector_size(16) aligned(1);
typedef Int16 Int16x8 vector_size(16), Int16x8U vector_size(16) aligned(1);
typedef Int32 Int32x4 vector_size(16), Int32x4U vector_size(16) aligned(1);
typedef Int64 Int64x2 vector_size(16), Int64x2U vector_size(16) aligned(1);

typedef UInt8 UInt8x16 vector_size(16), UInt8x16U vector_size(16) aligned(1);
typedef UInt16 UInt16x8 vector_size(16), UInt16x8U vector_size(16) aligned(1);
typedef UInt32 UInt32x4 vector_size(16), UInt32x4U vector_size(16) aligned(1);
typedef UInt64 UInt64x2 vector_size(16), UInt64x2U vector_size(16) aligned(1);

typedef Float Floatx2 vector_size(16), Floatx2U vector_size(16) aligned(1);

/* 256-bit vector types (x86/amd64 uses them for AVX ops). */

#ifndef NO_256_SIMD
typedef Char Charx32 vector_size(32), Charx32U vector_size(32) aligned(1);

typedef Int8 Int8x32 vector_size(32), Int8x32U vector_size(32) aligned(1);
typedef Int16 Int16x16 vector_size(32), Int16x16U vector_size(32) aligned(1);
typedef Int32 Int32x8 vector_size(32), Int32x8U vector_size(32) aligned(1);
typedef Int64 Int64x4 vector_size(32), Int64x4U vector_size(32) aligned(1);

typedef UInt8 UInt8x32 vector_size(32), UInt8x32U vector_size(32) aligned(1);
typedef UInt16 UInt16x16 vector_size(32), UInt16x16U vector_size(32) aligned(1);
typedef UInt32 UInt32x8 vector_size(32), UInt32x8U vector_size(32) aligned(1);
typedef UInt64 UInt64x4 vector_size(32), UInt64x4U vector_size(32) aligned(1);

typedef Float Floatx4 vector_size(32), Floatx4U vector_size(32) aligned(1);
#endif

class SIMD {
public:
    template<typename T, typename S> disable_ubsan  static inline always_inline
    T Convert(S Vector) { return __builtin_convertvector(Vector, T); }

    template<typename T, typename M> disable_ubsan static inline always_inline
    T Shuffle(T Vector, M Mask) { return __builtin_shuffle(Vector, Mask); }

    template<typename T, typename M> disable_ubsan static inline always_inline
    T Shuffle(T Vector1, T Vector2, M Mask) { return __builtin_shuffle(Vector1, Vector2, Mask); }

    disable_ubsan static inline always_inline
    Int64x2 LoadAlignedI16(const Void *Source) { return *reinterpret_cast<const Int64x2*>(Source); }

    disable_ubsan static inline always_inline
    Floatx2 LoadAlignedF16(const Void *Source) { return *reinterpret_cast<const Floatx2*>(Source); }

    disable_ubsan static inline always_inline
    Int64x2 LoadUnalignedI16(const Void *Source) { return *reinterpret_cast<const Int64x2U*>(Source); }

    disable_ubsan static inline always_inline
    Floatx2 LoadUnalignedF16(const Void *Source) { return *reinterpret_cast<const Floatx2U*>(Source); }
#ifndef NO_256_SIMD
    disable_ubsan static inline always_inline
    Int64x4 LoadAlignedI32(const Void *Source) { return *reinterpret_cast<const Int64x4*>(Source); }

    disable_ubsan static inline always_inline
    Floatx4 LoadAlignedF32(const Void *Source) { return *reinterpret_cast<const Floatx4*>(Source); }

    disable_ubsan static inline always_inline
    Int64x4 LoadUnalignedI32(const Void *Source) { return *reinterpret_cast<const Int64x4U*>(Source); }

    disable_ubsan static inline always_inline
    Floatx4 LoadUnalignedF32(const Void *Source) { return *reinterpret_cast<const Floatx4U*>(Source); }
#endif

    disable_ubsan static inline always_inline
    Void StoreAligned(Void *Buffer, Int64x2 Value) { *reinterpret_cast<Int64x2*>(Buffer) = Value; }

    disable_ubsan static inline always_inline
    Void StoreAligned(Void *Buffer, Floatx2 Value) { *reinterpret_cast<Floatx2*>(Buffer) = Value; }

    disable_ubsan static inline always_inline
    Void StoreUnaligned(Void *Buffer, Int64x2 Value) { *reinterpret_cast<Int64x2U*>(Buffer) = Value; }

    disable_ubsan static inline always_inline
    Void StoreUnaligned(Void *Buffer, Floatx2 Value) { *reinterpret_cast<Floatx2U*>(Buffer) = Value; }
#ifndef NO_256_SIMD
    disable_ubsan static inline always_inline
    Void StoreAligned(Void *Buffer, Int64x4 Value) { *reinterpret_cast<Int64x4*>(Buffer) = Value; }

    disable_ubsan static inline always_inline
    Void StoreAligned(Void *Buffer, Floatx4 Value) { *reinterpret_cast<Floatx4*>(Buffer) = Value; }

    disable_ubsan static inline always_inline
    Void StoreUnaligned(Void *Buffer, Int64x4 Value) { *reinterpret_cast<Int64x4U*>(Buffer) = Value; }

    disable_ubsan static inline always_inline
    Void StoreUnaligned(Void *Buffer, Floatx4 Value) { *reinterpret_cast<Floatx4U*>(Buffer) = Value; }
#endif

    /* Now everything else is arch-dependent (and as such let's include a special arch-specific header). */

#include <arch/simd.hxx>
};

}
