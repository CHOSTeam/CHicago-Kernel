/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 09 of 2021, at 14:24 BRT
 * Last edited on February 18 of 2021 at 10:43 BRT */

#pragma once

#include <stacktrace.hxx>

#define ASSERT(x) ((x) ? (Void)0 : Panic::AssertFailed(#x, __FILE__, __PRETTY_FUNCTION__, __LINE__))

namespace UBSan {

struct SourceLocation {
    const CHicago::Char *Filename;
    CHicago::UInt32 Line, Column;
};

struct TypeDescriptor {
    CHicago::UInt16 Kind, Info;
    CHicago::Char Name[0];
};

struct InvalidBuiltinData {
    SourceLocation Location;
    CHicago::UInt8 Kind;
};

struct TypeMismatchData {
    SourceLocation Location;
    const TypeDescriptor &Type;
    CHicago::UInt8 Align, Kind;
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

namespace CHicago {

class Panic {
public:
    static no_return Void AssertFailed(const String&, const String&, const String&, UInt32);
};

}
