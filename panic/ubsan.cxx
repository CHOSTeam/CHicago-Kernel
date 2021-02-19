/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 15 of 2021, at 11:36 BRT
 * Last edited on February 18 of 2021 at 18:04 BRT */

#include <arch.hxx>
#include <panic.hxx>

using namespace CHicago;
using namespace UBSan;

static const Char *TypeMismatchKind[] = {
    "load of", "store to", "reference binding to", "member access within", "member call on", "constructor call on",
    "downcast of", "downcast of", "upcast of", "cast to virtual base of", "_Nonnull binding to"
};

static inline always_inline Void Prologue(Void) {
    Debug.SetForeground(0xFFFF0000);
    Debug.Write("panic: ubsan: ");
}

static inline always_inline no_return Void Epilogue(const SourceLocation &Location) {
    Debug.Write("at: %s:%u:%u\n", Location.Filename, Location.Line, Location.Column);
    StackTrace::Dump();
    Arch::Halt(True);
}

extern "C" no_return Void __ubsan_handle_invalid_builtin(InvalidBuiltinData &Data) {
    Prologue();
    Debug.Write("passed 0 to %s\n", !Data.Kind ? "ctz()" : "clz()");
    Epilogue(Data.Location);
}

extern "C" no_return Void __ubsan_handle_out_of_bounds(TwoArgData &Data, UIntPtr) {
    Prologue();
    Debug.Write("out of bounds index for %s\n", Data.Left.Name);
    Epilogue(Data.Location);
}

extern "C" no_return Void __ubsan_handle_shift_out_of_bounds(TwoArgData &Data, UIntPtr, UIntPtr) {
    Prologue();
    Debug.Write("shift between %s and %s overflows\n", Data.Left.Name, Data.Right.Name);
    Epilogue(Data.Location);
}

extern "C" no_return Void __ubsan_handle_add_overflow(OneArgData &Data, UIntPtr, UIntPtr) {
    Prologue();
    Debug.Write("addition overflows type %s\n", Data.Type.Name);
    Epilogue(Data.Location);
}

extern "C" no_return Void __ubsan_handle_sub_overflow(OneArgData &Data, UIntPtr, UIntPtr) {
    Prologue();
    Debug.Write("subtraction overflows type %s\n", Data.Type.Name);
    Epilogue(Data.Location);
}

extern "C" no_return Void __ubsan_handle_mul_overflow(OneArgData &Data, UIntPtr, UIntPtr) {
    Prologue();
    Debug.Write("multiplication overflows type %s\n", Data.Type.Name);
    Epilogue(Data.Location);
}

extern "C" no_return Void __ubsan_handle_divrem_overflow(OneArgData &Data, UIntPtr, UIntPtr) {
    Prologue();
    Debug.Write("division/remainder overflows type %s\n", Data.Type.Name);
    Epilogue(Data.Location);
}

extern "C" no_return Void __ubsan_handle_negate_overflow(OneArgData &Data, UIntPtr) {
    Prologue();
    Debug.Write("negation overflows type %s\n", Data.Type.Name);
    Epilogue(Data.Location);
}

extern "C" no_return Void __ubsan_handle_pointer_overflow(SourceLocation &Location, UIntPtr Base, UIntPtr Result) {
    Prologue();
    Debug.Write("operation made pointer " UINTPTR_MAX_HEX " overflowed into " UINTPTR_MAX_HEX "\n", Base, Result);
    Epilogue(Location);
}

extern "C" no_return Void __ubsan_handle_load_invalid_value(OneArgData &Data, UIntPtr) {
    Prologue();
    Debug.Write("loaded invalid value into %s\n", Data.Type.Name);
    Epilogue(Data.Location);
}

extern "C" no_return Void __ubsan_handle_type_mismatch_v1(TypeMismatchData &Data, UIntPtr Address) {
    Prologue();

    if (!Address) {
        Debug.Write("%s %s null pointer\n", TypeMismatchKind[Data.Kind], Data.Type.Name);
    } else if (Address & (((UIntPtr)1 << Data.Align) - 1)) {
        Debug.Write("%s %s misaligned address " UINTPTR_MAX_HEX " (expected " UINTPTR_DEC "-byte alignment)\n",
                    TypeMismatchKind[Data.Kind], Data.Type.Name, Address, (UIntPtr)1 << Data.Align);
    } else {
        Debug.Write("%s address " UINTPTR_MAX_HEX " without enough space for %s\n", TypeMismatchKind[Data.Kind],
                    Address, Data.Type.Name);
    }

    Epilogue(Data.Location);
}
