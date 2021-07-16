/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 09 of 2021, at 12:54 BRT
 * Last edited on July 16 of 2021, at 09:47 BRT */

#include <util/stacktrace.hxx>

using namespace CHicago;

Boolean StackTrace::Initialized = False;
UIntPtr StackTrace::SymbolCount = 0;
BootInfoSymbol *StackTrace::Symbols = Null;

Void StackTrace::Initialize(const BootInfo &Info) {
    /* We don't really need the symbols, but it's nice to already have which function crashed right in the backtrace. */

    if (!Initialized && Info.Symbols.Count && Info.Symbols.Start != Null) {
        Initialized = True;
        Symbols = Info.Symbols.Start;
        SymbolCount = Info.Symbols.Count;
        Debug.Write("initialized the kernel symbol table, starting at 0x{:0*:16}, and there are {} symbols\n", Symbols,
                    SymbolCount);
    } else
        Debug.Write("the kernel symbols are unavailable, and name resolving on backtrace will be also unavailable\n");
}

Boolean StackTrace::GetSymbol(UIntPtr Address, StringView &Name, UIntPtr &Offset) {
    if (!Initialized) return False;

    /* Some symbols will not be included (0-length symbols, like __init_array_start), but we should have most symbols,
     * and of course the backtrace will normally only include the functions/virtual tables.
     * Here we just need to try to find a symbol with start <= address && end >= address, and then save both the name
     * and offset (the offset is of the address compared to the start of the symbol). */

    for (UIntPtr i = 0; i < SymbolCount; i++) {
        if (Symbols[i].Start > Address) break;
        else if (Symbols[i].Start <= Address && Symbols[i].End >= Address) {
            Name = Symbols[i].Name;
            Offset = Address - Symbols[i].Start;
            return True;
        }
    }

    return False;
}
