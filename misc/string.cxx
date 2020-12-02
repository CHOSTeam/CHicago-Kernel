/* File author is Ítalo Lima Marconato Matias
 *
 * Created on June 25 of 2020, at 11:52 BRT
 * Last edited on November 28 of 2020, at 11:24 BRT */

#include <string.hxx>

String::String() : Value(Null), Length(0), Allocated(0) { }

String::String(UIntPtr Length) : Length(0), Allocated(0) {
	Value = Length != 0 ? new Char[Length + 1] : Null;
	
	if (Value != Null) {
		Allocated = Length + 1;
	}
}

String::String(const String &Value) : Value(Null), Length(0), Allocated(0) {
	if (Value.Value != Null) {
		Append(Value);
	}
}

String::String(const Char *Value, Boolean Alloc) {
	/* Init all the fields normally, we don't need to allocate extra memory here. */
	
	for (Length = Allocated = 0; Value[Length]; Length++) ;
	
	this->Value = (Alloc && Length > 0) ? new Char[Length + 1] : (Char*)Value;
	
	if (Alloc && this->Value != Value) {
		Allocated = Length + 1;
		Length = 0;
		Append(Value);
	}
}

String::~String(Void) {
	/* If the allocated size (stored in the variable named Allocated) is greater than 0, we need to
	 * delete[] the array. Also remember to set the allocated size to 0, as we may be called multiple
	 * times accidentally (probably not going to happen, but let's do it just to be sure). */
	
	if (Allocated > 0) {
		delete[] Value;
		Allocated = 0;
	}
}

/* Assigning a new value is just a question of clearing the current contents, and appending the new
 * contents. */

String &String::operator =(const String &Source) {
	if (&Source != this) {
		Clear();
		Append(Source);
	}
	
	return *this;
}

String &String::operator =(const Char *Source) {
	if ((Char*)Source != Value) {
		Clear();
		
		Value = (Char*)Source;
		
		if (Value != Null) {
			for (; Value[Length]; Length++) ;
		}
	}
	
	return *this;
}

Void String::Clear(Void) {
	/* And clearing is just deallocating (if required) and setting the Length and the Allocated variables
	 * to 0. */
	
	if (Allocated > 0 && Value != Null) {
		delete[] Value;
		Value = Null;
	}
	
	Allocated = Length = 0;
}

Status String::Append(Char Value) {
	/* First, if our allocated buffer wasn't actually allocated, we gonna need to allocate it, if it is
	 * too small, we need to resize it. */
	
	if (Allocated == 0 || Length + 1 >= Allocated) {
		UIntPtr nlen = Length < 4 ? 4 : Length * 2 + 1;
		Char *buf = new Char[nlen];
		
		if (buf == Null) {
			return Status::OutOfMemory;
		}
		
		StrCopyMemory(buf, this->Value, Length);
		
		if (Allocated != 0) {
			delete[] this->Value;
		}
		
		this->Value = buf;
		Allocated = nlen;
	}
	
	/* Now, just add the character to the end of the array, and put the string end character. */
	
	this->Value[Length++] = Value;
	this->Value[Length] = 0;
	
	return Status::Success;
}

Status String::Append(IntPtr Value, UInt8 Base, UIntPtr MinLength, Char PadChar) {
	/* First, handle invalid bases and the case where the value is zero. */
	
	if (Base < 2 || Base > 36) {
		return Status::InvalidArg;
	} else if (Value == 0) {
		return Append("0", MinLength, PadChar);
	}
	
	/* Now, we need to create a temp buffer that can handle the max number on the min base.
	 * The max number would depend on the architecture, but let's put it as 2^64-1, the 64-bits
	 * limit, so, on the lowest base (base-2), it would take 64 characters + the string terminator
	 * in the end. BUT, this is takes signed integers, so we can actually ignore the first bit, as
	 * it is the sign of the number, still, this doesn't change the size, as we need to take in
	 * consideration the minus sign. */
	
	Char buf[65] = { 0 };
	Boolean neg = Value < 0;
	IntPtr i = 63;
	
	if (neg) {
		Value *= -1;
	}
	
	/* Now proceed to fill the string, and, after filling it, call ->AppendInt. */
	
	for (; Value && i >= 0; i--, Value /= Base) {
		buf[i] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[Value % Base];
	}
	
	/* But before calling Append, put the minus sign if required (and if we have space). */
	
	if (neg && i >= 0) {
		buf[i--] = '-';
	}
	
	return AppendInt(&buf[i + 1], MinLength, PadChar);
}

