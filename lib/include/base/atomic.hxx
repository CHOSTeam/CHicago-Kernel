/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 18 of 2020, at 11:49 BRT
 * Last edited on July 19 of 2021 at 10:03 BRT */

#pragma once

#include <base/traits.hxx>

namespace CHicago {

template<class T> static inline T AtomicLoad(T &Pointer, Int32 MemOrder = __ATOMIC_SEQ_CST) {
    return __atomic_load_n(&Pointer, MemOrder);
}

template<class T> static inline Void AtomicStore(T &Pointer, RemoveVolatileT<T> Value,
                                                 Int32 MemOrder = __ATOMIC_SEQ_CST) {
    __atomic_store_n(&Pointer, Value, MemOrder);
}

template<class T> static inline T AtomicExchange(T &Pointer, RemoveVolatileT<T> Value,
                                                 Int32 MemOrder = __ATOMIC_SEQ_CST) {
    return __atomic_exchange_n(&Pointer, Value, MemOrder);
}

template<class T> static inline Boolean AtomicCompareExchange(T &Pointer, RemoveVolatileT<T> Expected,
                                                             RemoveVolatileT<T> Value,
                                                             Int32 SuccessMemOrder = __ATOMIC_SEQ_CST,
                                                             Int32 FailMemOrder = __ATOMIC_SEQ_CST) {
    return __atomic_compare_exchange_n(&Pointer, &Expected, Value, False, SuccessMemOrder, FailMemOrder);
}

template<class T> static inline T AtomicFetchAdd(T &Pointer, RemoveVolatileT<T> Value,
                                                 Int32 MemOrder = __ATOMIC_SEQ_CST) {
    return __atomic_fetch_add(&Pointer, Value, MemOrder);
}

template<class T> static inline T AtomicFetchSub(T &Pointer, RemoveVolatileT<T> Value,
                                                 Int32 MemOrder = __ATOMIC_SEQ_CST) {
    return __atomic_fetch_sub(&Pointer, Value, MemOrder);
}

template<class T> static inline T AtomicFetchAnd(T &Pointer, RemoveVolatileT<T> Value,
                                                 Int32 MemOrder = __ATOMIC_SEQ_CST) {
    return __atomic_fetch_and(&Pointer, Value, MemOrder);
}

template<class T> static inline T AtomicFetchOr(T &Pointer, RemoveVolatileT<T> Value,
                                                Int32 MemOrder = __ATOMIC_SEQ_CST) {
    return __atomic_fetch_or(&Pointer, Value, MemOrder);
}

template<class T> static inline T AtomicFetchXor(T &Pointer, RemoveVolatileT<T> Value,
                                                 Int32 MemOrder = __ATOMIC_SEQ_CST) {
    return __atomic_fetch_xor(&Pointer, Value, MemOrder);
}

template<class T> static inline T AtomicFetchNand(T &Pointer, RemoveVolatileT<T> Value,
                                                  Int32 MemOrder = __ATOMIC_SEQ_CST) {
    return __atomic_fetch_nand(&Pointer, Value, MemOrder);
}

template<class T> static inline T AtomicAddFetch(T &Pointer, RemoveVolatileT<T> Value,
                                                 Int32 MemOrder = __ATOMIC_SEQ_CST) {
    return __atomic_add_fetch(&Pointer, Value, MemOrder);
}

template<class T> static inline T AtomicSubFetch(T &Pointer, RemoveVolatileT<T> Value,
                                                 Int32 MemOrder = __ATOMIC_SEQ_CST) {
    return __atomic_sub_fetch(&Pointer, Value, MemOrder);
}

template<class T> static inline T AtomicAndFetch(T &Pointer, RemoveVolatileT<T> Value,
                                                 Int32 MemOrder = __ATOMIC_SEQ_CST) {
    return __atomic_and_fetch(&Pointer, Value, MemOrder);
}

template<class T> static inline T AtomicOrFetch(T &Pointer, RemoveVolatileT<T> Value,
                                                Int32 MemOrder = __ATOMIC_SEQ_CST) {
    return __atomic_or_fetch(&Pointer, Value, MemOrder);
}

template<class T> static inline T AtomicXorFetch(T &Pointer, RemoveVolatileT<T> Value,
                                                 Int32 MemOrder = __ATOMIC_SEQ_CST) {
    return __atomic_xor_fetch(&Pointer, Value, MemOrder);
}

template<class T> static inline T AtomicNandFetch(T &Pointer, RemoveVolatileT<T> Value,
                                                  Int32 MemOrder = __ATOMIC_SEQ_CST) {
    return __atomic_nand_fetch(&Pointer, Value, MemOrder);
}

}
