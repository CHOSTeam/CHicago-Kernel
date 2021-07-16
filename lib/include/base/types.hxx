/* File author is Ítalo Lima Marconato Matias
 *
 * Created on May 11 of 2018, at 13:15 BRT
 * Last edited on July 16 of 2021, at 10:40 BRT */

#pragma once

namespace CHicago {

#define VERSION "next-6"

using Void = void;
using Char = char;

/* Unsigned types */

using ULong = unsigned long;
using UInt8 = unsigned char;
using UInt16 = unsigned short;
using UInt32 = unsigned int;
using UInt64 = unsigned long long;

/* Signed types */

using Long = signed long;
using Int8 = signed char;
using Int16 = signed short;
using Int32 = signed int;
using Int64 = signed long long;

/* Define our IntPtr type */

#ifdef _LP64
#define UINTPTR_MAX 0xFFFFFFFFFFFFFFFF
#define INTPTR_MIN -9223372036854775807
#define INTPTR_MAX 9223372036854775807

using UIntPtr = unsigned long long;
using IntPtr = signed long long;
#else
#define UINTPTR_MAX 0xFFFFFFFF
#define INTPTR_MIN -2147483648
#define INTPTR_MAX 2147483647

using UIntPtr = unsigned int;
using IntPtr = signed int;
#endif

using Float = double;

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

using Boolean = bool;

#define True true
#define False false

#define Null nullptr

}
