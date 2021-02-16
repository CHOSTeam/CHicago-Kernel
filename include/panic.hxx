/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 09 of 2021, at 14:24 BRT
 * Last edited on February 15 of 2021 at 22:48 BRT */

#pragma once

#include <stacktrace.hxx>
#include <textout.hxx>

#define ASSERT(x) ((x) ? (Void)0 : Panic::AssertFailed(#x, __FILE__, __PRETTY_FUNCTION__, __LINE__))

namespace UBSan {
    struct SourceLocation {
        const Char *Filename;
        UInt32 Line, Column;
    };

    struct TypeDescriptor {
        UInt16 Kind, Info;
        Char Name[0];
    };

    struct InvalidBuiltinData {
        SourceLocation Location;
        UInt8 Kind;
    };

    struct TypeMismatchData {
        SourceLocation Location;
        const TypeDescriptor &Type;
        UInt8 Align, Kind;
    };

    struct OneArgData {
        SourceLocation Location;
        const TypeDescriptor &Type;
    };

    struct TwoArgData {
        SourceLocation Location;
        const TypeDescriptor &Left, &Right;
    };
}

class Panic {
public:
    static no_return Void AssertFailed(const String&, const String&, const String&, UInt32);
};
