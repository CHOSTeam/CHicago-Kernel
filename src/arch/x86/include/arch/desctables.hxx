/* File author is Ítalo Lima Marconato Matias
 *
 * Created on June 29 of 2020, at 09:47 BRT
 * Last edited on July 21 of 2021, at 16:29 BRT */

#pragma once

#include <base/types.hxx>

namespace CHicago {

struct packed Registers {
    UIntPtr Ss, Gs, Fs, Es, Ds, Di, Si,
#ifndef __i386__
            R15, R14, R13, R12, R11, R10, R9, R8,
#endif
            Bp, Sp, Bx, Dx, Cx, Ax, IntNum, ErrCode, Ip, Cs, Flags, Sp2, Ss2;
};

struct packed TssEntry {
#ifdef __i386__
    UInt16 Link, Res0;
    UInt32 Esp0, Ss0, Esp1, Ss1, Esp2, Ss2, Cr3, Eip, EFlags, Eax, Ecx, Edx, Ebx, Esp, Ebp, Esi, Edi;
    UInt16 Es, Res1, Cs, Res2, Ss, Res3, Ds, Res4, Fs, Res5, Gs, Res6, Ldtr;
    UInt32 Res7;
#else
    UInt32 Res0;
    UInt64 Rsp0, Rsp1, Rsp2, Res1, Ist1, Ist2, Ist3, Ist4, Ist5, Ist6, Ist7, Res2;
    UInt16 Res3;
#endif
    UInt16 IoMapBase;
};

struct packed DescTablePointer {
	UInt16 Limit;
	UIntPtr Base;
};

class Gdt {
public:
    Void Initialize(Void);
    Void Reload(Void) const;
#ifdef __i386__
    Void LoadSpecialSegment(Boolean, UIntPtr);
#endif
private:
    Void SetTss(UInt8, UIntPtr);
    Void SetGate(UInt8, UIntPtr, UIntPtr, UInt8, UInt8);

    TssEntry Tss {};
    UInt8 Entries[8][8] {};
    DescTablePointer Pointer {};
};

typedef Void (*InterruptHandlerFunc)(Registers&);

extern "C" UIntPtr IdtDefaultHandlers[256];

/* Probably not really the best place to put it, but we do use MSRs and other system registers, and we need some
 * functions to easily read/write them. */

static inline UInt64 ReadMsr(UInt32 Num) {
#ifdef __i386__
    UInt64 val; asm volatile("rdmsr" : "=A"(val) : "c"(Num)); return val;
#else
    UInt32 eax, edx; asm volatile("rdmsr" : "=a"(eax), "=d"(edx) : "c"(Num)); return eax | ((UInt64)edx << 32);
#endif
}

static inline Void WriteMsr(UInt32 Num, UInt64 Value) {
#ifdef __i386__
    asm volatile("wrmsr" :: "A"(Value), "c"(Num));
#else
    asm volatile("wrmsr" :: "a"(Value & 0xFFFFFFFF), "c"(Num), "d"(Value >> 32));
#endif
}

Void IdtSetHandler(UInt8, InterruptHandlerFunc);
UInt8 IdtAllocIrq(Void);
Void IdtReload(Void);
Void IdtInit(Void);

}
