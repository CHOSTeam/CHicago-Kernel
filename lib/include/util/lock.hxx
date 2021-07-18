/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 17 of 2021, at 17:37 BRT
 * Last edited on July 18 of 2021 at 12:11 BRT */

#pragma once

#include <base/atomic.hxx>

namespace CHicago {

class SpinLock {
public:
    inline Void Acquire(Void) { while (!TryAcquire()) ; }
    inline Void Release(Void) { AtomicStore(Locked, False); }
    inline Boolean TryAcquire(Void) { return AtomicCompareExchange(Locked, False, True); }
private: volatile Boolean Locked = False;
};

}
