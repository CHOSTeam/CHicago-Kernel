/* File author is Ítalo Lima Marconato Matias
 *
 * Created on June 26 of 2020, at 14:54 BRT
 * Last edited on December 01 of 2020, at 15:00 BRT */

#include <textout.hxx>

/* Global TextOutput implementations. */

TextOutput *Debug = Null;
TextOutput *Console = Null;

/* Some functions to write stuff without calling the formatted string version, we need to have them
 * separate as a kind of "interface" to the true functions as we need to call AfterWrite. */

Void TextOutput::Write(Char Value) {
	Lock.Lock();
	WriteInt(Value);
	AfterWrite();
	Lock.Unlock();
}

Void TextOutput::Write(const String &Value) {
	Lock.Lock();
	
	if (Value.ToCString() != Null) {
		WriteInt(Value.ToCString(), 0, ' ');
		AfterWrite();
	}
	
	Lock.Unlock();
}

Void TextOutput::Write(IntPtr Value, UInt8 Base, UIntPtr MinLength, Char PadChar) {
	Lock.Lock();
	WriteInt(Value, Base, MinLength, PadChar);
	AfterWrite();
	Lock.Unlock();
}

Void TextOutput::Write(UIntPtr Value, UInt8 Base, UIntPtr MinLength, Char PadChar) {
	Lock.Lock();
	WriteInt(Value, Base, MinLength, PadChar);
	AfterWrite();
	Lock.Unlock();
}

static UIntPtr ParseFlags(const Char *Start, UIntPtr &MinLength, Char &PadChar) {
	/* The three "flags" that we accept are: '0', which indicates that the string should be padded
	 * using zeroes, a single space, which indicates that the string should be padded using spaces,
	 * and numbers from 1-9, that indicates a minimum length.
	 * Goto does make our job a bit easier lol. */
	
	UIntPtr ret = 0;
	
s:	switch (*Start) {
	case '0':
	case ' ': {
		PadChar = *Start;
		ret++;
		Start++;
		goto s;
	}
	default: {
		if (!(*Start >= '0' && *Start <= '9')) {
			break;
		}
		
		while (*Start >= '0' && *Start <= '9') {
			MinLength = (MinLength * 10) + (*Start++ - '0');
			ret++;
		}
		
		goto s;
	}
	}
	
	return ret;
}

Void TextOutput::Write(const Char *Format, ...) {
	/* Handle null format pointers, and, initialize the variadic argument list. */
	
	if (Format == Null) {
		return;
	}
	
	VariadicList va;
	VariadicStart(va, Format);
	
	/* Let's parse the format string: If a % is detected, the next character says what we're going
	 * to print, 's' is a C string, 'S' is a CHicago string (pointer to one at least), 'c' is a
	 * single character, 'd' is a signed number (base 10), 'u' is an unsigned number (base 10), 'b'
	 * is a binary number (unsigned), 'B' is to write a some size in bytes (for example, the RAM size),
	 * in the smallest possible way, 'o' is an octal number (unsigned) and 'x' is a hex number (unsigned
	 * as well). After the % and before the format itself, the user can also specify if we need to pad
	 * the result. */
	
	Lock.Lock();
	
	for (UIntPtr i = 0; Format[i]; i++) {
		if (Format[i] != '%') {
			WriteInt(Format[i]);
		} else {
			UIntPtr min = 0;
			Char pad = ' ';
			
			i += ParseFlags(&Format[i + 1], min, pad) + 1;
			
			switch (Format[i]) {
			case 's': {
				WriteInt((Char*)VariadicArg(va, Char*), min, pad);
				break;
			}
			case 'S': {
				WriteInt(((String*)VariadicArg(va, String*))->ToCString(), min, pad);
				break;
			}
			case 'c': {
				/* We could later also let the char argument use the padding options... */
				WriteInt((Char)VariadicArg(va, IntPtr));
				break;
			}
			case 'd': {
				WriteInt((IntPtr)VariadicArg(va, IntPtr), 10, min, pad);
				break;
			}
			case 'u': {
				WriteInt((UIntPtr)VariadicArg(va, UIntPtr), 10, min, pad);
				break;
			}
			case 'b': {
				WriteInt((UIntPtr)VariadicArg(va, UIntPtr), 2, min, pad);
				break;
			}
			case 'B': {
				UIntPtr left, right;
				Char unit;
				
				/* We need to call our helper, and manually write each part of the number. */
				
				ByteHelper::Convert((Float)VariadicArg(va, UInt64), unit, left, right);
				WriteInt(left, 10);
				
				if (right > 0) {
					/* The right part is only written if it is bigger than 0. */
					
					WriteInt('.');
					WriteInt(right, 10, 2, '0');
				}
				
				if (unit != 0) {
					/* And the unit is only written if it's not 'B' (which is represented by a literal
					 * 0. */
					
					WriteInt(unit);
				}
				
				break;
			}
			case 'o': {
				WriteInt((UIntPtr)VariadicArg(va, IntPtr), 8, min, pad);
				break;
			}
			case 'x': {
				WriteInt((UIntPtr)VariadicArg(va, UIntPtr), 16, min, pad);
				break;
			}
			default: {
				WriteInt((Char)Format[i]);
				break;
			}
			}
		}
	}
	
	AfterWrite();
	Lock.Unlock();
	
	/* Tell the compiler that we're done using the variadic list, there are some architectures that need
	 * to do some clean up. */
	
	VariadicEnd(va);
}

