/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 16 of 2021, at 13:22 BRT
 * Last edited on July 17 of 2021, at 22:18 BRT */

#include <sys/arch.hxx>
#include <sys/panic.hxx>

using namespace CHicago;

extern "C" no_return Void __stack_chk_fail() {
    /* CHicago is always (or at the least should be always) built with SSP enabled, so we need this function to halt
     * everything if someone tried smashing/overflowing the stack. */

    Arch::EnterPanicState();
    Debug.Write("{}panic: stack smashing detected\n", SetForeground { 0xFFFF0000 });
    StackTrace::Dump();
    Arch::Halt(True);
}

extern "C" no_return Void __cxa_pure_virtual() {
    /* GCC emits __cxa_pure_virtual calls when you try to call pure virtual functions (they aren't supposed
     * to be called). */

    Arch::EnterPanicState();
    Debug.Write("{}panic: pure virtual function called\n", SetForeground { 0xFFFF0000 });
    StackTrace::Dump();
    Arch::Halt(True);
}
