/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 17 of 2021, at 17:37 BRT
 * Last edited on July 19 of 2021 at 09:29 BRT */

#pragma once

#include <arch/misc.hxx>
#include <base/atomic.hxx>

namespace CHicago {

class SpinLock {
public:
    inline Void Acquire(Void) {
        while (True) {
            if (TryAcquire()) break;
            while (AtomicLoad(Locked, __ATOMIC_RELAXED)) ARCH_PAUSE();
        }
    }

    inline Void Release(Void) { AtomicStore(Locked, False, __ATOMIC_RELEASE); }
    inline Boolean TryAcquire(Void) { return !AtomicExchange(Locked, True, __ATOMIC_ACQUIRE); }
private: volatile Boolean Locked = False;
};

}
