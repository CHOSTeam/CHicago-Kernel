/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 06 of 2021, at 12:46 BRT
 * Last edited on July 16 of 2021 at 16:28 BRT */

#pragma once

#include <sys/boot.hxx>

namespace CHicago {

enum class TimeUnit { Nanoseconds, Microseconds, Milliseconds, Seconds };

class Arch {
public:
    static Void Initialize(const BootInfo&);

    static no_return Void Halt(Boolean = False);

    static Void Sleep(TimeUnit, UInt64);
    static UInt64 GetUpTime(TimeUnit);
};

}
