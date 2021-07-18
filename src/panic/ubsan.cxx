/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 15 of 2021, at 11:36 BRT
 * Last edited on July 17 of 2021, at 22:18 BRT */

#include <sys/arch.hxx>
#include <sys/panic.hxx>

using namespace CHicago;
using namespace UBSan;

static const Char *TypeMismatchKind[] = {
    "load of", "store to", "reference binding to", "member access within", "member call on", "constructor call on",
    "downcast of", "downcast of", "upcast of", "cast to virtual base of", "_Nonnull binding to"
};

static inline always_inline Void Prologue() {
    Arch::EnterPanicState();
    Debug.Write("{}panic: ubsan: ", SetForeground { 0xFFFF0000 });
}

static inline always_inline no_return Void Epilogue(const SourceLocation &Location) {
    Debug.Write("at: {}:{}:{}\n", Location.Filename, Location.Line, Location.Column);
    StackTrace::Dump();
    Arch::Halt(True);
}

extern "C" no_return Void __ubsan_handle_invalid_builtin(InvalidBuiltinData &Data) {
    Prologue();
    Debug.Write("passed 0 to {}\n", !Data.Kind ? "ctz()" : "clz()");
    Epilogue(Data.Location);
}

extern "C" no_return Void __ubsan_handle_vla_bound_not_positive(OneArgData &Data, UIntPtr) {
    Prologue();
    Debug.Write("negative vla bound for {}\n", Data.Type.Name);
    Epilogue(Data.Location);
}

extern "C" no_return Void __ubsan_handle_out_of_bounds(TwoArgData &Data, UIntPtr) {
    Prologue();
    Debug.Write("out of bounds index for {}\n", Data.Left.Name);
    Epilogue(Data.Location);
}

extern "C" no_return Void __ubsan_handle_shift_out_of_bounds(TwoArgData &Data, UIntPtr, UIntPtr) {
    Prologue();
    Debug.Write("shift between {} and {} overflows\n", Data.Left.Name, Data.Right.Name);
    Epilogue(Data.Location);
}

extern "C" no_return Void __ubsan_handle_add_overflow(OneArgData &Data, UIntPtr, UIntPtr) {
    Prologue();
    Debug.Write("addition overflows type {}\n", Data.Type.Name);
    Epilogue(Data.Location);
}

extern "C" no_return Void __ubsan_handle_sub_overflow(OneArgData &Data, UIntPtr, UIntPtr) {
    Prologue();
    Debug.Write("subtraction overflows type {}\n", Data.Type.Name);
    Epilogue(Data.Location);
}

extern "C" no_return Void __ubsan_handle_mul_overflow(OneArgData &Data, UIntPtr, UIntPtr) {
    Prologue();
    Debug.Write("multiplication overflows type {}\n", Data.Type.Name);
    Epilogue(Data.Location);
}

extern "C" no_return Void __ubsan_handle_divrem_overflow(OneArgData &Data, UIntPtr, UIntPtr) {
    Prologue();
    Debug.Write("division/remainder overflows type {}\n", Data.Type.Name);
    Epilogue(Data.Location);
}

extern "C" no_return Void __ubsan_handle_negate_overflow(OneArgData &Data, UIntPtr) {
    Prologue();
    Debug.Write("negation overflows type {}\n", Data.Type.Name);
    Epilogue(Data.Location);
}

extern "C" no_return Void __ubsan_handle_pointer_overflow(SourceLocation &Location, UIntPtr Base, UIntPtr Result) {
    Prologue();
    Debug.Write("operation made pointer 0x{:0*:16} overflowed into 0x{:0*:16}\n", Base, Result);
    Epilogue(Location);
}

extern "C" no_return Void __ubsan_handle_load_invalid_value(OneArgData &Data, UIntPtr) {
    Prologue();
    Debug.Write("loaded invalid value into {}\n", Data.Type.Name);
    Epilogue(Data.Location);
}

extern "C" no_return Void __ubsan_handle_type_mismatch_v1(TypeMismatchData &Data, UIntPtr Address) {
    Prologue();

    if (!Address) Debug.Write("{} {} null pointer\n", TypeMismatchKind[Data.Kind], Data.Type.Name);
    else if (Address & (((UIntPtr)1 << Data.Align) - 1)) {
        Debug.Write("{} {} misaligned address 0x{:0*:16} (expected {}-byte alignment)\n", TypeMismatchKind[Data.Kind],
                    Data.Type.Name, Address, (UIntPtr)1 << Data.Align);
    } else Debug.Write("{} address 0x{:0*:16} without enough space for {}\n", TypeMismatchKind[Data.Kind], Address,
                                                                              Data.Type.Name);

    Epilogue(Data.Location);
}
