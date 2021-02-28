/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 07 of 2021, at 14:08 BRT
 * Last edited on February 28 of 2021 at 13:42 BRT */

#include <string.hxx>

using namespace CHicago;

String::String() : Value(const_cast<Char*>("")), Capacity(0), Length(0), ViewStart(0), ViewEnd(0) { }

String::String(UIntPtr Size) : Value(Null), Capacity(0), Length(0), ViewStart(0), ViewEnd(0) {
    Value = Size != 0 ? new Char[Size + 1] : Null;

    if (Value != Null) {
        Capacity = Size + 1;
    } else {
        Value = const_cast<Char*>("");
    }
}

String::String(String &&Value)
        : Value(Value.Value), Capacity(Value.Capacity), Length(Value.Length), ViewStart(Value.ViewStart),
          ViewEnd(Value.ViewEnd) {
    Value.Value = Null;
    Value.Capacity = Value.Length = Value.ViewStart = Value.ViewEnd = 0;
}

String::String(const String &Value)
    : Value(Value.Value + Value.ViewStart), Capacity(0), Length(Value.ViewEnd - Value.ViewStart), ViewStart(0),
      ViewEnd(Value.ViewEnd - Value.ViewStart) {
    /* If the capacity is higher than 0, it means that this is not some static string on a fixed kernel location, but
     * something that we allocated (and as such, we need to append/clone it). */

    if (Value.Capacity > 0) {
        Char *str = new Char[Length + 1];

        if (str != Null) {
            CopyMemory(str, this->Value, Length);
            Capacity = Length + 1;
        } else {
            Length = 0;
        }

        this->Value = str;
    }
}

Void String::CalculateLength() {
    for (; Value != Null && Value[Length]; Length++) ;
    ViewEnd = Length;
}

String::String(const Char *Value, Boolean Alloc)
    : Value(const_cast<Char*>(Value)), Capacity(0), Length(0), ViewStart(0), ViewEnd(0) {
    /* Precalculate the length, so that the we can just use a .GetLength(), instead of calculating it every time. Also,
     * if the user said that want to alloc/copy the string, alloc enough space for that, and then copy it. */

    CalculateLength();

    if (Alloc && Length) {
        Char *str = new Char[Length + 1];

        if (str != Null) {
            CopyMemory(str, Value, Length);
            Capacity = Length + 1;
            this->Value = str;
        }
    }
}

String::~String() {
    /* If the allocated size (stored in the variable named Capacity) is greater than 0, we need to delete[] the array.
     * Also remember to set the allocated size to 0, as we may be called multiple times accidentally (probably not going
     * to happen, but let's do it just to be sure). */

    if (Capacity) {
        delete[] Value;
        Capacity = 0;
    }
}

String &String::operator =(const Char *Value) {
    /* Just set the new string value (if required) and recalculate the length. */

    if (this->Value != Value) {
        Clear();
        this->Value = const_cast<Char*>(Value);
        this->Capacity = this->Length = this->ViewStart = 0;
        CalculateLength();
    }

    return *this;
}

String &String::operator =(const String &Value) {
    /* Same as above, but fortunately the length is already calculated. */

    if (this != &Value) {
        Clear();

        if (Value.Capacity) {
            Append(Value);
        } else {
            this->Value = Value.Value + Value.ViewStart;
            ViewStart = 0;
            Length = ViewEnd = Value.ViewEnd - Value.ViewStart;
        }
    }

    return *this;
}

String String::FromStatus(Status Code) {
    /* We should probably try to make something better than this... */

    switch (Code) {
    case Status::Success: return "Success";
    case Status::AlreadyMapped: return "Already Mapped";
    case Status::InvalidArg: return "Invalid Argument";
    case Status::NotMapped: return "Not Mapped";
    case Status::OutOfMemory: return "Out Of Memory";
    default: return "Invalid Status Code";
    }
}

static UIntPtr CountDigits(UInt64 Value, UInt8 Base) {
    if (!Value) {
        return 1;
    }

    UIntPtr ret = 0;
    for (; Value; Value /= Base, ret++) ;

    return ret;
}

static Void FromUInt(Char *Buffer, UInt64 Value, UInt8 Base, IntPtr &Current, IntPtr End) {
    for (; Current >= End && Value; Current--, Value /= Base) {
        Buffer[Current] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[Value % Base];
    }
}

