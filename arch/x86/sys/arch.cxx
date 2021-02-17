/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 06 of 2021, at 12:47 BRT
 * Last edited on February 17 of 2021 at 11:39 BRT */

#include <arch/desctables.hxx>
#include <arch.hxx>
#include <textout.hxx>

using namespace CHicago;

Void Arch::Initialize(BootInfo&) {
    /* For now, we only have to initialize the descriptor tables (GDT and IDT, as they are required for pretty much
     * everything, including gracefully handling exceptions/errors). */

    GdtInit();
    Debug.Write("initialized the global descriptor table\n");

    IdtInit();
    Debug.Write("initialized the interrupt descriptor table\n");
}

no_return Void Arch::Halt(Void) {
    while (True) {
        asm volatile("hlt");
    }
}
