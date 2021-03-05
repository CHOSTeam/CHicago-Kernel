/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 07 of 2021, at 14:01 BRT
 * Last edited on March 05 of 2021 at 14:15 BRT */

#pragma once

#include <ds/list.hxx>
#include <util/misc.hxx>
#include <util/vararg.hxx>

namespace CHicago {

class String;

class StringView {
public:
    constexpr StringView(StringView &&Source)
        : Length { Exchange(Source.Length, 0) }, Value { Exchange(Source.Value, Null) } { }
    constexpr StringView(const Char *Value) : Length(0), Value(Value) { Length = CalculateLength(Value); }
    constexpr StringView(const StringView &Source) = default;
    constexpr StringView() : Length(0), Value(Null) { }
    StringView(const String&);

    constexpr StringView(const Char *Value, UIntPtr Length) : Length(Length), Value(Value) {
        UIntPtr len = CalculateLength(Value);

        if (Length > len) {
            this->Length = len;
        }
    }

    inline constexpr StringView &operator =(StringView &&Source) {
        if (this != &Source) {
            Length = Exchange(Source.Length, 0);
            Value = Exchange(Source.Value, Null);
        }

        return *this;
    }

    inline constexpr StringView &operator =(const StringView &Source) {
        if (this != &Source) {
            Length = Source.Length;
            Value = Source.Value;
        }

        return *this;
    }

    static StringView FromStatus(Status);
    static StringView FromInt(Char*, Int64, UIntPtr);
    static StringView FromUInt(Char*, UInt64, UIntPtr, UInt8);
    static StringView FromFloat(Char*, Float, UIntPtr, UIntPtr = 6);

    Int64 ToInt(UIntPtr&) const;
    Float ToFloat(UIntPtr&) const;
    UInt64 ToUInt(UIntPtr&, Boolean = False) const;
    inline Int64 ToInt() const { UIntPtr i = 0; return ToInt(i); }
    inline Float ToFloat() const { UIntPtr i = 0; return ToFloat(i); }
    inline UInt64 ToUInt(Boolean OnlyDec = False) const { UIntPtr i = 0; return ToUInt(i, OnlyDec); }

    Boolean Compare(const StringView&) const;
    Boolean StartsWith(const StringView&) const;
    List<String> Tokenize(const StringView&) const;

    inline constexpr UIntPtr GetLength() const { return Length; }
    inline constexpr const Char *GetValue() const { return Value; }

    /* Operators for ranges-for and for accessing the string as a normal character array. */

    inline ConstReverseIterator<Char, const Char*> Reverse() const { return { begin(), end() }; }

    inline constexpr const Char *begin() const { return Value; }
    inline constexpr const Char *end() const { return &Value[Length]; }

    inline constexpr Char operator [](UIntPtr Index) const { return Value[Index]; }
private:
    friend class String;

    static inline constexpr UIntPtr CalculateLength(const Char *Value) {
        UIntPtr ret = 0;
        for (; Value[ret]; ret++) ;
        return ret;
    }

    UIntPtr Length;
    const Char *Value;
};

class String {
public:
    String(UIntPtr);
    String(const Char*);
    String(const String&);
    String(String &&Source);
    String(const StringView&);
    String(ReverseIterator<Char, Char*>);
    String(const ConstReverseIterator<Char, const Char*>&);
    String() : Small {}, Capacity(0), Length(0), ViewStart(0), ViewEnd(0) { Value = Null; }

    ~String();

    String &operator =(String&&);
    String &operator =(const Char*);
    String &operator =(const String&);
    String &operator =(const StringView&);

    /* You maybe wondering: Hey, why doesn't the constructors allow initializing the string with formatted input? Well,
     * for this we have the Format function! And yeah, we actually need to make their body here as well, because of all
     * the template<> stuff. */

    template<typename... T> static inline String Format(const StringView &Format, T... Args) {
        /* First, let's create the string itself, we're going to use the 0-args initializer, as the Append function is
         * going to dynamically allocate the memory we need. */

        String str;
        return str.Append(Format, Args...), str;
    }

    Void Clear();
    Void SetView(UIntPtr, UIntPtr);

    inline Int64 ToInt() const { UIntPtr i = ViewStart; return ToInt(i); }
    inline Float ToFloat() const { UIntPtr i = ViewStart; return ToFloat(i); }
    inline Int64 ToInt(UIntPtr &Position) const { return StringView(*this).ToInt(Position); }
    inline Float ToFloat(UIntPtr &Position) const { return StringView(*this).ToFloat(Position); }
    inline UInt64 ToUInt(Boolean OnlyDec = False) const { UIntPtr i = ViewStart; return ToUInt(i, OnlyDec); }

    inline UInt64 ToUInt(UIntPtr &Position, Boolean OnlyDec = False) const {
        return StringView(*this).ToUInt(Position, OnlyDec);
    }

    Status Append(Char);
    inline UIntPtr Append(Int64 Value) { Char buf[65]; return Append(StringView::FromInt(buf, Value, 65)); }

    inline UIntPtr Append(UInt64 Value, UInt8 Base) {
        Char buf[65];
        return Append(StringView::FromUInt(buf, Value, 65, Base));
    }

    inline UIntPtr Append(Float Value, UIntPtr Precision = 6) {
        Char buf[65];
        return Append(StringView::FromFloat(buf, Value, 64, Precision));
    }

    /* The Append(String, ...) also needs to be inline, for the same reason as Format (because it is template<>). */

    template<typename... T> inline UIntPtr Append(const StringView &Format, T... Args) {
        return VariadicFormat([](Char Data, Void *Context) -> Boolean {
            return static_cast<String*>(Context)->Append(Data) == Status::Success;
        }, static_cast<Void*>(this), Format, Args...);
    }

    inline Boolean Compare(const StringView &Value) const { return StringView(*this).Compare(Value); }
    inline Boolean StartsWith(const StringView &Value) const { return StringView(*this).StartsWith(Value); }
    inline List<String> Tokenize(const StringView &Delimiters) const { return StringView(*this).Tokenize(Delimiters); }

    Char *GetValue() const;
    inline UIntPtr GetLength() const { return Length; }
    inline UIntPtr GetViewStart() const { return ViewStart; }
    inline UIntPtr GetViewEnd() const { return ViewEnd; }

    /* Operators for ranges-for and for accessing the string as a normal character array. */

    inline ReverseIterator<Char, Char*> Reverse() { return { begin(), end() }; }
    inline ConstReverseIterator<Char, const Char*> Reverse() const { return { begin(), end() }; }

    inline Char *begin() { return Capacity ? &Value[ViewStart] : &Small[ViewStart]; }
    inline const Char *begin() const { return Capacity ? &Value[ViewStart] : &Small[ViewStart]; }
    inline Char *end() { return Capacity ? &Value[ViewEnd] : &Small[ViewEnd]; }
    inline const Char *end() const { return Capacity ? &Value[ViewEnd] : &Small[ViewEnd]; }

    inline Char &operator [](UIntPtr Index) { return Capacity ? Value[ViewStart + Index] : Small[ViewStart + Index]; }

    inline Char operator [](UIntPtr Index) const {
        return Capacity ? Value[ViewStart + Index] : Small[ViewStart + Index];
    }
private:
    friend class StringView;

    union {
        Char *Value;
        Char Small[17];
    };

    UIntPtr Capacity, Length, ViewStart, ViewEnd;
};

Void SetMemory32(Void*, UInt32, UIntPtr);
Boolean CompareMemory(const Void*, const Void*, UIntPtr);

}