Status String::Append(UIntPtr Value, UInt8 Base, UIntPtr MinLength, Char PadChar) {
	/* Here we only handle unsigned number, so no need to check for negative numbers, but we
	 * still need to check for 0 and for invalid bases. */
	
	if (Base < 2 || Base > 36) {
		return Status::InvalidArg;
	} else if (Value == 0) {
		return Append("0", MinLength, PadChar);
	}
	
	/* Create the buffer, fill it, and call ->AppendInt. */
	
	Char buf[65] = { 0 };
	IntPtr i = 63;
	
	for (; Value && i >= 0; i--, Value /= Base) {
		buf[i] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[Value % Base];
	}
	
	return AppendInt(&buf[i + 1], MinLength, PadChar);
}

Status String::Append(const String &Value) {
	/* Just get the internal C string out of the CHicago string, and recall Append, passing the C
	 * string as the argument. */
	
	if (Value.Value == Null) {
		return Status::Success;
	}
	
	return AppendInt(Value.Value, 0, ' ');
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

Status String::Append(const Char *Format, ...) {
	if (Format == Null) {
		return Status::InvalidArg;
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
	
	for (UIntPtr i = 0; Format[i]; i++) {
		Status res;
		
		if (Format[i] != '%') {
			res = Append(Format[i]);
		} else {
			UIntPtr min = 0;
			Char pad = ' ';
			
			i += ParseFlags((const Char*)&Format[i + 1], min, pad) + 1;
			
			switch (Format[i]) {
			case 's': {
				res = AppendInt((Char*)VariadicArg(va, Char*), min, pad);
				break;
			}
			case 'S': {
				res = AppendInt(((String*)VariadicArg(va, String*))->ToCString(), min, pad);
				break;
			}
			case 'c': {
				/* We could later also let the char argument use the padding options... */
				res = Append((Char)VariadicArg(va, IntPtr));
				break;
			}
			case 'd': {
				res = Append((IntPtr)VariadicArg(va, IntPtr), 10, min, pad);
				break;
			}
			case 'u': {
				res = Append((UIntPtr)VariadicArg(va, UIntPtr), 10, min, pad);
				break;
			}
			case 'b': {
				res = Append((UIntPtr)VariadicArg(va, UIntPtr), 2, min, pad);
				break;
			}
			case 'B': {
				UIntPtr left, right;
				Char unit;
				
				/* We need to call our helper, and manually write each part of the number. */
				
				ByteHelper::Convert((Float)VariadicArg(va, UInt64), unit, left, right);
				
				if ((res = Append(left, 10)) != Status::Success) {
					break;
				} else if (right > 0) {
					/* The right part is only written if it is bigger than 0. */
					
					if ((res = Append('.')) != Status::Success ||
						(res = Append(right, 10, 2, '0')) != Status::Success) {
						break;
					}
				}
				
				if (unit != 0) {
					/* And the unit is only written if it's not 'B' (which is represented by a literal
					 * 0. */
					
					res = Append(unit);
				}
				
				break;
			}
			case 'o': {
				res = Append((UIntPtr)VariadicArg(va, IntPtr), 8, min, pad);
				break;
			}
			case 'x': {
				res = Append((UIntPtr)VariadicArg(va, UIntPtr), 16, min, pad);
				break;
			}
			default: {
				res = Append((Char)Value[i]);
				break;
			}
			}
			
			if (res != Status::Success) {
				return res;
			}
		}
	}
	
	VariadicEnd(va);
	
	return Status::Success;
}

Boolean String::Compare(const String &Value) const {
	/* Both strings SHOULD have the same size if they are equal, and we can just StrCompareMemory the
	 * contents. */
	
	if (Value.Value == this->Value) {
		return True;
	} else if (Value.Value == Null || this->Value == Null) {
		return False;
	}
	
	UIntPtr len = 0;
	
	for (; Value[len]; len++) ;
	
	return len == Length && StrCompareMemory(this->Value, Value.ToCString(), len);
}

static Boolean IsDelimiter(const String &Delimiters, Char Value) {
	for (Char ch : Delimiters) {
		if (ch == Value) {
			return True;
		}
	}
	
	return False;
}

List<String> String::Tokenize(const String &Delimiters) const {
	/* Start by checking if the delimiters string have at least one delimiter, creating the output list,
	 * and creating one "global" string pointer, we're going to initialize it to Null, and only alloc it
	 * if we need. */
	
	if (Value == Null || Delimiters.Length == 0) {
		return List<String>();
	}
	
	String str;
	List<String> ret;
	
	for (UIntPtr i = 0; i < Length; i++) {
		if (IsDelimiter(Delimiters, Value[i])) {
			/* If this is one of the delimiters, we can add the string we have been creating to the list.
			 * If the string is still empty, we can just skip and go to the next character. */
			
			if (str.GetLength() == 0) {
				continue;
			} else if (ret.Add(str) != Status::Success) {
				return List<String>();
			}
			
			str.Clear();
			
			continue;
		}
		
		if (str.Append(Value[i]) != Status::Success) {
			return List<String>();
		}
	}
	
	return (str.GetLength() == 0 || ret.Add(str) == Status::Success) ? ret : List<String>();
}

Char *String::ToMutCString(Void) const {
	Char *ret = new Char[Length + 1];
	
	if (ret != Null) {
		StrCopyMemory(ret, Value, Length + 1);
	}
	
	return ret;
}

Status String::AppendInt(const Char *Value, UIntPtr MinLength, Char PadChar) {
	/* This function does the magic of handling the padding of a string (and remember that everything
	 * except characters will become a string and pass through this function), so, let's start by
	 * filling what we need on the left side (before the string).
	 * Before starting to do the padding, we need to get the length of the string. */
	
	if (Value == Null) {
		Value = "";
	}
	
	UIntPtr len = 0;
	Status ret = Status::Success;
	
	for (; Value[len]; len++) ;
	
	/* Now, in the case the value length is smaller than the minimum length, we need to fill the
	 * left side with the PadChar, until the length that we filled + the length of the string will
	 * be the minimum length. */
	
	for (; len < MinLength; len++) {
		if ((ret = Append(PadChar)) != Status::Success) {
			return ret;
		}
	}
	
	/* And finally, we can just write the string itself. */
	
	for (; *Value; Value++) {
		if ((ret = Append(*Value)) != Status::Success) {
			return ret;
		}
	}
	
	return ret;
}

/* Next are all the memory functions, we could make a better and optimized implementation, but, for
 * now, let's just hope that the compiler will optimize it for us. */

Void StrCopyMemory(Void *Buffer, const Void *Source, UIntPtr Length) {
	if (Buffer == Null || Source == Null || Length == 0 || Buffer == Source) {
		return;
	}
	
	for (UInt8 *dst = (UInt8*)Buffer, *src = (UInt8*)Source; Length--;) {
		*dst++ = *src++;
	}
}

Void StrCopyMemory32(Void *Buffer, const Void *Source, UIntPtr Length) {
	if (Buffer == Null || Source == Null || Length == 0 || Buffer == Source) {
		return;
	}
	
	for (UInt32 *dst = (UInt32*)Buffer, *src = (UInt32*)Source; Length--;) {
		*dst++ = *src++;
	}
}

Void StrSetMemory(Void *Buffer, UInt8 Value, UIntPtr Length) {
	if (Buffer == Null || Length == 0) { 
		return;
	}
	
	for (UInt8 *buf = (UInt8*)Buffer; Length--;) {
		*buf++ = Value;
	}
}

Void StrSetMemory32(Void *Buffer, UInt32 Value, UIntPtr Length) {
	if (Buffer == Null || Length == 0) { 
		return;
	}
	
	for (UInt32 *buf = (UInt32*)Buffer; Length; Length--) {
		*buf++ = Value;
	}
}

Void StrMoveMemory(Void *Buffer, const Void *Source, UIntPtr Length) {
	/* While copy doesn't handle intersecting regions, move should handle them, there are two ways of
	 * doing that: Allocating a temp buffer, copying the source data into it and copying the data from
	 * the temp buffer into the destination, or checking if the source buffer overlaps with the destination
	 * when copying forward, and, if that's the case, copy backwards. We're going with the second way, as
	 * it's (probably) a good idea to make this work even without the memory allocator. */
	
	if (Buffer == Null || Source == Null || Length == 0 || Buffer == Source) {
		return;
	} else if ((UIntPtr)Buffer > (UIntPtr)Source &&
			   (UIntPtr)Source + Length >= (UIntPtr)Buffer) {
		for (UInt8 *dst = &((UInt8*)Buffer)[Length - 1],
				   *src = &((UInt8*)Source)[Length - 1]; Length--;) {
			*dst-- = *src--;
		}
	} else {
		StrCopyMemory(Buffer, Source, Length);
	}
}

Boolean StrCompareMemory(const Void *const Left, const Void *const Right, UIntPtr Length) {
	if (Left == Null || Right == Null || Length == 0) {
		return False;
	} else if (Left == Right) {
		return False;
	}
	
	for (UInt8 *m1 = (UInt8*)Left, *m2 = (UInt8*)Right; Length--;) {
		if (*m1++ != *m2++) {
			return False;
		}
	}
	
	return True;
}
