/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 09 of 2021, at 14:26 BRT
 * Last edited on July 18 of 2021, at 23:14 BRT */

#include <sys/arch.hxx>
#include <sys/panic.hxx>

using namespace CHicago;

no_return Void Panic::AssertFailed(const StringView &Expression, const StringView &File, const StringView &Function,
                                   UInt32 Line) {
    Arch::EnterPanicState();
    Debug.Write("{}panic in core {}: assertion '{}' failed at {}:{}, function '{}'\n", SetForeground { 0xFFFF0000 },
                Arch::GetCoreId(), Expression, File, Line, Function);
    StackTrace::Dump();
    Arch::Halt(True);
}
