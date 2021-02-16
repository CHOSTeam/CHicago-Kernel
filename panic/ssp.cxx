/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 16 of 2021, at 00:27 BRT
 * Last edited on February 16 of 2021 at 00:39 BRT */

#include <arch.hxx>
#include <panic.hxx>

extern "C" no_return Void __stack_chk_fail(Void) {
    Debug.SetForeground(0xFFFF0000);
    Debug.Write("panic: stack smashing detected\n");
    StackTrace::Dump();
    Arch::Halt();
}
