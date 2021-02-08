/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 07 of 2021, at 14:08 BRT
 * Last edited on February 07 of 2021 at 17:47 BRT */

#include <string.hxx>

String::String() : Value(Null), Length(0) { }
String::String(const String &Value) : Value(Value.Value), Length(Value.Length) { }

String::String(const Char *Value) : Value(Value), Length(0) {
    /* Precalculate the length, so that the we can just use a .GetLength(), instead of calculating it every time. */

    CalculateLength();
}

String &String::operator =(const Char *Value) {
    /* Just set the new string value (if required) and recalculate the length. */

    if (this->Value != Value) {
        this->Value = Value;
        this->Length = 0;
        CalculateLength();
    }

    return *this;
}

String &String::operator =(const String &Value) {
    /* Same as above, but fortunately the length is already calculated. */

    if (this->Value != Value.Value) {
        this->Value = Value.Value;
        this->Length = Value.Length;
    }

    return *this;
}

String String::FromInt(Char *Buffer, Int64 Value, UIntPtr Size, UInt8 Base) {
    if (Buffer == Null || Size < 2 || Base < 2 || Base > 36) {
        return {};
    } else if (!Value) {
        return "0";
    }

    /* Now with the easy cases out of the way, let's first handle saving the sign, and converting the value into a
     * positive value. */

    Int64 sign = Value < 0 ? -1 : 1;
    IntPtr cur = Size - 2, end = Value < 0 ? 1 : 0;

    Value *= sign;

    /* Now we can just use our global FromUInt function (as the value is now a valid UInt), add the sign (if required),
     * and return! */

    FromUInt(Buffer, Value, Base, cur, end);

    if (sign == -1) {
        Buffer[cur--] = '-';
    }

    return &Buffer[cur + 1];
}

String String::FromUInt(Char *Buffer, UInt64 Value, UIntPtr Size, UInt8 Base) {
    if (Buffer == Null || Size < 2 || Base < 2 || Base > 36) {
        return {};
    } else if (!Value) {
        return "0";
    }

    /* As the UInt value is always positive, we can just call the int function and return (no need to handle the val
     * sign). */

    IntPtr cur = Size - 2;
    FromUInt(Buffer, Value, Base, cur, 0);

    return &Buffer[cur + 1];
}

Boolean String::Compare(const String &Value) const {
    /* Very basic compare function, it just returns if both strings are equal (same length and contents). */

    if (this->Value == Value.Value) {
        return True;
    } else if (this->Value == Null || Value.Value == Null || Length != Value.Length) {
        return False;
    }

    return CompareMemory(this->Value, Value.Value, Length);
}

Void String::CalculateLength(Void) {
    for (; Value[Length]; Length++) ;
}

Void String::FromUInt(Char *Buffer, UInt64 Value, UInt8 Base, IntPtr &Current, IntPtr End) {
    for (; Current >= End && Value; Current--, Value /= Base) {
        Buffer[Current] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[Value % Base];
    }
}
