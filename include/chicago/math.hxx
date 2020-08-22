/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 01 of 2020, at 11:28 BRT
 * Last edited on August 22 of 2020, at 11:41 BRT */

#ifndef __CHICAGO_MATH_HXX__
#define __CHICAGO_MATH_HXX__

#include <chicago/types.hxx>

/* Comparing floats by just doing A == B (or A != B) is dangerous, instead, you should do abs(A - B) and compare if the value is
 * lower/greater than the epsilon. */

#define EPSILON 0x1.6849b86a12b9bp-47

#define PI 0x1.921fb54442d18p+1
#define PI_MUL_2 0x1.921fb54442d18p+2
#define PI_OVER_2 0x1.921fb54442d18p+0
#define PI_OVER_4 0x1.921fb54442d18p-1
#define ONE_OVER_PI 0x1.45f306dc9c883p-2
#define ONE_OVER_2PI 0x1.45f306dc9c883p-3
#define DEGREE_RAD 0x1.ca5dc1a63c1f8p+5

class Math {
public:
	static IntPtr Map(IntPtr, IntPtr, IntPtr, IntPtr, IntPtr);
	static inline Float Abs(Float Value) { return __builtin_fabs(Value); }
	static inline IntPtr Abs(IntPtr Value) { return __builtin_abs(Value); }
	static inline IntPtr Round(Float Value) { return __builtin_roundf(Value); }
	static inline Float Min(Float Value1, Float Value2) { return __builtin_fmin(Value1, Value2); }
	static inline IntPtr Min(IntPtr Value1, IntPtr Value2) { return Value1 < Value2 ? Value1 : Value2; }
	static inline UIntPtr Min(UIntPtr Value1, UIntPtr Value2) { return Value1 < Value2 ? Value1 : Value2; }
	static inline Float Max(Float Value1, Float Value2) { return __builtin_fmax(Value1, Value2); }
	static inline IntPtr Max(IntPtr Value1, IntPtr Value2) { return Value1 > Value2 ? Value1 : Value2; }
	static inline UIntPtr Max(UIntPtr Value1, UIntPtr Value2) { return Value1 > Value2 ? Value1 : Value2; }
	static Float Pow(Float, UInt8);
	static inline Float SquareRoot(Float Value) { return __builtin_sqrt(Value); }
	static Float Sine(Float);
	static inline Float Cosine(Float Value) { return Sine(Value + PI_OVER_2); }
	static Float ArcTangent(Float);
	static Float ArcTangent(Float, Float);
	static UInt8 SolveQuad(Float, Float, Float, Float[2]);
};

#endif
