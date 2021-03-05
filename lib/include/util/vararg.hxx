/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 22 of 2021, at 15:27 BRT
 * Last edited on March 05 of 2021 at 14:12 BRT */

#pragma once

#include <base/status.hxx>

namespace CHicago {

class String;
class StringView;

/* The Long and ULong types are required for 64-bits support (else, we gonna get some errors to do with ambiguity). */

enum class ArgumentType {
    Float, Long, Int32, Int64,
    ULong, UInt32, UInt64,
    Char, Boolean, Pointer, CString,
    Status, CHString, CHStringView
};

union ArgumentValue {
    Char CharValue;
    Long LongValue;
    Float FloatValue;
    ULong ULongValue;
    Int32 Int32Value;
    Int64 Int64Value;
    UInt32 UInt32Value;
    UInt64 UInt64Value;
    Status StatusValue;
    Boolean BooleanValue;
    const Void *PointerValue;
    const Char *CStringValue;
    const String *CHStringValue;
    const StringView *CHStringViewValue;
};

class Argument {
public:
    /* And yes, we need one constructor for each argument type (and let's already inline everything). Any new valid arg
     * type should also be added here. */

    Argument(Char Value) : Type(ArgumentType::Char), Value({ .CharValue = Value }) { }
    Argument(Long Value) : Type(ArgumentType::Long), Value({ .LongValue = Value }) { }
    Argument(Float Value) : Type(ArgumentType::Float), Value({ .FloatValue = Value }) { }
    Argument(ULong Value) : Type(ArgumentType::ULong), Value({ .ULongValue = Value }) { }
    Argument(Int32 Value) : Type(ArgumentType::Int32), Value({ .Int32Value = Value }) { }
    Argument(Int64 Value) : Type(ArgumentType::Int64), Value({ .Int64Value = Value }) { }
    Argument(UInt32 Value) : Type(ArgumentType::UInt32), Value({ .UInt32Value = Value }) { }
    Argument(UInt64 Value) : Type(ArgumentType::UInt64), Value({ .UInt64Value = Value }) { }
    Argument(Status Value) : Type(ArgumentType::Status), Value({ .StatusValue = Value }) { }
    Argument(Boolean Value) : Type(ArgumentType::Boolean), Value({ .BooleanValue = Value }) { }
    Argument(const Void *Value) : Type(ArgumentType::Pointer), Value({ .PointerValue = Value }) { }
    Argument(const Char *Value) : Type(ArgumentType::CString), Value({ .CStringValue = Value }) { }
    Argument(const String &Value) : Type(ArgumentType::CHString), Value({ .CHStringValue = &Value }) { }
    Argument(const StringView &Value) : Type(ArgumentType::CHStringView), Value({ .CHStringViewValue = &Value }) { }

    inline ArgumentType GetType() const { return Type; }
    inline ArgumentValue GetValue() const { return Value; }
private:
    ArgumentType Type;
    ArgumentValue Value;
};

class ArgumentList {
public:
    ArgumentList(UIntPtr Count, const Argument *List) : Count(Count), List(List) { }

    inline const Argument &operator [](UIntPtr Index) const { return List[Index]; }

    inline UIntPtr GetCount() const { return Count; }
    inline const Argument *GetList() const { return List; }
private:
    UIntPtr Count;
    const Argument *List;
};

/* Let's already define our variadic formatting function here, there will actually be two of them, one that takes the
 * ArgumentList, and one that takes normal varargs, the second one will convert the varargs into an ArgumentList, and
 * redirect (and as it is a template<> function, it needs to be inline). */

UIntPtr VariadicFormatInt(Boolean (*)(Char, Void*), Void*, const StringView&, const ArgumentList&);

template<typename... T> static inline UIntPtr VariadicFormat(Boolean (*Function)(Char, Void*), Void *Context,
                                                             const StringView &Format, T... Args) {
    Argument list[] { Args... };
    return VariadicFormatInt(Function, Context, Format, ArgumentList(sizeof...(Args), list));
}

}
