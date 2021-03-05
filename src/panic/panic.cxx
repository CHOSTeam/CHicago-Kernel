/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 09 of 2021, at 14:26 BRT
 * Last edited on March 05 of 2021, at 13:46 BRT */

#include <sys/arch.hxx>
#include <sys/panic.hxx>

using namespace CHicago;

no_return Void Panic::AssertFailed(const StringView &Expression, const StringView &File, const StringView &Function,
                                   UInt32 Line) {
    Debug.SetForeground(0xFFFF0000);
    Debug.Write("panic: assertion '{}' failed at {}:{}, function '{}'\n", Expression, File, Line, Function);
    StackTrace::Dump();
    Arch::Halt(True);
}