String String::FromInt(Char *Buffer, Int64 Value, UIntPtr Size) {
    /* Extract the sign (we gonna plot it at the end), and do a basic buffer size checks. */

    Int64 sign = Value < 0 ? -1 : 1;
    Value *= sign;

    if (Buffer == Null || (Value && Size < CountDigits(Value, 10) + (sign < 0 ? 1 : 0) + 1)) {
        return {};
    } else if (!Value) {
        return "0";
    }

    /* Now with the easy cases out of the way, let's first handle saving the sign, and converting the value into a
     * positive value. */

    IntPtr cur = static_cast<IntPtr>(Size - 2), end = sign < 0 ? 1 : 0;

    /* Now we can just use our global FromUInt function (as the value is now a valid UInt), add the sign (if required),
     * and return! */

    ::FromUInt(Buffer, Value, 10, cur, end);

    if (sign) {
        Buffer[cur--] = '-';
    }

    return &Buffer[cur + 1];
}

String String::FromUInt(Char *Buffer, UInt64 Value, UIntPtr Size, UInt8 Base) {
    if (Buffer == Null || Base < 2 || Base > 36 || (Value && Size < CountDigits(Value, Base) + 1)) {
        return {};
    } else if (!Value) {
        return "0";
    }

    /* As the UInt value is always positive, we can just call the int function and return (no need to handle the val
     * sign). */

    auto cur = static_cast<IntPtr>(Size - 2);
    ::FromUInt(Buffer, Value, Base, cur, 0);

    return &Buffer[cur + 1];
}

static Float Pow10Neg[] = {
        0.5, 0.05, 0.005, 0.0005, 0.00005, 0.000005, 0.0000005, 0.00000005, 0.000000005, 0.0000000005, 0.00000000005,
        0.000000000005, 0.0000000000005, 0.00000000000005, 0.000000000000005, 0.0000000000000005, 0.00000000000000005,
        0.000000000000000005
};

static Boolean IsInfinite(Float Value) {
    /* According to IEEE-754, we know that a number is infinite if all the exponent bits are 1, and all the fraction
     * bits are 0, and then we can just check the sign bit to know if it is -inf or +inf. */

    union { Float FloatValue; UInt64 IntValue; } val { .FloatValue = Value };

    return !(val.IntValue & 0xFFFFFFFFFFFFF) && (val.IntValue & 0x7FF0000000000000) == 0x7FF0000000000000;
}

static Boolean IsNaN(Float Value) {
    /* And for checking NaNs, it's similar, but the fraction has to be anything except all zeroes. */

    union { Float FloatValue; UInt64 IntValue; } val { .FloatValue = Value };

    return (val.IntValue & 0xFFFFFFFFFFFFF) && (val.IntValue & 0x7FF0000000000000) == 0x7FF0000000000000;
}

String String::FromFloat(Char *Buffer, Float Value, UIntPtr Size, UIntPtr Precision) {
    /* First, check for infinite and nan (as we have to handle those in a different way), and after that, limit the max
     * precision to 17 (trying to print more than this is a bit useless/starts to lose too much accuracy, and also we
     * use a precalculated table for pow10). */

    if (IsInfinite(Value)) {
        return Value < 0 ? "-Infinite" : "Infinite";
    } else if (IsNaN(Value)) {
        return "NaN";
    } else if (Precision > 17) {
        Precision = 17;
    }

    /* Extract the sign, and do the basic buffer size checks. */

    Int64 sign = Value < 0 ? -1 : 1;
    Value *= sign;

    if (Buffer == Null || Size < CountDigits(Value, 10) + (sign < 1 ? 1 : 0) + (Precision ? Precision + 1 : 0) + 1) {
        return {};
    }

    /* Add pow10(-prec), to make sure we will, for example, print 1 instead of 0.999999... */

    Value += Pow10Neg[Precision];

    /* Start by printing the fractional part into the buffer. */

    IntPtr cur = static_cast<IntPtr>(Size - (Precision ? Precision + 1 : 0) - 2), start = cur - 1,
           end = sign < 0 ? 1 : 0;

    if (Precision) {
        /* Add pow10(-prec), to make sure we will print 1 instead of 0.999999... (for example). */

        Float tmp = Value - static_cast<Int64>(Value);

        Buffer[cur++] = '.';

        while (Precision--) {
            auto dig = static_cast<Int64>(tmp *= 10);
            Buffer[cur++] = dig + '0';
            tmp -= dig;
        }
    }

    /* And then the integer/whole part of the float into the buffer (this uses the same process as FromInt). */

    if (!static_cast<UInt64>(Value)) {
        Buffer[start--] = '0';
    } else {
        ::FromUInt(Buffer, Value, 10, start, end);
    }

    if (sign < 0) {
        Buffer[start--] = '-';
    }

    return &Buffer[start + 1];
}

