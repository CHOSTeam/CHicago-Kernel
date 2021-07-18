/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 07 of 2021, at 14:01 BRT
 * Last edited on July 17 of 2021 at 21:42 BRT */

#pragma once

#include <base/string_view.hxx>
#include <util/vararg.hxx>

namespace CHicago {

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
        return VariadicFormat([](UInt8 Type, UInt32 Data, Void *Context) -> Boolean {
            return !Type ? static_cast<String*>(Context)->Append(static_cast<Char>(Data)) == Status::Success : True;
        }, static_cast<Void*>(this), Format, Args...);
    }

    inline Boolean Compare(const StringView &Value) const { return StringView(*this).Compare(Value); }
    inline Boolean StartsWith(const StringView &Value) const { return StringView(*this).StartsWith(Value); }

    inline List<String> Tokenize(const StringView &Delimiters) const { return StringView(*this).Tokenize(Delimiters); }

    Char *GetValue() const;
    inline UIntPtr GetLength() const { return Length; }
    inline UIntPtr GetViewLength() const { return ViewEnd - ViewStart; }
    inline UIntPtr GetViewStart() const { return ViewStart; }
    inline UIntPtr GetViewEnd() const { return ViewEnd; }

    /* Operators for ranges-for and for accessing the string as a normal character array. */

    inline ReverseIterator<Char, Char*> Reverse() { return { begin(), end() }; }
    inline ConstReverseIterator<Char, const Char*> Reverse() const { return { begin(), end() }; }

    inline Char *begin() { return (Capacity ? Value : Small) + ViewStart; }
    inline const Char *begin() const { return (Capacity ? Value : Small) + ViewStart; }
    inline Char *end() { return (Capacity ? Value : Small) + ViewEnd; }
    inline const Char *end() const { return (Capacity ? Value : Small) + ViewEnd; }

    inline Char &operator [](UIntPtr Index) { return (Capacity ? Value : Small)[ViewStart + Index]; }
    inline Char operator [](UIntPtr Index) const { return (Capacity ? Value : Small)[ViewStart + Index]; }
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