Void TextOutput::WriteInt(const Char *Value, UIntPtr MinLength, Char PadChar) {
	/* This function does the magic of handling the padding of a string (and remember that everything
	 * except characters will become a string and pass through this function), so, let's start by
	 * filling what we need on the left side (before the string).
	 * Before starting to do the padding, we need to get the length of the string. */
	
	if (Value == Null) {
		Value = "";
	}
	
	UIntPtr len = 0;
	
	for (; Value[len]; len++) ;
	
	/* Now, in the case the value length is smaller than the minimum length, we need to fill the left
	 * side with the PadChar, until the length that we filled + the length of the string will be the
	 * minimum length. */
	
	for (; len < MinLength; len++) {
		WriteInt(PadChar);
	}
	
	/* And finally, we can just write the string itself. */
	
	for (; *Value; Value++) {
		WriteInt(*Value);
	}
}

Void TextOutput::WriteInt(IntPtr Value, UInt8 Base, UIntPtr MinLength, Char PadChar) {
	/* First, handle invalid bases and the case where the value is zero. */
	
	if (Base < 2 || Base > 36) {
		return;
	} else if (Value == 0) {
		WriteInt("0", MinLength, PadChar);
		return;
	}
	
	/* Now, we need to create a temp buffer that can handle the max number on the min base.
	 * The max number would depend on the architecture, but let's put it as 2^64-1, the 64-bits
	 * limit, so, on the lowest base (base-2), it would take 64 characters + the string terminator
	 * in the end. BUT, this is takes signed integers, so we can actually ignore the first bit,
	 * as it is the sign of the number, still, this doesn't change the size, as we need to take
	 * in consideration the minus sign. */
	
	Char buf[65] = { 0 };
	Boolean neg = Value < 0;
	IntPtr i = 63;
	
	if (neg) {
		Value *= -1;
	}
	
	/* Now proceed to fill the string, and, after filling it, call ->WriteInt. */
	
	for (; Value && i >= 0; i--, Value /= Base) {
		buf[i] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[Value % Base];
	}
	
	/* But before calling WriteInt, put the minus sign if required (and if we have space). */
	
	if (neg && i >= 0) {
		buf[i--] = '-';
	}
	
	WriteInt(&buf[i + 1], MinLength, PadChar);
}

Void TextOutput::WriteInt(UIntPtr Value, UInt8 Base, UIntPtr MinLength, Char PadChar) {
	/* Here we only handle unsigned number, so no need to check for negative numbers, but we
	 * still need to check for 0 and for invalid bases. */
	
	if (Base < 2 || Base > 36) {
		return;
	} else if (Value == 0) {
		WriteInt("0", MinLength, PadChar);
		return;
	}
	
	/* Create the buffer, fill it, and call ->WriteInt. */
	
	Char buf[65] = { 0 };
	IntPtr i = 63;
	
	for (; Value && i >= 0; i--, Value /= Base) {
		buf[i] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[Value % Base];
	}
	
	WriteInt(&buf[i + 1], MinLength, PadChar);
}
