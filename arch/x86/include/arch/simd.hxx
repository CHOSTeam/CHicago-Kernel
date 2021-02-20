/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 18 of 2021, at 13:17 BRT
 * Last edited on February 19 of 2021 at 20:43 BRT */

#pragma once

/* No need to enter the CHicago namespace, nor anything like that, only simd.hxx (the global one) should be including
 * us, and we should already be inside the SIMD class. */

disable_ubsan static inline always_inline Floatx2 Round(Floatx2 Vector) { return __builtin_ia32_roundpd(Vector, 0); }
#ifndef NO_256_SIMD
disable_ubsan static inline always_inline Floatx4 Round(Floatx4 Vector) { return __builtin_ia32_roundpd256(Vector, 0); }
#endif

disable_ubsan static inline always_inline Floatx2 SquareRoot(Floatx2 Vector) { return __builtin_ia32_sqrtpd(Vector); }

#ifndef NO_256_SIMD
disable_ubsan static inline always_inline
Floatx4 SquareRoot(Floatx4 Vector) { return __builtin_ia32_sqrtpd256(Vector); }
#endif

disable_ubsan static inline always_inline Void StoreNonTemporal(Void *Buffer, Int64x2 Value) {
    __builtin_ia32_movntdq(reinterpret_cast<Int64x2*>(Buffer), Value);
}

disable_ubsan static inline always_inline Void StoreNonTemporal(Void *Buffer, Floatx2 Value) {
    __builtin_ia32_movntpd(reinterpret_cast<Float*>(Buffer), Value);
}

#ifndef NO_256_SIMD
disable_ubsan static inline always_inline Void StoreNonTemporal(Void *Buffer, Int64x4 Value) {
    __builtin_ia32_movntdq256(reinterpret_cast<Int64x4*>(Buffer), Value);
}

disable_ubsan static inline always_inline Void StoreNonTemporal(Void *Buffer, Floatx4 Value) {
    __builtin_ia32_movntpd256(reinterpret_cast<Float*>(Buffer), Value);
}
#endif
