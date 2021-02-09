/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 08 of 2021, at 15:57 BRT
 * Last edited on February 09 of 2021 at 13:29 BRT */

#pragma once

#include <boot.hxx>
#include <string.hxx>

/* On some archs (well, on all the three currently supported archs), we can just use __builtin_frame_address(0), and
 * what we get will already be a valid StackFrame pointer. */

#ifndef CUSTOM_STACK_FRAME
struct packed StackFrame {
    StackFrame *Parent;
    UIntPtr Address;
};

static unused no_inline StackFrame *GetStackFrame(Void) {
    return reinterpret_cast<StackFrame*>(__builtin_frame_address(0));
}
#else
#include <arch/stackframe.hxx>
#endif

class StackTrace {
public:
    static Void Initialize(BootInfo&);

    static Boolean GetSymbol(UIntPtr, String&, UIntPtr&);
    static UIntPtr Trace(UIntPtr *Addresses, UIntPtr Max) { return Trace(GetStackFrame(), Addresses, Max); }

    static always_inline UIntPtr Trace(StackFrame *Frame, UIntPtr *Addresses, UIntPtr Max) {
        /* While we wouldn't page fault accidentally if didn't handle the frame pointer here, we do have to make sure
         * we handle the null Addresses pointer (and the Max size has to be at least 1). */
    
        if (Frame == Null || Addresses == Null || !Max) {
            return 0;
        }
    
        UIntPtr i = 0;
    
        for (; Frame != Null && i < Max; Frame = Frame->Parent) {
            Addresses[i++] = Frame->Address;
        }
    
        return i;
    }
private:
    static Boolean Initialized;
    static UIntPtr SymbolCount;
    static BootInfoSymbol *Symbols;
};
