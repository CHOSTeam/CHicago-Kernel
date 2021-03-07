/* File author is Ítalo Lima Marconato Matias
 *
 * Created on March 05 of 2021, at 16:09 BRT
 * Last edited on March 06 of 2021 at 21:51 BRT */

#pragma once

#include <ds/list.hxx>

namespace CHicago {

class String;

class StringView {
public:
    constexpr StringView(StringView &&Source)
            : Value { Exchange(Source.Value, Null) }, Length { Exchange(Source.Length, 0) },
              ViewStart { Exchange(Source.ViewStart, 0) }, ViewEnd { Exchange(Source.ViewEnd, 0) } { }
    constexpr StringView() : Value(Null), Length(0), ViewStart(0), ViewEnd(0) { }
    constexpr StringView(const StringView &Source) = default;
    StringView(const String&);

    constexpr StringView(const Char *Value, UIntPtr Start = 0) : Value(Value), Length(0), ViewStart(Start), ViewEnd(0) {
        Length = CalculateLength(Value);

        if (Start > Length) {
            ViewStart = Length;
        }

        ViewEnd = Length - ViewStart;
    }

    constexpr StringView(const Char *Value, UIntPtr Start, UIntPtr End) : StringView(Value, Start) {
        if (End <= Length) {
            ViewEnd = End;
        }
    }

    inline constexpr StringView &operator =(StringView &&Source) {
        if (this != &Source) {
            Value = Exchange(Source.Value, Null);
            Length = Exchange(Source.Length, 0);
            ViewStart = Exchange(Source.ViewStart, 0);
            ViewEnd = Exchange(Source.ViewEnd, 0);
        }

        return *this;
    }

    inline constexpr StringView &operator =(const StringView &Source) {
        if (this != &Source) {
            Value = Source.Value;
            Length = Source.Length;
            ViewStart = Source.ViewStart;
            ViewEnd = Source.ViewEnd;
        }

        return *this;
    }

    static StringView FromStatus(Status);
    static StringView FromInt(Char*, Int64, UIntPtr);
    static StringView FromUInt(Char*, UInt64, UIntPtr, UInt8);
    static StringView FromFloat(Char*, Float, UIntPtr, UIntPtr = 6);

    Void SetView(UIntPtr, UIntPtr);

    Int64 ToInt(UIntPtr&) const;
    Float ToFloat(UIntPtr&) const;
    UInt64 ToUInt(UIntPtr&, Boolean = False) const;
    inline Int64 ToInt() const { UIntPtr i = ViewStart; return ToInt(i); }
    inline Float ToFloat() const { UIntPtr i = ViewStart; return ToFloat(i); }
    inline UInt64 ToUInt(Boolean OnlyDec = False) const { UIntPtr i = ViewStart; return ToUInt(i, OnlyDec); }

    Boolean Compare(const StringView&) const;
    Boolean StartsWith(const StringView&) const;
    List<String> Tokenize(const StringView&) const;

    inline constexpr UIntPtr GetLength() const { return Length; }
    inline constexpr const Char *GetValue() const { return Value; }
    inline constexpr UIntPtr GetViewStart() const { return ViewStart; }
    inline constexpr UIntPtr GetViewEnd() const { return ViewEnd; }

    /* Operators for ranges-for and for accessing the string as a normal character array. */

    inline ConstReverseIterator<Char, const Char*> Reverse() const { return { begin(), end() }; }

    inline constexpr const Char *begin() const { return Value + ViewStart; }
    inline constexpr const Char *end() const { return Value + ViewEnd; }

    inline constexpr Char operator [](UIntPtr Index) const { return Value[ViewStart + Index]; }
private:
    friend class String;

    static inline constexpr UIntPtr CalculateLength(const Char *Value) {
        UIntPtr ret = 0;
        for (; Value[ret]; ret++) ;
        return ret;
    }

    const Char *Value;
    UIntPtr Length, ViewStart, ViewEnd;
};

}
