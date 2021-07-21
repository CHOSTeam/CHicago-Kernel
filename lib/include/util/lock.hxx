/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 17 of 2021, at 17:37 BRT
 * Last edited on July 21 of 2021 at 15:43 BRT */

#pragma once

#include <arch/misc.hxx>
#include <base/atomic.hxx>

namespace CHicago {

class SpinLock {
public:
    inline Boolean TryAcquire(Void) {
        ARCH_SENSITIVE_START();

        if (AtomicExchange(Locked, True, __ATOMIC_ACQUIRE)) {
            ARCH_SENSITIVE_END();
            return False;
        }

        return True;
    }

    inline Void Acquire(Void) {
        while (True) {
            if (TryAcquire()) break;
            while (AtomicLoad(Locked, __ATOMIC_RELAXED)) ARCH_PAUSE();
        }
    }

    inline Void Release(Void) { AtomicStore(Locked, False, __ATOMIC_RELEASE); ARCH_SENSITIVE_END(); }
private:
    UIntPtr Context = 0;
    volatile Boolean Locked = False;
};

}
