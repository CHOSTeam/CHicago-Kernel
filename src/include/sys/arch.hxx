/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 06 of 2021, at 12:46 BRT
 * Last edited on March 11 of 2021 at 16:11 BRT */

#pragma once

#include <sys/boot.hxx>

namespace CHicago {

class Arch {
public:
    static Void Initialize(BootInfo&);

    static no_return Void Halt(Boolean = False);

    static Void Sleep(UInt64);
    static UInt64 GetUpTime();
};

}
