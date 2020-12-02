/* File author is Ítalo Lima Marconato Matias
 *
 * Created on June 29 of 2020, at 09:47 BRT
 * Last edited on November 30 of 2020, at 18:30 BRT */

#ifndef __ARCH_DESCTABLES_HXX__
#define __ARCH_DESCTABLES_HXX__

#include <arch/arch.hxx>

struct packed TSSEntry {
#ifdef ARCH_64
	UInt32 Res0;
	UInt64 Rsp0, Rsp1, Rsp2;
	UInt64 Res1;
	UInt64 Ist1, Ist2, Ist3, Ist4, Ist5, Ist6, Ist7;
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
	UInt32 Eax, Ecx, Edx, Ebx, Esp, Ebp, Esi, Edi;
	UInt32 Es, Cs, Ss, Ds, Fs, Gs;
	UInt32 Ldt;
	UInt16 Rrap;
#endif
	UInt16 IoMapBase;
};

struct packed DescTablePointer {
	UInt16 Limit;
	UIntPtr Base;
};

typedef Void (*InterruptHandlerFunc)(Registers*);

extern "C" UIntPtr IdtDefaultHandlers[256];

Void GdtInit(Void);

Void IdtSetHandler(UInt8, InterruptHandlerFunc);
Void IdtInit(Void);

#endif
