/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 06 of 2021, at 12:47 BRT
 * Last edited on March 11 of 2021, at 18:05 BRT */

#include <arch/desctables.hxx>
#include <sys/arch.hxx>
#include <vid/console.hxx>

using namespace CHicago;

Void Arch::Initialize(BootInfo&) {
    /* Initialize the descriptor tables (GDT and IDT, as they are required for pretty much everything, including
     * gracefully handling exceptions/errors).*/

    GdtInit();
    Debug.Write("initialized the global descriptor table\n");

    IdtInit();
    Debug.Write("initialized the interrupt descriptor table\n");
}

no_return Void Arch::Halt(Boolean Full) {
    if (Full) {
        while (True) {
            asm volatile("cli; hlt");
        }
    } else {
        while (True) {
            asm volatile("hlt");
        }
    }
}
