/* File author is Ítalo Lima Marconato Matias
 *
 * Created on May 11 of 2018, at 13:15 BRT
 * Last edited on March 04 of 2021, at 17:04 BRT */

#pragma once

namespace CHicago {

#define VERSION "next-4"

typedef void Void;
typedef char Char;

/* Unsigned types */

typedef unsigned long ULong;
typedef unsigned char UInt8;
typedef unsigned short UInt16;
typedef unsigned int UInt32;
typedef unsigned long long UInt64;

/* Signed types */

typedef signed long Long;
typedef signed char Int8;
typedef signed short Int16;
typedef signed int Int32;
typedef signed long long Int64;

/* Define our IntPtr type */

#ifdef _LP64
#define UINTPTR_MAX 0xFFFFFFFFFFFFFFFF
#define INTPTR_MIN -9223372036854775807
#define INTPTR_MAX 9223372036854775807

typedef unsigned long long UIntPtr;
typedef signed long long IntPtr;
#else
#define UINTPTR_MAX 0xFFFFFFFF
#define INTPTR_MIN -2147483648
#define INTPTR_MAX 2147483647

typedef unsigned int UIntPtr;
typedef signed int IntPtr;
#endif

typedef double Float;

/* Attributes */

#define packed __attribute__((packed))
#define unused __attribute__((unused))
#define no_return __attribute__((noreturn))
#define no_inline __attribute__((noinline))
#define section(x) __attribute__((section(x)))
#define aligned(x) __attribute__((aligned(x)))
#define always_inline __attribute__((always_inline))
#define disable_ubsan __attribute__((no_sanitize("undefined")))
#define vector_size(x) __attribute__((vector_size(x), may_alias))
#define force_align_arg_pointer __attribute__((force_align_arg_pointer))

/* Boolean and other defines */

typedef bool Boolean;

#define True true
#define False false

#define Null nullptr

}
