/* File author is Ítalo Lima Marconato Matias
 *
 * Created on March 05 of 2021, at 16:12 BRT
 * Last edited on March 06 of 2021 at 21:17 BRT */

#include <base/string.hxx>

using namespace CHicago;

StringView::StringView(const String &Source)
        : Value(Source.Capacity ? Source.Small : Source.Value), Length(Source.Length), ViewStart(Source.ViewStart),
          ViewEnd(Source.ViewEnd) { }

StringView StringView::FromStatus(Status Code) {
    /* We should probably try to make something better than this... */

    switch (Code) {
        case Status::Success: return "Success";
        case Status::InvalidArg: return "Invalid Argument";
        case Status::Unsupported: return "Unsupported";
        case Status::OutOfMemory: return "Out Of Memory";
        case Status::NotMapped: return "Not Mapped";
        case Status::AlreadyMapped: return "Already Mapped";
        case Status::InvalidFs: return "Invalid FileSystem";
        case Status::NotMounted: return "Not Mounted";
        case Status::AlreadyMounted: return "Already Mounted";
        case Status::DoesntExist: return "Doesn't Exist";
        case Status::AlreadyExists: return "Already Exist";
        case Status::NotFile: return "Not a File";
        case Status::NotDirectory: return "Not a Directory";
        case Status::NotRead: return "Not Readable";
        case Status::NotWrite: return "Not Writeable";
        case Status::NotExec: return "Not Executable";
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

StringView StringView::FromInt(Char *Buffer, Int64 Value, UIntPtr Size) {
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

StringView StringView::FromUInt(Char *Buffer, UInt64 Value, UIntPtr Size, UInt8 Base) {
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

StringView StringView::FromFloat(Char *Buffer, Float Value, UIntPtr Size, UIntPtr Precision) {
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

Void StringView::SetView(UIntPtr Start, UIntPtr End) {
    if (Start > End) {
        return;
    } else if (Start <= Length) {
        ViewStart = Start;
    }

    if (End <= Length) {
        ViewEnd = End;
    }
}

static inline Boolean IsDigit(Char Value) { return Value >= '0' && Value <= '9'; }
static inline Boolean IsHex(Char Value) { return IsDigit(Value) || (Value >= 'a' && Value <= 'f')
                                                 || (Value >= 'A' && Value <= 'F'); }

static inline UInt64 ToHexUInt(const Char *Value, UIntPtr Length, UIntPtr &Position) {
    UInt64 ret = 0;

    for (; Position < Length && IsHex(Value[Position]); Position++) {
        ret = (ret * 16) + (IsDigit(Value[Position]) ? Value[Position] - '0' :
                            ((Value[Position] >= 'a' && Value[Position] <= 'f' ? Value[Position] - 'a' :
                              Value[Position] - 'A') + 10));
    }

    return ret;
}

Int64 StringView::ToInt(UIntPtr &Position) const {
    /* ToInt doesn't need to handle different bases (only base 10), so we can just parse everything while we encounter
     * characters from '0' to '9'. */

    if (Position >= Length) {
        return 0;
    }

    UInt64 ret;
    Boolean neg = False;

    if (Position < Length && Value[Position] == '-') {
        Position++;
        neg = True;
    }

    return ret = ToUInt(Position, True), neg ? -ret : ret;
}

static UInt64 Pow(UIntPtr X, UIntPtr Y) {
    UInt64 i = 1;

    while (Y--) {
        i *= X;
    }

    return i;
}

Float StringView::ToFloat(UIntPtr &Position) const {
    /* And at last we have ToFloat(), the first part is the same as ToInt, and if we don't find a dot somewhere, we ARE
     * the same as ToInt, but once we enter the actual float/double world, we gonna store the precision (as we add more
     * digits), and at the end divide by 10^prec. */

    if (Position >= Length) {
        return 0;
    }

    Float ret;
    Boolean neg = False, nege = False;
    UInt64 main, dec = 0, exp = 0, prec = 1, base = 10;

    if (Position < Length && Value[Position] == '-') {
        Position++;
        neg = True;
    }

    if (Position + 1 < Length && Value[Position] == '0' && Value[Position + 1] == 'x') {
        /* Base 16/hex float, for the main and dec values we use hexadecimal (handle them in the same way that we do
         * any hex int in ToUInt). */

        base = 2;
        main = ToUInt(Position);

        if (Position < Length && Value[Position] == '.') {
            /* Just as we're going to do in the base 10 case, manually handle the dec part. */

            for (Position++; Position < Length && IsHex(Value[Position]); Position++, prec *= 16) {
                dec = (dec * 16) + (IsDigit(Value[Position]) ? Value[Position] - '0' :
                                    ((Value[Position] >= 'a' && Value[Position] <= 'f' ? Value[Position] - 'a' :
                                      Value[Position] - 'A') + 10));
            }
        }
    } else {
        main = ToUInt(Position);

        if (Position < Length && Value[Position] == '.') {
            /* Manually handle the dec part, as we also have to increase the precision at each step. */

            for (Position++; Position < Length && IsDigit(Value[Position]); Position++, prec *= 10) {
                dec = (dec * 10) + (Value[Position] - '0');
            }
        }
    }

    /* And for exponents, we expect pZ/p-Z or eZ/e-Z. eZ/e-Z is for normal base 10 floats (Z is a power of 10), pZ/p-Z
     * is for hex floats (Z is a power of 2). */

    if (Position < Length && ((base == 2 && (Value[Position] == 'p' || Value[Position] == 'P')) ||
                              (base == 10 && (Value[Position] == 'e' || Value[Position] == 'E')))) {
        if (Position + 1 < Length && Value[Position + 1] == '-') {
            Position++;
            nege = True;
        }

        Position++;
        exp = ToUInt(Position, True);
    }

    return ret = main + (dec ? static_cast<Float>(dec) / prec : 0),
            ret = exp ? (nege ? ret / Pow(base, exp) : ret * Pow(base, exp)) : ret, neg ? -ret : ret;
}

UInt64 StringView::ToUInt(UIntPtr &Position, Boolean OnlyDec) const {
    /* ToUInt does need to handle other bases, and for that we take the first two characters, for different bases, they
     * should always be 0<n> where <n> is 'b' for binary, 'o' for octal and 'x' for hexadecimal. */

    if (Position >= Length) {
        return 0;
    }

    UInt64 ret = 0;
    Boolean canb = !OnlyDec && Position + 1 < Length && Value[Position] == '0';

    if (canb && Value[Position + 1] == 'b') {
        for (Position += 2; Position < Length && (Value[Position] == '0' || Value[Position] == '1'); Position++) {
            ret = (ret * 2) + (Value[Position] - '0');
        }
    } else if (canb && Value[Position + 1] == 'o') {
        for (Position += 2; Position < Length && Value[Position] >= '0' && Value[Position] <= '7'; Position++) {
            ret = (ret * 8) + (Value[Position] - '0');
        }
    } else if (canb && Value[Position + 1] == 'x') {
        Position += 2;
        ret = ToHexUInt(Value, Length, Position);
    } else {
        for (; Position < Length && IsDigit(Value[Position]); Position++) {
            ret = (ret * 10) + (Value[Position] - '0');
        }
    }

    return ret;
}

Boolean StringView::Compare(const StringView &Value) const {
    /* Very basic compare function, it just returns if both strings are equal (same length and contents). */

    if (this->Value == Value.Value) {
        return True;
    } else if (this->Value == Null || Value.Value == Null || Length != Value.Length) {
        return False;
    }

    return CompareMemory(this->Value, Value.Value, Length);
}

Boolean StringView::StartsWith(const StringView &Value) const {
    /* This is like the Compare function, but we want to limit the length to the Value's length/active view (so our
     * length/active view only needs to be at least the same as the Value's one, not exactly the same). */

    if (this->Value == Value.Value) {
        return True;
    } else if (this->Value == Null || Value.Value == Null || Length < Value.Length) {
        return False;
    }

    return CompareMemory(this->Value, Value.Value, Value.Length);
}

static Boolean IsDelimiter(const StringView &Delimiters, Char Value) {
    for (Char ch : Delimiters) {
        if (ch == Value) {
            return True;
        }
    }

    return False;
}

List<String> StringView::Tokenize(const StringView &Delimiters) const {
    /* Start by checking if the delimiters string have at least one delimiter, creating the output list, and creating
     * one "global" string pointer, we're going to initialize it to Null, and only alloc it if we need. */

    if (Value == Null || !Delimiters.Length) {
        return {};
    }

    String str;
    List<String> ret;

    for (UIntPtr i = 0; i < Length; i++) {
        if (IsDelimiter(Delimiters, Value[i])) {
            /* If this is one of the delimiters, we can add the string we have been creating to the list. If the string
             * is still empty, we can just skip and go to the next character. */

            if (!str.Length) {
                continue;
            } else if (ret.Add(str) != Status::Success) {
                return {};
            }

            str.Clear();

            continue;
        }

        if (str.Append(Value[i]) != Status::Success) {
            return {};
        }
    }

    if (str.Length && ret.Add(str) != Status::Success) {
        return {};
    }

    return ret;
}
