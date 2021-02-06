/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 06 of 2021, at 12:47 BRT
 * Last edited on February 06 of 2021 at 12:47 BRT */

#include <arch.hxx>

no_return Void Arch::Halt(Void) {
    while (True) {
        asm volatile("hlt");
    }
}
