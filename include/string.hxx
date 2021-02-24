/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 07 of 2021, at 14:01 BRT
 * Last edited on February 24 of 2021 at 10:37 BRT */

#pragma once

#include <status.hxx>
#include <vararg.hxx>

namespace CHicago {

class String {
public:
    String(Void);
    String(UIntPtr);
    String(const String&);
    String(const Char*, Boolean = False);

    ~String(Void);

    String &operator =(const Char*);
    String &operator =(const String&);

    static String FromStatus(Status);
    static String FromInt(Char*, Int64, UIntPtr);
    static String FromUInt(Char*, UInt64, UIntPtr, UInt8);
    static String FromFloat(Char*, Float, UIntPtr, UIntPtr = 6);

    /* You maybe wondering: Hey, why doesn't the constructors allow initializing the string with formatted input? Well,
     * for this we have the Format function! And yeah, we actually need to make their body here as well, because of all
     * the template<> stuff. */

    template<typename... T> static inline String Format(const Char *Format, T... Args) {
        /* First, let's create the string itself, we're going to use the 0-args initializer, as the Append function is
         * going to dynamically allocate the memory we need. */

        String str;

        /* Now we just need to call the Append function. */

        if (Format == Null) {
            return str;
        }

        return str.Append(Format, Args...), str;
    }

    Void Clear(Void);

    Void SetView(UIntPtr, UIntPtr);

    Status Append(Char);
    UIntPtr Append(Int64);
    UIntPtr Append(UInt64, UInt8);
    UIntPtr Append(Float, UIntPtr = 6);

    /* The Append(String, ...) also needs to be inline, for the same reason as Format (because it is template<>). */

    template<typename... T> inline UIntPtr Append(const String &Format, T... Args) {
        return VariadicFormat([](Char Data, Void *Context) -> Boolean {
            return static_cast<String*>(Context)->Append(Data) == Status::Success;
        }, static_cast<Void*>(this), Format, Args...);
    }

    Boolean Compare(const String&) const;

    Char *GetMutValue(Void) const;
    const Char *GetValue(Void) const { return Value; }
    UIntPtr GetLength(Void) const { return Length; }
    UIntPtr GetViewStart(Void) const { return ViewStart; }
    UIntPtr GetViewEnd(Void) const { return ViewEnd; }

    /* Operators for ranges-for and for accessing the string as a normal character array. */

    const Char *begin(Void) { return &Value[ViewStart]; }
    const Char *begin(Void) const { return &Value[ViewStart]; }
    const Char *end(Void) { return &Value[ViewEnd]; }
    const Char *end(Void) const { return &Value[ViewEnd]; }

    Char operator [](UIntPtr Index) const { return Value[ViewStart + Index]; }
private:
    Void CalculateLength(Void);

    Char *Value;
    UIntPtr Capacity, Length, ViewStart, ViewEnd;
};

Void CopyMemory(Void*, const Void*, UIntPtr);
Void SetMemory(Void*, UInt8, UIntPtr);
Void SetMemory32(Void*, UInt32, UIntPtr);
Void MoveMemory(Void*, const Void*, UIntPtr);
Boolean CompareMemory(const Void*, const Void*, UIntPtr);

}
