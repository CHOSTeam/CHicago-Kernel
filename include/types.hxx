/* File author is Ítalo Lima Marconato Matias
 *
 * Created on May 11 of 2018, at 13:15 BRT
 * Last edited on February 17 of 2021, at 10:21 BRT */

#pragma once

#define VERSION "next-4"

#define Void void

#define Char char

/* Unsigned types */

#define UInt8 unsigned char
#define UInt16 unsigned short
#define UInt32 unsigned int
#define UInt64 unsigned long long

/* Signed types */

#define Int8 signed char
#define Int16 signed short
#define Int32 signed int
#define Int64 signed long long

/* Define our IntPtr type */

#ifdef _LP64
#define UINTPTR_MAX_HEX "0x%016X"
#define UINTPTR_HEX "0x%X"
#define UINTPTR_DEC "%U"

#define UINTPTR_MAX 0xFFFFFFFFFFFFFFFF
#define INTPTR_MIN -9223372036854775807
#define INTPTR_MAX 9223372036854775807

#define UIntPtr unsigned long long
#define IntPtr signed long long
#else
#define UINTPTR_MAX_HEX "0x%08x"
#define UINTPTR_HEX "0x%x"
#define UINTPTR_DEC "%u"

#define UINTPTR_MAX 0xFFFFFFFF
#define INTPTR_MIN -2147483648
#define INTPTR_MAX 2147483647

#define UIntPtr unsigned int
#define IntPtr signed int
#endif

/* I know it's a bit confusing, but our default float type is double... */

#define Float double

/* Variadic arguments */

#define VariadicList __builtin_va_list
#define VariadicStart(v, l) __builtin_va_start(v, l)
#define VariadicEnd(v) __builtin_va_end(v)
#define VariadicArg(v, l) __builtin_va_arg(v, l)
#define VariadicCopy(d, s) __builtin_va_copy(d, s)

/* Attributes */

#define packed __attribute__((packed))
#define unused __attribute__((unused))
#define no_return __attribute__((noreturn))
#define no_inline __attribute__((noinline))
#define section(x) __attribute__((section(x)))
#define always_inline __attribute__((always_inline))
#define force_align_arg_pointer __attribute__((force_align_arg_pointer))

/* A few builtin functions. */

#define AssumeAligned(x, s) __builtin_assume_aligned(x, s)

/* Boolean and other defines */

typedef bool Boolean;

#define True true
#define False false

#define Null nullptr

/* We use this for aligning an allocation (from ::new or ::delete), the type name NEEDS to be align_val_t btw. nd yes,
 * this is hackish as hell, I know. */

#define AlignAlloc align_val_t
namespace std { enum class AlignAlloc : long UInt32 { }; }
using namespace std;
