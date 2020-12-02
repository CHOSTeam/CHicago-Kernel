/* File author is Ítalo Lima Marconato Matias
 *
 * Created on May 11 of 2018, at 13:15 BRT
 * Last edited on October 27 of 2020, at 08:56 BRT */

#ifndef __TYPES_HXX__
#define __TYPES_HXX__

#define Void void

/* Unsigned types */

#define UInt8 unsigned char
#define UInt16 unsigned short
#define UInt32 unsigned int
#define ULong unsigned long
#define UInt64 unsigned long long

/* Signed types */

#define Int8 signed char
#define Int16 signed short
#define Int32 signed int
#define Int64 signed long long

/* Define our IntPtr type */

#ifdef ARCH_64
#define UINTPTR_MAX 0xFFFFFFFFFFFFFFFF
#define INTPTR_MIN -9223372036854775807
#define INTPTR_MAX 9223372036854775807
#define UIntPtr unsigned long long
#define IntPtr signed long long
#else
#define UINTPTR_MAX 0xFFFFFFFF
#define INTPTR_MIN -2147483648
#define INTPTR_MAX 2147483647
#define UIntPtr unsigned int
#define IntPtr signed int
#endif

/* Other types */

#define Char char
#define Short short
#define Int int
#define Long long

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
#define no_return __attribute__((noreturn))
#define no_inline __attribute__((noinline))
#define section(x) __attribute__((section(x)))

/* Boolean and other defines */

typedef Char Boolean;

#define True 1
#define False 0

#define Null nullptr

#define Auto auto

#define TextifyMacro1(n) TextifyMacro2(#n)
#define TextifyMacro2(n) L##n
#define TextifyMacro3(n) #n
#define TextifyMacro(n) TextifyMacro1(n)
#define TextifyMacroC(n) TextifyMacro3(n)

/* We use this for aligning an allocation (from ::new or ::delete), the type name NEEDS to be align_val_t btw.
 * And yes, this is hackish as hell, I know. */

#define AlignAlloc align_val_t
namespace std { enum class AlignAlloc : Long UInt32 { }; }
using namespace std;

#endif
