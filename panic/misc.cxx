/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 16 of 2021, at 13:22 BRT
 * Last edited on February 28 of 2021 at 13:42 BRT */

#include <arch.hxx>
#include <panic.hxx>

using namespace CHicago;

extern "C" no_return Void __stack_chk_fail() {
    /* CHicago is always (or at the least should be always) built with SSP enabled, so we need this function to halt
     * everything if someone tried smashing/overflowing the stack. */

    Debug.SetForeground(0xFFFF0000);
    Debug.Write("panic: stack smashing detected\n");
    StackTrace::Dump();
    Arch::Halt(True);
}

extern "C" no_return Void __cxa_pure_virtual() {
    /* GCC emits __cxa_pure_virtual calls when you try to call pure virtual functions (they aren't supposed
     * to be called). */

    Debug.SetForeground(0xFFFF0000);
    Debug.Write("panic: stack smashing detected\n");
    StackTrace::Dump();
    Arch::Halt(True);
}
