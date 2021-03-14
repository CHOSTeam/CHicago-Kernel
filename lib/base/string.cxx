/* File author is Ítalo Lima Marconato Matias
 *
 * Created on March 05 of 2021, at 10:21 BRT
 * Last edited on March 14 of 2021 at 11:15 BRT */

#include <base/string.hxx>

using namespace CHicago;

String::String(UIntPtr Length) : String() {
    /* We only need to alloc memory if the length is higher than 16 (as anything less or equal to that we can store in
     * the String class itself). */

    if (Length > 16) {
        if ((Value = new Char[Length + 1]) != Null) {
            Capacity = Length + 1;
        }
    }
}

#define CONSTRUCT(l, val) \
    do { UIntPtr len = l; \
         \
         if (len > 16 && (this->Value = new Char[len + 1]) != Null) { \
             Capacity = len + 1; \
             Length = ViewEnd = len; \
             CopyMemory(this->Value, val, len); \
         } else if (len <= 16) { \
             Length = ViewEnd = len; \
             CopyMemory(Small, val, len + 1); \
         } } while (False)

String::String(const Char *Value) : String() {
    CONSTRUCT(StringView::CalculateLength(Value), Value);
}

String::String(const String &Source) : String() {
    CONSTRUCT(Source.ViewEnd - Source.ViewStart, Source.Value + Source.ViewStart);
}

String::String(String &&Source)
    : Small {}, Capacity { Exchange(Source.Capacity, 0) }, Length { Exchange(Source.Length, 0) },
      ViewStart { Exchange(Source.ViewStart, 0) }, ViewEnd { Exchange(Source.ViewEnd, 0) } {
    if (Capacity) {
        Value = Exchange(Source.Value, Null);
    } else {
        CopyMemory(Small, Source.Small, Source.Length + 1);
    }
}

String::String(const StringView &Source) : String() {
    CONSTRUCT(Source.ViewEnd - Source.ViewStart, Source.Value + Source.ViewStart);
}

String::String(ReverseIterator<Char, Char*> Source)
        : String(ConstReverseIterator<Char, const Char*>(Source.end().GetIterator() + 1,
                                                         Source.begin().GetIterator() + 1)) { }

String::String(const ConstReverseIterator<Char, const Char*> &Source) : String() {
    /* .begin() and .end() are inverted (that is, to get the actual length we need to do begin() - end() instead of
     * end() - begin()). We just need to get the length, try allocating the required space, and then copying. */

    UIntPtr len = Source.begin().GetIterator() - Source.end().GetIterator(), i = 0;

    if (len > 16 && (Value = new Char[len + 1]) != Null) {
        for (Char ch : Source) {
            Value[i++] = ch;
        }

        Value[i] = 0;
        Capacity = len + 1;
        Length = ViewEnd = len;
    } else if (len <= 16) {
        for (Char ch : Source) {
            Small[i++] = ch;
        }

        Small[i] = 0;
        Length = ViewEnd = len;
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

String &String::operator =(String &&Source) {
    /* This is just the move constructor, but we can't use ': field1(field1), field2(field2), etc', instead, we have to
     * manually do it. */

    if (this != &Source) {
        Clear();

        if (Source.Capacity) {
            Value = Exchange(Source.Value, Null);
        } else {
            CopyMemory(Small, Source.Small, Source.Length + 1);
        }

        Capacity = Exchange(Source.Capacity, 0);
        Length = Exchange(Source.Length, 0);
        ViewStart = Exchange(Source.ViewStart, 0);
        ViewEnd = Exchange(Source.ViewEnd, 0);
    }

    return *this;
}

String &String::operator =(const Char *Value) {
    /* And this is the copy constructor, but again, we need to manually do everything. */

    Clear();
    CONSTRUCT(StringView::CalculateLength(Value), Value);

    return *this;
}

String &String::operator =(const String &Source) {
    if (this != &Source) {
        Clear();
        CONSTRUCT(Source.ViewEnd - Source.ViewStart, Source.Value + Source.ViewStart);
    }

    return *this;
}

String &String::operator =(const StringView &Source) {
    Clear();
    CONSTRUCT(Source.ViewEnd - Source.ViewStart, Source.Value + Source.ViewStart);
    return *this;
}

Void String::Clear() {
    /* Clearing is just deallocating (if required) and setting all the fields/variables to 0. */

    if (Capacity) {
        delete[] Value;
    }

    Value = Null;
    Capacity = Length = ViewStart = ViewEnd = 0;
}

Void String::SetView(UIntPtr Start, UIntPtr End) {
    if (Start > End) {
        return;
    } else if (Start <= Length) {
        ViewStart = Start;
    }

    if (End <= Length) {
        ViewEnd = End;
    }
}

Status String::Append(Char Value) {
    /* First, if our allocated buffer wasn't actually allocated, we gonna need to allocate it, if it is too small, we
     * need to resize it. */

    if (!Capacity && Length + 1 < 16) {
        /* Special case: No capacity, but we still fit in the small buffer (That shares space with .Value). */

        Small[Length++] = Value;
        Small[Length] = 0;
    } else if (!Capacity || Length + 1 >= Capacity) {
        UIntPtr nlen = Length < 4 ? 4 : Length * 2 + 1;
        Char *buf = new Char[nlen];

        if (buf == Null) {
            return Status::OutOfMemory;
        } else if (Length) {
            CopyMemory(buf, Capacity ? this->Value : Small, Length);
        }

        if (Capacity) {
            delete[] this->Value;
        }

        this->Value = buf;
        Capacity = nlen;
    }

    /* Now, just add the character to the end of the array, and put the string end character. Also, appending resets the
     * view start/end. */

    if (Capacity) {
        this->Value[Length++] = Value;
        this->Value[Length] = 0;
    }

    ViewStart = 0;
    ViewEnd = Length;

    return Status::Success;
}

Char *String::GetValue() const {
    UIntPtr len = ViewEnd - ViewStart;
    Char *str = len ? new Char[len + 1] : Null;

    if (str != Null) {
        CopyMemory(str, (Capacity ? Value : Small) + ViewStart, len + 1);
    }

    return str;
}
