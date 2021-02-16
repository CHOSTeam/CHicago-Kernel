/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 06 of 2021, at 12:47 BRT
 * Last edited on February 16 of 2021 at 10:27 BRT */

#include <arch.hxx>

using namespace CHicago;

no_return Void Arch::Halt(Void) {
    while (True) {
        asm volatile("hlt");
    }
}
