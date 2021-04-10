/* File author is Ítalo Lima Marconato Matias
 *
 * Created on March 15 of 2021, at 15:53 BRT
 * Last edited on March 15 of 2021 at 16:42 BRT */

#pragma once

#include <base/types.hxx>

namespace CHicago {

/* Compile-time static constant base type (and specialization for booleans). */

template<bool, class = Void> struct EnableIf {};
template<class T> struct EnableIf<True, T> { using Type = T; };
template<bool B, class T = Void> using EnableIfT = typename EnableIf<B, T>::Type;

template<class T, T V> struct CompileConstant { static constexpr T Value = V; };
template<bool V> using BoolConstant = CompileConstant<bool, V>;
using FalseConstant = BoolConstant<False>;
using TrueConstant = BoolConstant<True>;

template<class T> struct RemoveRef { using Type = T; };
template<class T> struct RemoveRef<T&> { using Type = T; };
template<class T> struct RemoveRef<T&&> { using Type = T; };
template<class T> using RemoveRefT = typename RemoveRef<T>::Type;

template<class T> struct RemoveConst { using Type = T; };
template<class T> struct RemoveConst<const T> { using Type = T; };
template<class T> using RemoveConstT = typename RemoveConst<T>::Type;

template<class T> struct RemoveVolatile { using Type = T; };
template<class T> struct RemoveVolatile<volatile T> { using Type = T; };
template<class T> using RemoveVolatileT = typename RemoveVolatile<T>::Type;

template<class T> struct RemoveCV { using Type = RemoveConstT<RemoveVolatileT<T>>; };
template<class T> using RemoveCVT = typename RemoveCV<T>::Type;

template<class T> struct RemoveRCV { using Type = RemoveRefT<RemoveCVT<T>>; };
template<class T> using RemoveRCVT = typename RemoveRCV<T>::Type;

/* Type properties. */

template<class T> struct IsConst : FalseConstant {};
template<class T> struct IsConst<const T> : TrueConstant {};
template<class T> inline constexpr bool IsConstV = IsConst<T>::Value;

template<class T> struct IsVolatile : FalseConstant {};
template<class T> struct IsVolatile<volatile T> : TrueConstant {};
template<class T> inline constexpr bool IsVolatileV = IsVolatile<T>::Value;

template<class T> struct IsLValRef : FalseConstant {};
template<class T> struct IsLValRef<T&> : TrueConstant {};
template<class T> inline constexpr bool IsLValRefV = IsLValRef<T>::Value;

template<class T> struct IsRValRef : FalseConstant {};
template<class T> struct IsRValRef<T&&> : TrueConstant {};
template<class T> inline constexpr bool IsRValRefV = IsRValRef<T>::Value;

template<class T> struct IsRef : BoolConstant<IsLValRefV<T> || IsRValRefV<T>> {};
template<class T> inline constexpr bool IsRefV = IsRef<T>::Value;

template<class T, class U> struct IsSame : FalseConstant {};
template<class T> struct IsSame<T, T> : TrueConstant {};
template<class T, class U> inline constexpr bool IsSameV = IsSame<T, U>::Value;

template<class T, class U> struct IsBaseOf : BoolConstant<__is_base_of(T, U)> {};
template<class T, class U> inline constexpr bool IsBaseOfV = IsBaseOf<T, U>::Value;

/* Basic/primary type (other than bool) specializations. */

template<class T> struct IsVoid : IsSame<Void, RemoveCVT<T>> {};
template<class T> inline constexpr bool IsVoidV = IsVoid<T>::Value;

template<class T> struct IsNull : IsSame<decltype(Null), RemoveCVT<T>> {};
template<class T> inline constexpr bool IsNullV = IsNull<T>::Value;

#define MAKE_SPEC(x) \
template<> struct IsInt<signed x> : TrueConstant {}; \
template<> struct IsInt<unsigned x> : TrueConstant {}

template<class T> struct IsInt : FalseConstant {};
template<> struct IsInt<bool> : TrueConstant {};
template<> struct IsInt<char> : TrueConstant {};
template<> struct IsInt<wchar_t> : TrueConstant {};
template<> struct IsInt<char8_t> : TrueConstant {};
template<> struct IsInt<char16_t> : TrueConstant {};
template<> struct IsInt<char32_t> : TrueConstant {};
MAKE_SPEC(char);
MAKE_SPEC(short);
MAKE_SPEC(int);
MAKE_SPEC(long);
MAKE_SPEC(long long);
template<class T> inline constexpr bool IsIntV = IsInt<T>::Value;
#undef MAKE_SPEC

template<class T> struct IsFloat : FalseConstant {};
template<> struct IsFloat<float> : TrueConstant {};
template<> struct IsFloat<double> : TrueConstant {};
template<> struct IsFloat<long double> : TrueConstant {};
template<class T> inline constexpr bool IsFloatV = IsFloat<T>::Value;

template<class T> struct IsArr : FalseConstant {};
template<class T> struct IsArr<T[]> : TrueConstant {};
template<class T, UIntPtr N> struct IsArr<T[N]> : TrueConstant {};
template<class T> inline constexpr bool IsArrV = IsArr<T>::Value;

template<class T> struct IsEnum : BoolConstant<__is_enum(T)> {};
template<class T> inline constexpr bool IsEnumV = IsEnum<T>::Value;

template<class T> struct IsUnion : BoolConstant<__is_union(T)> {};
template<class T> inline constexpr bool IsUnionV = IsUnion<T>::Value;

template<class T> struct IsClass : BoolConstant<__is_class(T)> {};
template<class T> inline constexpr bool IsClassV = IsClass<T>::Value;

template<class T> struct IsFunc : BoolConstant<!IsConstV<const T> && !IsRefV<T>> {};
template<class T> inline constexpr bool IsFuncV = IsFunc<T>::Value;

template<class T> struct __IsPtr : FalseConstant {};
template<class T> struct __IsPtr<T*> : TrueConstant {};
template<class T> struct IsPtr : __IsPtr<RemoveCVT<T>> {};
template<class T> inline constexpr bool IsPtrV = IsPtr<T>::Value;

/* Other specializations/traits. */

template<class T> struct IsArith : BoolConstant<IsIntV<T> || IsFloatV<T>> {};
template<class T> inline constexpr bool IsArithV = IsArith<T>::Value;

template<class T> struct IsFund : BoolConstant<IsArithV<T> || IsVoidV<T> || IsNullV<T>> {};
template<class T> inline constexpr bool IsFundV = IsFund<T>::Value;

template<class T> struct IsSigned : BoolConstant<IsArithV<T> && T(-1) < T(0)> {};
template<class T> inline constexpr bool IsSignedV = IsSigned<T>::Value;

}
