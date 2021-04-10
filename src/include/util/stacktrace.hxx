/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 08 of 2021, at 15:57 BRT
 * Last edited on April 10 of 2021, at 17:08 BRT */

#pragma once

#include <vid/console.hxx>

/* On some archs (well, on all the three currently supported archs), we can just use __builtin_frame_address(0), and
 * what we get will already be a valid StackFrame pointer. */

namespace CHicago {

#ifndef CUSTOM_STACK_FRAME
struct packed StackFrame {
    StackFrame *Parent;
    UIntPtr Address;
};

static unused no_inline StackFrame *GetStackFrame() {
    return reinterpret_cast<StackFrame*>(__builtin_frame_address(0));
}
#else
#include <arch/stackframe.hxx>
#endif

class StackTrace {
public:
    static Void Initialize(BootInfo&);

    static Boolean GetSymbol(UIntPtr, StringView&, UIntPtr&);
    static UIntPtr Trace(UIntPtr *Addresses, UIntPtr Max) { return Trace(GetStackFrame(), Addresses, Max); }

    static inline always_inline UIntPtr Trace(StackFrame *Frame, UIntPtr *Addresses, UIntPtr Max) {
        /* While we wouldn't page fault accidentally if didn't handle the frame pointer here, we do have to make sure
         * we handle the null Addresses pointer (and the Max size has to be at least 1). */

        if (Frame == Null || Addresses == Null || !Max) return 0;

        UIntPtr i = 0;
        for (; Frame != Null && i < Max; Frame = Frame->Parent) Addresses[i++] = Frame->Address;

        return i;
    }

    static inline always_inline Void Dump(StackFrame *Frame = GetStackFrame()) {
        /* We have to take caution with the backtrace. If possible, we want to skip the first entry to the stacktrace
         * (as that would be the panic function), but if not possible, we just want to print that there is no backtrace
         * available. */

        UIntPtr addr[32];

        if (Frame == Null || Frame->Parent == Null) {
            Debug.Write("no backtrace available\n");
            return;
        }

        UIntPtr count = Trace(Frame->Parent, addr, sizeof(addr) / sizeof(UIntPtr));

        Debug.Write("backtrace:\n");

        for (UIntPtr off, i = 0; i < count; i++) {
            /* If possible let's print the symbol information (may not be possible if it's some left over of the
             * bootloader, or if the symbol table hasn't been initialized). */

            StringView name;

            if (GetSymbol(addr[i], name, off)) Debug.Write("    0x{:0*:16}: {} +0x{:0:16}\n", addr[i], name, off);
            else Debug.Write("    0x{:0*:16}: <no symbol information available>\n", addr[i]);
        }
    }
private:
    static Boolean Initialized;
    static UIntPtr SymbolCount;
    static BootInfoSymbol *Symbols;
};

}
