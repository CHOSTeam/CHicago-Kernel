/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 17 of 2021, at 17:37 BRT
 * Last edited on July 17 of 2021 at 22:37 BRT */

#pragma once

#include <base/types.hxx>

namespace CHicago {

class SpinLock {
public:
    inline Boolean TryAcquire(Void) {
        Boolean clear = False;
        return __atomic_compare_exchange_n(&Locked, &clear, True, False, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    }

    inline Void Acquire(Void) { while (!TryAcquire()) ; }
    inline Void Release(Void) { __atomic_clear(&Locked, __ATOMIC_SEQ_CST); }
private: volatile Boolean Locked = False;
};

}
