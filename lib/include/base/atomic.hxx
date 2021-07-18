/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 18 of 2020, at 11:49 BRT
 * Last edited on July 18 of 2021 at 12:18 BRT */

#pragma once

#include <base/traits.hxx>

namespace CHicago {

template<class T> static inline T AtomicLoad(T &Pointer) {
    return __atomic_load_n(&Pointer, __ATOMIC_SEQ_CST);
}

template<class T> static inline Void AtomicStore(T &Pointer, RemoveVolatileT<T> Value) {
    __atomic_store_n(&Pointer, Value, __ATOMIC_SEQ_CST);
}

template<class T> static inline T AtomicExchange(T &Pointer, RemoveVolatileT<T> Value) {
    return __atomic_exchange_n(&Pointer, Value, __ATOMIC_SEQ_CST);
}

template<class T> static inline Boolean AtomicCompareExchange(T &Pointer, RemoveVolatileT<T> Expected,
                                                              RemoveVolatileT<T> Value) {
    return __atomic_compare_exchange_n(&Pointer, &Expected, Value, False, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

template<class T> static inline T AtomicFetchAdd(T &Pointer, RemoveVolatileT<T> Value) {
    return __atomic_fetch_add(&Pointer, Value, __ATOMIC_SEQ_CST);
}

template<class T> static inline T AtomicFetchSub(T &Pointer, RemoveVolatileT<T> Value) {
    return __atomic_fetch_sub(&Pointer, Value, __ATOMIC_SEQ_CST);
}

template<class T> static inline T AtomicFetchAnd(T &Pointer, RemoveVolatileT<T> Value) {
    return __atomic_fetch_and(&Pointer, Value, __ATOMIC_SEQ_CST);
}

template<class T> static inline T AtomicFetchOr(T &Pointer, RemoveVolatileT<T> Value) {
    return __atomic_fetch_or(&Pointer, Value, __ATOMIC_SEQ_CST);
}

template<class T> static inline T AtomicFetchXor(T &Pointer, RemoveVolatileT<T> Value) {
    return __atomic_fetch_xor(&Pointer, Value, __ATOMIC_SEQ_CST);
}

template<class T> static inline T AtomicFetchNand(T &Pointer, RemoveVolatileT<T> Value) {
    return __atomic_fetch_nand(&Pointer, Value, __ATOMIC_SEQ_CST);
}

template<class T> static inline T AtomicAddFetch(T &Pointer, RemoveVolatileT<T> Value) {
    return __atomic_add_fetch(&Pointer, Value, __ATOMIC_SEQ_CST);
}

template<class T> static inline T AtomicSubFetch(T &Pointer, RemoveVolatileT<T> Value) {
    return __atomic_sub_fetch(&Pointer, Value, __ATOMIC_SEQ_CST);
}

template<class T> static inline T AtomicAndFetch(T &Pointer, RemoveVolatileT<T> Value) {
    return __atomic_and_fetch(&Pointer, Value, __ATOMIC_SEQ_CST);
}

template<class T> static inline T AtomicOrFetch(T &Pointer, RemoveVolatileT<T> Value) {
    return __atomic_or_fetch(&Pointer, Value, __ATOMIC_SEQ_CST);
}

template<class T> static inline T AtomicXorFetch(T &Pointer, RemoveVolatileT<T> Value) {
    return __atomic_xor_fetch(&Pointer, Value, __ATOMIC_SEQ_CST);
}

template<class T> static inline T AtomicNandFetch(T &Pointer, RemoveVolatileT<T> Value) {
    return __atomic_nand_fetch(&Pointer, Value, __ATOMIC_SEQ_CST);
}

}
