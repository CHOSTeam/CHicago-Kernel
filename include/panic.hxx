/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 09 of 2021, at 14:24 BRT
 * Last edited on February 15 of 2021 at 10:01 BRT */

#pragma once

#include <stacktrace.hxx>
#include <textout.hxx>

#define ASSERT(x) ((x) ? (Void)0 : Panic::AssertFailed(#x, __FILE__, __PRETTY_FUNCTION__, __LINE__))

class Panic {
public:
    static no_return Void AssertFailed(const String&, const String&, const String&, UInt32);
private:
    static always_inline Void DumpBacktrace(Void) {
        /* We have to take caution with the backtrace. If possible, we want to skip the first entry to the stacktrace
         * (as that would be the panic function), but if not possible, we just want to print that there is no backtrace
         * available. */

        UIntPtr addr[32];
        StackFrame *frame = GetStackFrame();

        if (frame == Null || frame->Parent == Null) {
            Debug.Write("no backtrace available\n");
            return;
        }

        UIntPtr count = StackTrace::Trace(frame->Parent, addr, sizeof(addr) / sizeof(UIntPtr));

        Debug.Write("backtrace:\n");

        for (UIntPtr off, i = 0; i < count; i++) {
            /* If possible let's print the symbol information (may not be possible if it's some left over of the
             * bootloader, or if the symbol table hasn't been initialized). */

            String name;

            if (StackTrace::GetSymbol(addr[i], name, off)) {
                Debug.Write("    " UINTPTR_MAX_HEX ": %s +" UINTPTR_HEX "\n", addr[i], name.GetValue(), off);
            } else {
                Debug.Write("    " UINTPTR_MAX_HEX ": <no symbol information available>\n", addr[i]);
            }
        }
    }
};
