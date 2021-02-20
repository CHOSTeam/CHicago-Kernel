/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 07 of 2021, at 15:57 BRT
 * Last edited on February 20 of 2021 at 18:39 BRT */

#include <string.hxx>

using namespace CHicago;

/* Some macros to make our life a bit easier. */

#define WRITE_CHAR(c) \
    Buffer = WriteCharacter(c, Function, Context, Buffer, Size, Limit, written, err); \
    \
    if (err) { \
        return written; \
    } \
    \
    written++

#define WRITE_STRING(str, sz) \
    Buffer = WriteString(str, sz, Function, Context, Buffer, Size, Limit, written, err); \
    \
    if (err) { \
        return written; \
    } \
    \
    written += sz

#define PAD(cond, cnt, c) \
    do { if (cond) { \
             for (UIntPtr i = 0; i < cnt; i++) { \
                 WRITE_CHAR(c); \
             } \
         } } while (False)

#define WRITE_SIGN(cond) \
    do { if (cond) { \
             if (val < 0) { \
                 WRITE_CHAR('-'); \
             } else if (sign == 1) { \
                 WRITE_CHAR('+'); \
             } else if (sign == 2) { \
                 WRITE_CHAR(' '); \
             } \
         } } while (False)

namespace CHicago {

/* TODO: Move those functions before ParseFlags somewhere else (as we will probably need them not only here). */

static Boolean IsDigit(Char Value) {
    return Value >= '0' && Value <= '9';
}

static UIntPtr ToUInt(const String &Value, UIntPtr &Current) {
    UIntPtr ret = 0;

    while (IsDigit(Value[Current])) {
        ret = (ret * 10) + (Value[Current++] - '0');
    }

    return ret;
}

static const Char *FindFirst(const Char *Data, Char Value, UIntPtr Length) {
    /* Just find the first occurrence of the character Value in the buffer Data. This can be extended to not only
     * strings (if we just change const Char* into const Void*). */

    for (; Length--; Data++) {
        if (*Data == Value) {
            return Data;
        }
    }

    return Null;
}

static UIntPtr ParseFlags(const String &Format, VariadicList &Arguments, UIntPtr Current, Boolean &LeftJust,
                          UInt8 &Sign, Boolean &Zero, Boolean &WidthSet, Boolean &PrecisionSet, UIntPtr &Width,
                          UIntPtr &Precision) {
    UInt8 state = 0;
    UIntPtr ret = Current;

    /* The minus sign indicated that we should justify to the left (instead of right).
     * The plus sign indicates that we should always append the sign.
     * A zero indicates that the padding should use zeroes instead of spaces.
     * A space indicates that the padding should use spaces instead of zeroes, or that we should use a space instead
     * of the plus sign (if it is a positive number that we're printing).
     * An asterisk indicates that we should set the field width (using the VariadicList).
     * A dot indicates that we should set the precision/number of digits after the floating point (well, it's actually
     * not only for floats/doubles).
     * Finally, if there are numbers, it indicates that we should set the field width (without using the
     * VariadicList). */

    while (True) {
        if (state == 1) {
            return 0;
        } else if (state == 2) {
            return ret - Current;
        }

        switch (Format[ret]) {
            case '-': LeftJust = True; ret++; break;
            case '+': Sign = 1; ret++; break;
            case '0': Zero = True; ret++; break;
            case ' ': {
                if (Sign) {
                    Sign++;
                }

                Zero = False;
                ret++;

                break;
            }
            case '*': {
                if (WidthSet) {
                    /* We don't want to set the width twice, error out. */

                    state = 1;
                } else {
                    Width = VariadicArg(Arguments, UIntPtr);
                    WidthSet = True;
                    ret++;
                }

                break;
            }
            case '.': {
                if (PrecisionSet) {
                    state = 1;
                    break;
                }

                ret++;

                /* Check if we should use the variadic args, if we should manually parse the number from the format
                 * string, or maybe if we should just error out. */

                if (IsDigit(Format[ret])) {
                    Precision = ToUInt(Format, ret);
                    PrecisionSet = True;
                } else if (Format[ret] == '*') {
                    Precision = VariadicArg(Arguments, UIntPtr);
                    PrecisionSet = True;
                    ret++;
                } else {
                    state = 1;
                }

                break;
            }
            default: {
                if (IsDigit(Format[ret]) && !WidthSet) {
                    Width = ToUInt(Format, ret);
                    WidthSet = True;
                } else if (IsDigit(Format[ret])) {
                    state = 1;
                } else {
                    state = 2;
                }

                break;
            }
        }
    }
}

static Char *WriteCharacter(Char Data, Boolean (*Function)(Char, Void*), Void *Context, Char *Buffer, UIntPtr Size,
                            UIntPtr Limit, UIntPtr Position, Boolean &Error) {
    /* We have to handle both when the output is a function, and when we just have to write to the screen. We
     * determine which one is the correct using the Function pointer. */

    Error = False;

    if (Function == Null && !(Limit && (Position + 1) >= Size)) {
        *Buffer++ = Data;
    } else if (Function == Null) {
        Error = True;
    } else {
        Error = !Function(Data, Context);
    }

    return Buffer;
}

static Char *WriteString(const Char *Data, UIntPtr DataSize, Boolean (*Function)(Char, Void*), Void *Context,
                         Char *Buffer, UIntPtr Size, UIntPtr Limit, UIntPtr Position, Boolean &Error) {
    /* Same as above, but now we have to do it for each character in the string (while taking caution with the
     * DataSize field). */

    Error = False;

    while (*Data) {
        if (!(DataSize--)) {
            break;
        } else if (Function == Null && !(Limit && (Position + 1) >= Size)) {
            *Buffer++ = *Data++;
        } else if (Function == Null) {
            Error = True;
        } else {
            Error = !Function(*Data++, Context);
        }

        if (Error) {
            return Buffer;
        }
    }

    return Buffer;
}

UIntPtr VariadicFormat(const String &Format, VariadicList &Arguments, Boolean (*Function)(Char, Void*), Void *Context,
                       Char *Buffer, UIntPtr Size, UIntPtr Limit) {
    if (Function == Null && Buffer == Null) {
        return 0;
    }

    UIntPtr pos = 0, written = 0;
    Boolean err = False;

    /* Let's parse the format string. */

    while (Format[pos]) {
        /* Let's already get the easiest one of the way: If the character isn't the format character (%), just print it
         * normally (also get where the first format character would be, so that we can print all text before it at the
         * same time). */

        if (Format[pos] != '%') {
            const Char *end = FindFirst(Format.GetValue() + pos, '%', Format.GetLength() - pos);
            UIntPtr size = end != Null ? end - Format.GetValue() - pos : Format.GetLength() - pos;

            Buffer = WriteString(Format.GetValue() + pos, size, Function, Context, Buffer, Size, Limit, pos, err);

            if (err) {
                return written;
            }

            pos += size;
            written += size;

            continue;
        }

        /* We do need to parse it, so first call our parse flag function (to get the field width, if we should always
         * print the sign, etc). */

        UInt8 sign = 0;
        Char buf[65] = { 0 };
        UIntPtr width = 0, pr = 0;
        Boolean lj = False, zero = False, wset = False, pset = False;

        pos += ParseFlags(Format, Arguments, pos + 1, lj, sign, zero, wset, pset, width, pr) + 1;

        Char fmtc = Format[pos++];
        UIntPtr pad = zero ? (width > pr ? width : pr) : pr;

        switch (fmtc) {
        case 'd': case 'i':
        case 'D': case 'I': {
            /* Signed base-10 32-bits (D and I are 64-bits) integer, we have a little bit of work to do: Get the
             * number, use FromUInt to convert it into a string (after saving the sign, and making sure that we are
             * passing the absolute value), do the left padding, write the sign (if necessary), write the number
             * itself, and, finally, do the right side padding (if required). */

            Int64 val;

            if (fmtc == 'd' || fmtc == 'i') {
                val = VariadicArg(Arguments, Int32);
            } else {
                val = VariadicArg(Arguments, Int64);
            }

            String str = String::FromUInt(buf, val < 0 ? -val : val, 65, 10);
            UIntPtr len = str.GetLength(), flen = len + (val < 0 || sign),
                    spaces = !zero && width > flen && width > pr ? width - pr - (pr ? 0 : flen) : 0;

            pad = pad > flen ? pad - flen : 0;

            PAD(!zero && !lj && width > pr, spaces, ' ');
            WRITE_SIGN(True);
            PAD(zero || pr, pad, '0');
            WRITE_STRING(str.GetValue(), len);
            PAD(!zero && lj && width > pr, spaces, ' ');

            break;
        }
        case 'u': case 'U':
        case 'b': case 'B':
        case 'o': case 'O':
        case 'x': case 'X': {
            /* All of them are unsigned integers (the upper case ones are 64-bits), what we gonna do is very similar
             * to what we did above, but instead of handling the sign, we have to handle the base.
             * u and U are base 10, b and B are base 2, o and O are base 8, x and X are base 16. */

            UInt8 base = (fmtc == 'u' || fmtc == 'U') ? 10 :
                         ((fmtc == 'b' || fmtc == 'B') ? 2 :
                         ((fmtc == 'o' || fmtc == 'O') ? 8 : 16));
            UInt64 val;

            if (fmtc == 'u' || fmtc == 'b' || fmtc == 'o' || fmtc == 'x') {
                val = VariadicArg(Arguments, UInt32);
            } else {
                val = VariadicArg(Arguments, UInt64);
            }

            String str = String::FromUInt(buf, val, 65, base);
            UIntPtr len = str.GetLength(),
                    spaces = !zero && width > len && width > pr ? width - pr - (pr ? 0 : len) : 0;

            pad = pad > len ? pad - len : 0;

            PAD(!zero && !lj && width > pr, spaces, ' ');
            PAD(zero || pr, pad, '0');
            WRITE_STRING(str.GetValue(), len);
            PAD(!zero && lj && width > pr, spaces, ' ');

            break;
        }
        case 'f': {
            if (!pset) {
                pr = 6;
            }

            Char padc = zero ? '0' : ' ';
            Float val = VariadicArg(Arguments, Float);
            String str = String::FromFloat(buf, val < 0 ? -val : val, 65, pr);
            UIntPtr len = str.GetLength(), flen = len + (val < 0 || sign), pad = width > flen ? width - flen : 0;

            WRITE_SIGN(zero);
            PAD(!lj && width > flen, pad, padc);
            WRITE_SIGN(!zero);
            WRITE_STRING(str.GetValue(), len);
            PAD(lj && width > flen, pad, padc);

            break;
        }
        case 'c': {
            /* Single character + padding. */

            PAD(!lj && width > 2, width - 1, ' ');
            WRITE_CHAR(VariadicArg(Arguments, IntPtr));
            PAD(lj && width > 2, width - 1, ' ');

            break;
        }
        case 's': {
            /* String: We have to remember to limit the size (if required) and apply the padding (again, if
             * required). */

            String str(VariadicArg(Arguments, const Char*));
            UIntPtr len = str.GetLength();

            if (pset && len > pr) {
                len = pr;
            }

            PAD(!lj && width > len, width - len, ' ');
            WRITE_STRING(str.GetValue(), len);
            PAD(lj && width > len, width - len, ' ');

            break;
        }
        default: {
            /* None of the above, probably just escaping another % or some other character. */

            WRITE_CHAR(Format[pos++]);

            break;
        }
        }
    }

    return pos;
}

}
