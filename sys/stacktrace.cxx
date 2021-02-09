/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 09 of 2021, at 12:54 BRT
 * Last edited on February 09 of 2021 at 14:56 BRT */

#include <stacktrace.hxx>

Boolean StackTrace::Initialized = False;
UIntPtr StackTrace::SymbolCount = 0;
BootInfoSymbol *StackTrace::Symbols = Null;

#include <textout.hxx>

Void StackTrace::Initialize(BootInfo &Info) {
    /* We don't really need the symbols, but it's nice to already have which function crashed right in the backtrace. */

    if (!Initialized && Info.Symbols.Count && Info.Symbols.Start != Null) {
        Initialized = True;
        Symbols = Info.Symbols.Start;
        SymbolCount = Info.Symbols.Count;
    }
}

#include <textout.hxx>

Boolean StackTrace::GetSymbol(UIntPtr Address, String &Name, UIntPtr &Offset) {
    if (!Initialized) {
        return False;
    }

    /* Some symbols will not be included (0-length symbols, like __init_array_start), but we should have most symbols,
     * and of course the backtrace will normally only include the functions/virtual tables.
     * Here we just need to try to find a symbol with start <= address && end >= address, and then save both the name
     * and offset (the offset is of the address comparated to the start of the symbol). */

    for (UIntPtr i = 0; i < SymbolCount; i++) {
        if (Symbols[i].Start > Address) {
            break;
        } else if (Symbols[i].Start <= Address && Symbols[i].End >= Address) {
            Name = Symbols[i].Name;
            Offset = Address - Symbols[i].Start;
            return True;
        }
    }

    return False;
}