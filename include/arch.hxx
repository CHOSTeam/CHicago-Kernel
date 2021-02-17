/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 06 of 2021, at 12:46 BRT
 * Last edited on February 17 of 2021 at 11:36 BRT */

#pragma once

#include <boot.hxx>

namespace CHicago {

class Arch {
public:
    static Void Initialize(BootInfo&);
    static no_return Void Halt(Void);
};

}
