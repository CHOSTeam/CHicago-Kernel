/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 07 of 2021, at 15:57 BRT
 * Last edited on February 24 of 2021 at 11:03 BRT */

#include <string.hxx>

using namespace CHicago;

/* Some macros to make our life a bit easier. */

#define WRITE_CHAR(c) \
    if (!Function(c, Context)) { \
        return written; \
    } \
    \
    written++

#define WRITE_STRING(str, sz) \
    if (!WriteString(str, sz, Function, Context)) { \
        return written; \
    } \
    \
    written += sz

#define PAD(cnt, c) \
    do { for (UIntPtr i = 0; i < (cnt); i++) { \
             WRITE_CHAR(c); \
         } \
       } while (False)

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

static Boolean WriteString(const Char *Data, UIntPtr DataSize, Boolean (*Function)(Char, Void*), Void *Context) {
    while (*Data) {
        if (!(DataSize--)) {
            break;
        } else if (!Function(*Data++, Context)) {
            return False;
        }
    }

    return True;
}

static Boolean IsNormal(Float Value) {
    union { Float FloatValue; UInt64 IntValue; } val { .FloatValue = Value };
    return !(val.IntValue & 0xFFFFFFFFFFFFF);
}

UIntPtr VariadicFormatInt(Boolean (*Function)(Char, Void*), Void *Context, const String &Format,
                          const ArgumentList &Arguments) {
    if (Function == Null) {
        return 0;
    }

    UIntPtr last = 0, pos = 0, written = 0;

    /* Let's parse the format string, most of it will probably just be raw text. */

    while (Format[pos]) {
        /* Let's already get the easiest one of the way: If the character isn't the format start character, just print
         * it normally (also get where the first format character would be, so that we can print all text before it at
         * the same time). */

        if (Format[pos] != '{') {
            const Char *end = FindFirst(Format.GetValue() + pos, '{', Format.GetLength() - pos);
            UIntPtr size = end != Null ? end - Format.GetValue() - pos : Format.GetLength() - pos;

            if (!WriteString(Format.GetValue() + pos, size, Function, Context)) {
                break;
            }

            pos += size;
            written += size;

            continue;
        }

        /* Now it is parsing time, we except something on the format '{pos:width.prec:base}', where everything is
         * optional (and the type is parsed automatically using the argument list). It has to be on that format/order,
         * if it isn't, we're going to error out. Also, some things are invalid in a few types (like the base on floats,
         * or the precision in characters), but in those cases we're just going to ignore said invalid data. */

        pos++;

        Int8 pset = 0;
        UInt8 base = 10;
        Char buf[65] = { 0 };
        UIntPtr idx = 0, width = 0, prec = 0;
        Boolean zero = False, iset = False, wset = False;

        if (Format[pos] == '{') {
            /* One extra valid format: '{{', it means that we should just print '{'. */

            WRITE_CHAR('{');
            pos++;

            continue;
        }

        if (Format[pos] != '}' && Format[pos] != ':') {
            /* Expect a number, which will tell us the position of the argument on the arg list, if it is not here,
             * error out. */

            if (!IsDigit(Format[pos])) {
                return written;
            }

            idx = ToUInt(Format, pos);
            iset = True;

            /* Remember to make sure the index is not crazy (as we DO have the var arg list size). */

            if (idx >= Arguments.GetCount()) {
                return written;
            }
        }

        if (Format[pos] == ':') {
            /* Width/precision, both separated by a dot, read them in (and of course, if they aren't here, just error
             * out). Also, before the width, there might be a zero, that means that we should pad using zeroes instead
             * of spaces (and it is NOT part of the main width/prec that we're going to parse). */

            if (Format[++pos] == '0') {
                zero = True;
                pos++;
            }

            /* Well, and also there is the case where we have an '*' instead of the width/precision. */

            if (Format[pos] != '.' && Format[pos] != ':' && Format[pos] != '}') {
                if (Format[pos] == '*') {
                    wset = True;
                    pos++;
                } else {
                    if (!IsDigit(Format[pos])) {
                        return written;
                    }

                    width = ToUInt(Format, pos);
                }
            }

            if (Format[pos] != '.' && Format[pos] != ':' && Format[pos] != '}') {
                return written;
            } else if (Format[pos] == '.' && Format[pos + 1] == '*') {
                prec = sizeof(UIntPtr) * 2;
                pset = 2;
                pos += 2;
            } else if (Format[pos] == '.') {
                if (!IsDigit(Format[++pos])) {
                    return written;
                }

                prec = ToUInt(Format, pos);
                pset = 1;
            }
        }

        if (Format[pos] == ':') {
            /* Last possible format specifier, the base, just parse it as an integer. */

            if (!IsDigit(Format[++pos])) {
                return written;
            }

            base = ToUInt(Format, pos);
        }

        if (Format[pos++] != '}') {
            return written;
        }

        /* Now, let's go into actually printing: We can what kind of data we should print using the argument type, so
         * it is not hard. But before that, we have to make sure to set the index if it hasn't been set yet. */

        if (!iset) {
            idx = last++;

            if (idx >= Arguments.GetCount()) {
                return written;
            }
        }

        ArgumentType type = Arguments[idx].GetType();
        ArgumentValue val = Arguments[idx].GetValue();

        switch (type) {
        case ArgumentType::Long: case ArgumentType::Int32: case ArgumentType::Int64: {
            /* Signed integers. We have a little bit of work to do: Use FromUInt to convert the number (after saving the
             * sign and making it positive) into a string, do the padding, write the sign (if necessary), and finally
             * write the number. */

            if (wset) {
                width = sizeof(UIntPtr) * 2;
            }

            if (pset == 2) {
                prec = sizeof(UIntPtr) * 2;
            }

            Int64 ival = type == ArgumentType::Long ? val.LongValue : (type == ArgumentType::Int32 ? val.Int32Value
                                                                                                   : val.Int64Value);
            String str = String::FromUInt(buf, ival < 0 ? -ival : ival, 65, base);
            UIntPtr len = str.GetLength(), flen = len + (ival < 0),
                    spaces = !zero && width > flen && width > prec ? width - prec - (prec ? 0 : flen) : 0,
                    pad = zero ? (width > prec ? width : prec) : prec;

            pad = pad > flen ? pad - flen : 0;

            PAD(spaces, ' ');

            if (ival < 0) {
                WRITE_CHAR('-');
            }

            PAD(pad, '0');
            WRITE_STRING(str.GetValue(), len);

            break;
        }
        case ArgumentType::ULong: case ArgumentType::UInt32: case ArgumentType::UInt64: case ArgumentType::Pointer: {
            /* For unsigned integers, what we gonna do is very similar to what we did above, but there is no need to
             * handle the sign. */

            if (wset) {
                width = sizeof(UIntPtr) * 2;
            }

            if (pset == 2) {
                prec = sizeof(UIntPtr) * 2;
            }

            UInt64 ival = type == ArgumentType::ULong ? val.ULongValue :
                          (type == ArgumentType::UInt32 ? val.UInt32Value :
                          (type == ArgumentType::UInt64 ? val.UInt64Value :
                                                          reinterpret_cast<UIntPtr>(val.PointerValue)));
            String str = String::FromUInt(buf, ival, 65, base);
            UIntPtr len = str.GetLength(),
                    spaces = !zero && width > len && width > prec ? width - prec - (prec ? 0 : len) : 0,
                    pad = zero ? (width > prec ? width : prec) : prec;

            pad = pad > len ? pad - len : 0;

            PAD(spaces, ' ');
            PAD(pad, '0');
            WRITE_STRING(str.GetValue(), len);

            break;
        }
        case ArgumentType::Float: {
            /* For floats/doubles, again, it's pretty much the same, but the precision is handled differently, and we
             * use FromFloat. */

            if (pset == 2) {
                prec = 16;
            }

            Float fval = val.FloatValue;
            Boolean neg = IsNormal(fval) && fval < 0;
            String str = String::FromFloat(buf, neg ? -fval : fval, 65, pset ? prec : 6);
            UIntPtr len = str.GetLength(), flen = len + neg, pad = width > flen ? width - flen : 0;

            if (neg) {
                WRITE_CHAR('-');
            }

            PAD(pad, IsNormal(fval) ? '0' : ' ');
            WRITE_STRING(str.GetValue(), len);

            break;
        }
        case ArgumentType::Boolean: case ArgumentType::Status: case ArgumentType::CString: {
            /* And for strings, we just need to remember the padding (which will be spaces), and limiting the length
             * (using the precision). */

            String str = type == ArgumentType::CString ? val.CStringValue :
                         (type == ArgumentType::Boolean ? (val.BooleanValue ? "True" : "False") :
                                                           String::FromStatus(val.StatusValue));
            UIntPtr len = str.GetLength();

            if (pset && len > prec) {
                len = prec;
            }

            PAD(width > len ? width - len : 0, ' ');
            WRITE_STRING(str.GetValue(), len);

            break;
        }
        case ArgumentType::CHString: {
            /* While CHString is also a string, we need to handle it in a different way, as the start and end depends
             * on the set view start/end. */

            UIntPtr len = val.CHStringValue->GetViewEnd() - val.CHStringValue->GetViewStart();

            if (pset && len > prec) {
                len = prec;
            }

            PAD(width > len ? width - len : 0, ' ');
            WRITE_STRING(val.CHStringValue->GetValue() + val.CHStringValue->GetViewStart(), len);

            break;
        }
        case ArgumentType::Char: {
            /* Finally, for characters, well, we just need to handle the padding. */

            PAD(width > 1 ? width - 1 : 0, ' ');
            WRITE_CHAR(val.CharValue);

            break;
        }
        }
    }

    return written;
}

}