Void String::Clear() {
    /* And clearing is just deallocating (if required) and setting the Length and the Capacity variables to 0. */

    if (Capacity && Value != Null) {
        delete[] Value;
    }

    Value = Null;
    Capacity = Length = ViewStart = ViewEnd = 0;
}

Void String::SetView(UIntPtr Start, UIntPtr End) {
    /* We just need to make sure that the start and end are valid, and that the end is higher than the start. */

    if (Start > Length || End > Length || Start > End) {
        return;
    }

    ViewStart = Start;
    ViewEnd = End;
}

static inline Boolean IsDigit(Char Value) { return Value >= '0' && Value <= '9'; }
static inline Boolean IsHex(Char Value) { return IsDigit(Value) || (Value >= 'a' && Value <= 'f')
                                                                || (Value >= 'A' && Value <= 'F'); }

static inline UInt64 ToHexUInt(const Char *Value, UIntPtr ViewEnd, UIntPtr &Position) {
    UInt64 ret = 0;

    for (; Position < ViewEnd && IsHex(Value[Position]); Position++) {
        ret = (ret * 16) + (IsDigit(Value[Position]) ? Value[Position] - '0' :
                            ((Value[Position] >= 'a' && Value[Position] <= 'f' ? Value[Position] - 'a' :
                              Value[Position] - 'A') + 10));
    }

    return ret;
}

Int64 String::ToInt(UIntPtr &Position) const {
    /* ToInt doesn't need to handle different bases (only base 10), so we can just parse everything while we encounter
     * characters from '0' to '9'. */

    UInt64 ret;
    Boolean neg = False;

    if (Position < ViewEnd && Value[Position] == '-') {
        Position++;
        neg = True;
    }

    return ret = ToUInt(Position, True), neg ? -ret : ret;
}

UInt64 String::ToUInt(UIntPtr &Position, Boolean OnlyDec) const {
    /* ToUInt does need to handle other bases, and for that we take the first two characters, for different bases, they
     * should always be 0<n> where <n> is 'b' for binary, 'o' for octal and 'x' for hexadecimal. */

    UInt64 ret = 0;
    Boolean canb = !OnlyDec && Position + 1 < ViewEnd && Value[Position] == '0';

    if (canb && Value[Position + 1] == 'b') {
        for (Position += 2; Position < ViewEnd && (Value[Position] == '0' || Value[Position] == '1'); Position++) {
            ret = (ret * 2) + (Value[Position] - '0');
        }
    } else if (canb && Value[Position + 1] == 'o') {
        for (Position += 2; Position < ViewEnd && Value[Position] >= '0' && Value[Position] <= '7'; Position++) {
            ret = (ret * 8) + (Value[Position] - '0');
        }
    } else if (canb && Value[Position + 1] == 'x') {
        Position += 2;
        ret = ToHexUInt(Value, ViewEnd, Position);
    } else {
        for (; Position < ViewEnd && IsDigit(Value[Position]); Position++) {
            ret = (ret * 10) + (Value[Position] - '0');
        }
    }

    return ret;
}

static UInt64 Pow(UIntPtr X, UIntPtr Y) {
    UInt64 i = 1;

    while (Y--) {
        i *= X;
    }

    return i;
}

