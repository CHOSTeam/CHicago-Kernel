/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 18 of 2021, at 13:17 BRT
 * Last edited on July 20 of 2021 at 15:20 BRT */

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
    __builtin_ia32_movntdq(static_cast<Int64x2*>(Buffer), Value);
}

disable_ubsan static inline always_inline Void StoreNonTemporal(Void *Buffer, Floatx2 Value) {
    __builtin_ia32_movntpd(static_cast<Float*>(Buffer), Value);
}

#ifndef NO_256_SIMD
disable_ubsan static inline always_inline Void StoreNonTemporal(Void *Buffer, Int64x4 Value) {
    __builtin_ia32_movntdq256(static_cast<Int64x4*>(Buffer), Value);
}

disable_ubsan static inline always_inline Void StoreNonTemporal(Void *Buffer, Floatx4 Value) {
    __builtin_ia32_movntpd256(static_cast<Float*>(Buffer), Value);
}
#endif
