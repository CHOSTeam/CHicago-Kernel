/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 07 of 2021, at 23:22 BRT
 * Last edited on February 22 of 2021, at 14:16 BRT */

#include <textout.hxx>

using namespace CHicago;

Void TextOutput::Write(Char Data) {
    WriteInt(Data);
    AfterWrite();
}

UIntPtr TextOutput::Write(const String &Format, ...) {
    /* Differently from the Image class, here we only have one thing to pass as the context, and that is ourselves
     * (and we can pass that as a single pointer, instead of an array of addresses). */

    if (!WriteInt(0)) {
        return 0;
    }

    VariadicList args;
    VariadicStart(args, Format);

    UIntPtr ret = VariadicFormat(Format, args, [](Char Data, Void *Context) -> Boolean {
        return static_cast<TextOutput*>(Context)->WriteInt(Data);
    }, static_cast<Void*>(this));

    VariadicEnd(args);
    AfterWrite();

    return ret;
}
