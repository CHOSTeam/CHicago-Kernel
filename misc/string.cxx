/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 07 of 2021, at 14:08 BRT
 * Last edited on February 16 of 2021 at 20:01 BRT */

#include <string.hxx>

using namespace CHicago;

String::String(Void) : Value(Null), Capacity(0), Length(0) { }

String::String(UIntPtr Size) : Value(Null), Capacity(0), Length(0) {
    Value = Size != 0 ? new Char[Size + 1] : Null;

    if (Value != Null) {
        Capacity = Size + 1;
    }
}

String::String(const String &Value) : Value(Value.Value), Capacity(0), Length(Value.Length) {
    /* If the capacity is higher than 0, it means that this is not some static string on a fixed kernel location, but
     * something that we allocated (and as such, we need to append/clone it). */

    if (Value.Capacity > 0) {
        Char *str = new Char[Length + 1];

        if (str != Null) {
            CopyMemory(str, Value.Value, Length);
            Capacity = Length + 1;
        } else {
            Length = 0;
        }

        this->Value = str;
    }
}

Void String::CalculateLength(Void) {
    for (; Value != Null && Value[Length]; Length++) ;
}

String::String(const Char *Value, Boolean Alloc) : Value(const_cast<Char*>(Value)), Capacity(0), Length(0) {
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

String::~String(Void) {
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
        this->Capacity = this->Length = 0;
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
            this->Value = Value.Value;
            this->Length = Value.Length;
        }
    }

    return *this;
}

Void String::FromUInt(Char *Buffer, UInt64 Value, UInt8 Base, IntPtr &Current, IntPtr End) {
    for (; Current >= End && Value; Current--, Value /= Base) {
        Buffer[Current] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[Value % Base];
    }
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
    IntPtr cur = static_cast<IntPtr>(Size - 2), end = Value < 0 ? 1 : 0;

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

    auto cur = static_cast<IntPtr>(Size - 2);
    FromUInt(Buffer, Value, Base, cur, 0);

    return &Buffer[cur + 1];
}

Void String::Clear(Void) {
    /* And clearing is just deallocating (if required) and setting the Length and the Capacity variables to 0. */

    if (Capacity && Value != Null) {
        delete[] Value;
    }

    Value = Null;
    Capacity = Length = 0;
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

    /* Now, just add the character to the end of the array, and put the string end character. */

    this->Value[Length++] = Value;
    this->Value[Length] = 0;

    return Status::Success;
}

Status String::Append(Int64 Value, UInt8 Base) {
    Char buf[65];
    return Append(FromInt(buf, Value, 65, Base));
}

Status String::Append(UInt64 Value, UInt8 Base) {
    Char buf[65];
    return Append(FromUInt(buf, Value, 65, Base));
}

Status String::Append(const String &Format, ...) {
    /* Use VariadicFormat (like all the other functions), while passing ourselves as the context. */

    Status status = Status::Success;
    UIntPtr ctx[2] = { reinterpret_cast<UIntPtr>(&status), reinterpret_cast<UIntPtr>(this) };

    VariadicList args;
    VariadicStart(args, Format);

    VariadicFormat(Format, args, [](Char Data, Void *Context) -> Boolean {
        auto ctx = reinterpret_cast<UIntPtr*>(Context);
        return (*reinterpret_cast<Status*>(ctx[0]) =
                 reinterpret_cast<String*>(ctx[1])->Append(Data)) == Status::Success;
    }, ctx, Null, 0, 0);

    VariadicEnd(args);
    return status;
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

Char *String::GetMutValue(Void) const {
    Char *str = Length > 0 ? new Char[Length + 1] : Null;

    if (str != Null) {
        CopyMemory(str, Value, Length);
    }

    return str;
}
