/* File author is Ítalo Lima Marconato Matias
 *
 * Created on June 26 of 2020, at 13:16 BRT
 * Last edited on July 17 of 2021 at 21:46 BRT */

#pragma once

#include <base/string.hxx>
#include <util/lock.hxx>

/* To inherit this class you only need to implement the ->WriteInt(Char) function (AfterWrite is empty by default and
 * is not fully virtual, so you don't need to overwrite it if you don't want). */

namespace CHicago {

class TextOutput {
public:
    Void SetBackground(UInt32 Value) { Lock.Acquire(); SetBackgroundInt(Value); AfterWrite(); Lock.Release(); }
    Void SetForeground(UInt32 Value) { Lock.Acquire(); SetForegroundInt(Value); AfterWrite(); Lock.Release(); }
    Void RestoreBackground(Void) { Lock.Acquire(); RestoreBackgroundInt(); AfterWrite(); Lock.Release(); }
    Void RestoreForeground(Void) { Lock.Acquire(); RestoreForegroundInt(); AfterWrite(); Lock.Release(); }
    Void Write(Char Data) { Lock.Acquire(); WriteInt(Data); AfterWrite(); Lock.Release(); }

    template<typename... T> inline UIntPtr Write(const StringView &Format, T... Args) {
        /* Here we can call WriteInt one time (passing 0 as an arg) to make sure the write is even possible. Other than
         * that, it's the same processes as the String and Image formatted text output functions. */

        if (!WriteInt(0)) return 0;

        Lock.Acquire();

        UIntPtr ret = VariadicFormat([](UInt8 Type, UInt32 Data, Void *Context) -> Boolean {
            TextOutput *out = static_cast<TextOutput*>(Context);
            return !Type ? out->WriteInt(Data) : ((Type == 1 ? out->SetBackgroundInt(Data) :
                                                  (Type == 2 ? out->SetForegroundInt(Data) :
                                                  (Type == 3 ? out->RestoreBackgroundInt() :
                                                               out->RestoreForegroundInt()))), True);
        }, static_cast<Void*>(this), Format, Args...);

        AfterWrite();
        Lock.Release();

        return ret;
    }
private:
    virtual Void AfterWrite() { }
    virtual Boolean WriteInt(Char) = 0;
    virtual Void SetBackgroundInt(UInt32) { };
    virtual Void SetForegroundInt(UInt32) { };
    virtual Void RestoreBackgroundInt(Void) { };
    virtual Void RestoreForegroundInt(Void) { };
protected:
    SpinLock Lock {};
};

}
