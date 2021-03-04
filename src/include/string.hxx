/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 07 of 2021, at 14:01 BRT
 * Last edited on March 03 of 2021 at 12:16 BRT */

#pragma once

#include <list.hxx>
#include <vararg.hxx>

namespace CHicago {

class String {
public:
    String();
    String(UIntPtr);
    String(String&&);
    String(const String&);
    String(const Char*, Boolean = False);
    String(ReverseIterator<Char, Char*>);
    String(const ConstReverseIterator<Char, const Char*>&);

    ~String();

    String &operator =(String&&);
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

    Void Clear();

    Void SetView(UIntPtr, UIntPtr);

    Int64 ToInt(UIntPtr&) const;
    UInt64 ToUInt(UIntPtr&, Boolean = False) const;
    Float ToFloat(UIntPtr&) const;
    inline Int64 ToInt() const { UIntPtr i = ViewStart; return ToInt(i); }
    inline UInt64 ToUInt(Boolean OnlyDec = False) const { UIntPtr i = ViewStart; return ToUInt(i, OnlyDec); }
    inline Float ToFloat() const { UIntPtr i = ViewStart; return ToFloat(i); }

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
    Boolean StartsWith(const String&) const;
    List<String> Tokenize(const String&) const;

    Char *GetMutValue() const;
    inline const Char *GetValue() const { return Value; }
    inline UIntPtr GetLength() const { return Length; }
    inline UIntPtr GetViewStart() const { return ViewStart; }
    inline UIntPtr GetViewEnd() const { return ViewEnd; }

    inline ReverseIterator<Char, Char*> Reverse() { return { begin(), end() }; }
    inline ConstReverseIterator<Char, const Char*> Reverse() const { return { begin(), end() }; }

    /* Operators for ranges-for and for accessing the string as a normal character array. */

    inline Char *begin() { return &Value[ViewStart]; }
    inline const Char *begin() const { return &Value[ViewStart]; }
    inline Char *end() { return &Value[ViewEnd]; }
    inline const Char *end() const { return &Value[ViewEnd]; }

    inline Char &operator [](UIntPtr Index) { return Value[ViewStart + Index]; }
    inline Char operator [](UIntPtr Index) const { return Value[ViewStart + Index]; }
private:
    Void CalculateLength();

    Char *Value;
    UIntPtr Capacity, Length, ViewStart, ViewEnd;
};

Void SetMemory32(Void*, UInt32, UIntPtr);
Boolean CompareMemory(const Void*, const Void*, UIntPtr);

}