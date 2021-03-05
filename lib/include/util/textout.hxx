/* File author is Ítalo Lima Marconato Matias
 *
 * Created on June 26 of 2020, at 13:16 BRT
 * Last edited on March 05 of 2021 at 12:55 BRT */

#pragma once

#include <base/string.hxx>

/* To inherit this class you only need to implement the ->WriteInt(Char) function (AfterWrite is empty by default and
 * is not fully virtual, so you don't need to overwrite it if you don't want). */

namespace CHicago {

class TextOutput {
public:
    Void Write(Char Data) { WriteInt(Data); AfterWrite(); }

    template<typename... T> inline UIntPtr Write(const StringView &Format, T... Args) {
        /* Here we can call WriteInt one time (passing 0 as an arg) to make sure the write is even possible. Other than
         * that, it's the same processes as the String and Image formatted text output functions. */

        if (!WriteInt(0)) {
            return 0;
        }

        UIntPtr ret = VariadicFormat([](Char Data, Void *Context) -> Boolean {
            return static_cast<TextOutput*>(Context)->WriteInt(Data);
        }, static_cast<Void*>(this), Format, Args...);

        AfterWrite();

        return ret;
    }
private:
    virtual Void AfterWrite() { }
    virtual Boolean WriteInt(Char) = 0;
};

}
