/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 07 of 2021, at 23:22 BRT
 * Last edited on February 08 of 2021, at 00:29 BRT */

#include <textout.hxx>

Void TextOutput::Write(Char Data) {
    WriteInt(Data);
    AfterWrite();
}

Void TextOutput::Write(const String &Format, ...) {
    /* Differently from the Image class, here we only have one thing to pass as the context, and that is ourselves
     * (and we can pass that as a single pointer, instead of an array of addresses). */

    if (!WriteInt(0)) {
        return;
    }

    VariadicList args;
    VariadicStart(args, Format);

    VariadicFormat(Format, args, [](Char Data, Void *Context) -> Boolean {
        return reinterpret_cast<TextOutput*>(Context)->WriteInt(Data);
    }, reinterpret_cast<Void*>(this), Null, 0, 0);

    VariadicEnd(args);
    AfterWrite();
}