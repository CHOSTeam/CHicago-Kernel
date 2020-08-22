/* File author is Ítalo Lima Marconato Matias
 *
 * Created on June 29 of 2020, at 09:47 BRT
 * Last edited on August 22 of 2020, at 12:26 BRT */

#ifndef __CHICAGO_ARCH_DESCTABLES_HXX__
#define __CHICAGO_ARCH_DESCTABLES_HXX__

#include <chicago/types.hxx>

struct packed TSSEntry {
#ifdef ARCH_64
	UInt32 Res0;
	UInt64 Rsp0;
	UInt64 Rsp1;
	UInt64 Rsp2;
	UInt64 Res1;
	UInt64 Ist1;
	UInt64 Ist2;
	UInt64 Ist3;
	UInt64 Ist4;
	UInt64 Ist5;
	UInt64 Ist6;
	UInt64 Ist7;
	UInt64 Res2;
	UInt16 Res3;
#else
	UInt32 Prev;
	UInt32 Esp0;
	UInt32 Ss0;
	UInt32 Esp1;
	UInt32 Ss1;
	UInt32 Esp2;
	UInt32 Ss2;
	UInt32 Cr3;
	UInt32 Eip;
	UInt32 EFlags;
	UInt32 Eax;
	UInt32 Ecx;
	UInt32 Edx;
	UInt32 Ebx;
	UInt32 Esp;
	UInt32 Ebp;
	UInt32 Esi;
	UInt32 Edi;
	UInt32 Es;
	UInt32 Cs;
	UInt32 Ss;
	UInt32 Ds;
	UInt32 Fs;
	UInt32 Gs;
	UInt32 Ldt;
	UInt16 Rrap;
#endif
	UInt16 IoMapBase;
};

struct packed DescTablePointer {
	UInt16 Limit;
	UIntPtr Base;
};

#ifdef ARCH_64
struct packed Registers {
	UInt64 Es, Ds;
	UInt64 R15, R14, R13, R12, R11, R10, R9, R8;
	UInt64 Rbp, Rdi, Rsi, Rdx, Rcx, Rbx, Rax;
	UInt64 IntNum, ErrCode;
	UInt64 Rip, Cs, RFlags, Rsp, Ss;
};
#else
struct packed Registers {
	UInt32 Gs, Fs, Es, Ds;
	UInt32 Edi, Esi, Ebp, Unused, Ebx, Edx, Ecx, Eax;
	UInt32 IntNum, ErrCode;
	UInt32 Eip, Cs, EFlags, Esp, Ss;
};
#endif

typedef Void (*InterruptHandlerFunc)(Registers*);

extern "C" UIntPtr IdtDefaultHandlers[256];

Void GdtInit(Void);

Void IdtSetHandler(UInt8, InterruptHandlerFunc);
Void IdtInit(Void);

#endif
