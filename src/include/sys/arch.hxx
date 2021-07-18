/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 06 of 2021, at 12:46 BRT
 * Last edited on July 17 of 2021 at 22:19 BRT */

#pragma once

#include <sys/boot.hxx>

namespace CHicago {

enum class TimeUnit { Nanoseconds, Microseconds, Milliseconds, Seconds };

class Arch {
public:
    static Void InitializeCore(Void);
    static Void Initialize(const BootInfo&);

    static UIntPtr GetCoreId(Void);

    static Void EnterPanicState(Void);
    static no_return Void Halt(Boolean = False);

    static Void Sleep(TimeUnit, UInt64);
    static UInt64 GetUpTime(TimeUnit);
};

}
