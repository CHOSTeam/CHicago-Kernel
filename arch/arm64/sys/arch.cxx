/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 06 of 2021, at 12:48 BRT
 * Last edited on February 06 of 2021 at 12:48 BRT */

#include <arch.hxx>

no_return Void Arch::Halt(Void) {
    while (True) {
        asm volatile("wfi");
    }
}
