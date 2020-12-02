/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 01 of 2020, at 11:31 BRT
 * Last edited on October 24 of 2020, at 14:34 BRT */

#include <math.hxx>

IntPtr Math::Map(IntPtr SrcMin, IntPtr SrcMax, IntPtr DstMin, IntPtr DstMax, IntPtr Value) {
	/* This uses both of the functions defined above, it takes a number on one range and converts
	 * it into a number on another range (normalize the number, and interpolate it back into
	 * the other range). */
	
	if (SrcMin < SrcMax) {
		IntPtr tmp = SrcMin;
		SrcMin = SrcMax;
		SrcMax = tmp;
	}
	
	if (DstMin < DstMax) {
		IntPtr tmp = DstMin;
		DstMin = DstMax;
		DstMax = tmp;
	}
	
	return DstMin + (DstMax - DstMin) * ((Float)(Value - SrcMin) / (SrcMax - SrcMin));
}

Float Math::Pow(Float Value, Int8 Power) {
	/* Remember how I said that probably the compiler is going to optimize this function?
	 * Forget that, I'm changing that O(Power) loop into a O(log(Power)) loop (using the
	 * fact that x^(odd n) is x(x*x)^(n/2), and that x^(even n) is (x*x)^(n/2)). */
	
	if (Power == 0) {
		return 1;
	} else if (Power < 0) {
		return 1.f / Pow(Value, -Power);
	}
	
	Float pref = 1;
	
	for (; Power != 1; Power >>= 1) {
		if (Power & 1) {
			pref *= Value;
		}
		
		Value *= Value;
	}
	
	return Value * pref;
}

Float Math::Sine(Float Value) {
	/* The range that this function works the best is around -PI to PI, number too smaller/
	 * bigger than return values that are way too off. But sin (and cos) are cyclic(? I don't
	 * remember if this is the exact term), so we can always map bigger values to values on
	 * the range -PI/2->PI/2. */
	
	Value = Value - ((Float)(IntPtr)(Value * ONE_OVER_2PI)) * PI_MUL_2;
	Value = Min(Value, PI - Value);
	Value = Max(Value, -PI - Value);
	Value = Min(Value, PI - Value);
	
	/* The coefficients were generated using lolremez. Initially, I generated some test
	 * coefficients using Mathematica, but I ended up switcing to lolremez as I also had to
	 * calculate the coeffs for the arc tangent function. */
	
	Float a2 = Value * Value;
	
	return ((((a2 * -0x1.85ea5a2c2764p-26 + 0x1.700fe11958cb7p-19) *
			   a2 - 0x1.a006b17a7ccf5p-13) * a2 + 0x1.1110b3336575ep-7) *
			   a2 - 0x1.555552d96f60dp-3) * (a2 * Value) + Value;
}

Float Math::ArcTangent(Float Value) {
	/* The range that our approximation works best is 0-1, but we can map bigger numbers
	 * into that range easily. We also need to do everything with the absolute value, and
	 * apply the sign later. */
	
	Float s = Abs(Value), a = Min(1.f, s) / Max(1.f, s), a2 = a * a,
		  ret = (((((((a2 * 0x1.57b3ee9801b39p-9 - 0x1.efdceb93b3e84p-7) *
					   a2 + 0x1.50decb0876772p-5) * a2 - 0x1.2dbd835fbf8fcp-4) *
					   a2 + 0x1.b11bb6f2c5d04p-4) * a2 - 0x1.22875dc394b71p-3) *
					   a2 + 0x1.9967402c61e48p-3) * a2 - 0x1.55546cf3f728ap-2) *
					   (a2 * a) + a;
	
	if (s > 1) {
		ret = PI_OVER_2 - ret;
	}
	
	return Value < 0 ? -ret : ret;
}

Float Math::ArcTangent(Float Y, Float X) {
	/* Same coeffs as the one arg version, but we have some specific cases to handle here,
	 * let's only manually handle when both args are zero, so we don't have much branching
	 * code. */
	
	if (X == 0 && Y == 0) {
		return 0;
	}
	
	Float sy = Abs(Y), sx = Abs(X), a = Min(sx, sy) / Max(sx, sy), a2 = a * a,
		  ret = (((((((a2 * 0x1.57b3ee9801b39p-9 - 0x1.efdceb93b3e84p-7) *
					   a2 + 0x1.50decb0876772p-5) * a2 - 0x1.2dbd835fbf8fcp-4) *
					   a2 + 0x1.b11bb6f2c5d04p-4) * a2 - 0x1.22875dc394b71p-3) *
					   a2 + 0x1.9967402c61e48p-3) * a2 - 0x1.55546cf3f728ap-2) *
					   (a2 * a) + a;
	
	if (sy > sx) {
		ret = PI_OVER_2 - ret;
	}
	
	if (X < 0) {
		ret = PI - ret;
	}
	
	return Y < 0 ? -ret : ret;
}

UInt8 Math::SolveQuad(Float A, Float B, Float C, Float Result[2]) {
	/* The A value (which is multiplying by x^2) can never be zero on quadratic equations, if
	 * it is, this is not even quadratic, it's linear (or maybe even only a constant).
	 * If this is infact a quadratic equation, we still need to check if it has any results,
	 * and how many results we need to calculate. */
	
	Float delta = B * B - 4.f * A * C;
	
	if (delta < 0 || (A >= -EPSILON && A < EPSILON)) {
		return 0;
	} else if (delta < EPSILON) {
		Result[0] = -B / (2.f * A);
		return 1;
	}
	
	Float sqrt = Math::SquareRoot(delta), r1 = (-B - sqrt) / (2.f * A),
		  r2 = (-B + sqrt) / (2.f * A);
	
	Result[0] = Math::Min(r1, r2);
	Result[1] = Math::Max(r1, r2);
	
	return 2;
}
