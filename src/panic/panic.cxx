/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 09 of 2021, at 14:26 BRT
 * Last edited on February 22 of 2021 at 18:36 BRT */

#include <arch.hxx>
#include <panic.hxx>

using namespace CHicago;

no_return Void Panic::AssertFailed(const String &Expression, const String &File, const String &Function, UInt32 Line) {
    Debug.SetForeground(0xFFFF0000);
    Debug.Write("panic: assertion '{}' failed at {}:{}, function '{}'\n", Expression, File, Line, Function);
    StackTrace::Dump();
    Arch::Halt(True);
}