Float String::ToFloat(UIntPtr &Position) const {
    /* And at last we have ToFloat(), the first part is the same as ToInt, and if we don't find a dot somewhere, we ARE
     * the same as ToInt, but once we enter the actual float/double world, we gonna store the precision (as we add more
     * digits), and at the end divide by 10^prec. */

    if (Position < ViewStart || Position >= ViewEnd) {
        return 0;
    }

    Float ret;
    Boolean neg = False, nege = False;
    UInt64 main, dec = 0, exp = 0, prec = 1, base = 10;

    if (Position < ViewEnd && Value[Position] == '-') {
        Position++;
        neg = True;
    }

    if (Position + 1 < ViewEnd && Value[Position] == '0' && Value[Position + 1] == 'x') {
        /* Base 16/hex float, for the main and dec values we use hexadecimal (handle them in the same way that we do
         * any hex int in ToUInt). */

        base = 2;
        main = ToUInt(Position);

        if (Position < ViewEnd && Value[Position] == '.') {
            /* Just as we're going to do in the base 10 case, manually handle the dec part. */

            for (Position++; Position < ViewEnd && IsHex(Value[Position]); Position++, prec *= 16) {
                dec = (dec * 16) + (IsDigit(Value[Position]) ? Value[Position] - '0' :
                                    ((Value[Position] >= 'a' && Value[Position] <= 'f' ? Value[Position] - 'a' :
                                      Value[Position] - 'A') + 10));
            }
        }
    } else {
        main = ToUInt(Position);

        if (Position < ViewEnd && Value[Position] == '.') {
            /* Manually handle the dec part, as we also have to increase the precision at each step. */

            for (Position++; Position < ViewEnd && IsDigit(Value[Position]); Position++, prec *= 10) {
                dec = (dec * 10) + (Value[Position] - '0');
            }
        }
    }

    /* And for exponents, we expect pZ/p-Z or eZ/e-Z. eZ/e-Z is for normal base 10 floats (Z is a power of 10), pZ/p-Z
     * is for hex floats (Z is a power of 2). */

    if (Position < ViewEnd && ((base == 2 && (Value[Position] == 'p' || Value[Position] == 'P')) ||
                               (base == 10 && (Value[Position] == 'e' || Value[Position] == 'E')))) {
        if (Position + 1 < ViewEnd && Value[Position + 1] == '-') {
            Position++;
            nege = True;
        }

        Position++;
        exp = ToUInt(Position, True);
    }

    return ret = main + (dec ? static_cast<Float>(dec) / prec : 0),
           ret = exp ? (nege ? ret / Pow(base, exp) : ret * Pow(base, exp)) : ret, neg ? -ret : ret;
}

Status String::Append(Char Value) {
    /* First, if our allocated buffer wasn't actually allocated, we gonna need to allocate it, if it is too small, we
     * need to resize it. */

    if (!Capacity || Length + 1 >= Capacity) {
        UIntPtr nlen = Length < 4 ? 4 : Length * 2 + 1;
        Char *buf = new Char[nlen];

        if (buf == Null) {
            return Status::OutOfMemory;
        } else if (Length != 0) {
            CopyMemory(buf, this->Value, Length);
        }

        if (Capacity != 0) {
            delete[] this->Value;
        }

        this->Value = buf;
        Capacity = nlen;
    }

    /* Now, just add the character to the end of the array, and put the string end character. Also, appending resets the
     * view start/end. */

    this->Value[Length++] = Value;
    this->Value[Length] = 0;
    ViewStart = 0;
    ViewEnd = Length;

    return Status::Success;
}

UIntPtr String::Append(Int64 Value) {
    Char buf[65];
    return Append(FromInt(buf, Value, 65));
}

UIntPtr String::Append(UInt64 Value, UInt8 Base) {
    Char buf[65];
    return Append(FromUInt(buf, Value, 65, Base));
}

UIntPtr String::Append(Float Value, UIntPtr Precision) {
    Char buf[65];
    return Append(FromFloat(buf, Value, 65, Precision));
}

Boolean String::Compare(const String &Value) const {
    /* Very basic compare function, it just returns if both strings are equal (same length and contents). */

    if (this->Value == Value.Value) {
        return True;
    } else if (this->Value == Null || Value.Value == Null || ViewEnd - ViewStart != Value.ViewEnd - Value.ViewStart) {
        return False;
    }

    return CompareMemory(this->Value + ViewStart, Value.Value + Value.ViewStart, ViewEnd - ViewStart);
}

Boolean String::StartsWith(const String &Value) const {
    /* This is like the Compare function, but we want to limit the length to the Value's length/active view (so our
     * length/active view only needs to be at least the same as the Value's one, not exactly the same). */

    if (this->Value == Value.Value) {
        return True;
    } else if (this->Value == Null || Value.Value == Null || ViewEnd - ViewStart < Value.ViewEnd - Value.ViewStart) {
        return False;
    }

    return CompareMemory(this->Value + ViewStart, Value.Value + Value.ViewStart, Value.ViewEnd - Value.ViewStart);
}

Char *String::GetMutValue() const {
    UIntPtr len = ViewEnd - ViewStart;
    Char *str = len ? new Char[len + 1] : Null;

    if (str != Null) {
        CopyMemory(str, Value + ViewStart, len);
    }

    return str;
}
