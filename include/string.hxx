/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 07 of 2021, at 14:01 BRT
 * Last edited on February 16 of 2021 at 14:38 BRT */

#pragma once

#include <status.hxx>

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

    static String FromInt(Char*, Int64, UIntPtr, UInt8);
    static String FromUInt(Char*, UInt64, UIntPtr, UInt8);

    /* You maybe wondering: Hey, why doesn't the constructors allow initializing the string with formatted input? Well,
     * for this we have the Format function! And yeah, we actually need to make their body here as well, because of all
     * the template<> stuff. */

    template<typename... Args> static String Format(const Char *Format, Args... VaArgs) {
        /* First, let's create the string itself, we're going to use the 0-args initializer, as the Append function is
         * going to dynamically allocate the memory we need. */

        String str;

        /* Now we just need to call the Append function. */

        if (Format == Null) {
            return str;
        } else if (str.Append(Format, VaArgs...) != Status::Success) {
            return {};
        }

        return str;
    }

    Void Clear(Void);

    Status Append(Char);
    Status Append(Int64, UInt8);
    Status Append(UInt64, UInt8);
    Status Append(const String&, ...);

    Boolean Compare(const String&) const;

    Char *GetMutValue(Void) const;
    const Char *GetValue(Void) const { return Value; }
    UIntPtr GetLength(Void) const { return Length; }

    /* Operators for ranges-for and for accessing the string as a normal character array. */

    const Char *begin(Void) { return &Value[0]; }
    const Char *begin(Void) const { return &Value[0]; }
    const Char *end(Void) { return &Value[Length]; }
    const Char *end(Void) const { return &Value[Length]; }

    Char operator [](UIntPtr Index) const { return Index >= Length ? '\0' : Value[Index]; }
private:
    static Void FromUInt(Char*, UInt64, UInt8, IntPtr&, IntPtr);
    Void CalculateLength(Void);

    Char *Value;
    UIntPtr Capacity, Length;
};

UIntPtr VariadicFormat(const String&, VariadicList&, Boolean (*)(Char, Void*), Void*, Char*, UIntPtr, UIntPtr);

Void CopyMemory(Void*, const Void*, UIntPtr);
Void SetMemory(Void*, UInt8, UIntPtr);
Void SetMemory32(Void*, UInt32, UIntPtr);
Void MoveMemory(Void*, const Void*, UIntPtr);
Boolean CompareMemory(const Void*, const Void*, UIntPtr);

}
