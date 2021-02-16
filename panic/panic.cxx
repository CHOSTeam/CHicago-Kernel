/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 09 of 2021, at 14:26 BRT
 * Last edited on February 15 of 2021 at 12:13 BRT */

#include <arch.hxx>
#include <panic.hxx>

no_return Void Panic::AssertFailed(const String &Expression, const String &File, const String &Function, UInt32 Line) {
    Debug.SetForeground(0xFFFF0000);
    Debug.Write("panic: assertion '%s' failed at %s:%u, function '%s'\n", Expression.GetValue(), File.GetValue(),
                Line, Function.GetValue());
    StackTrace::Dump();
    Arch::Halt();
}
