/* File author is Ítalo Lima Marconato Matias
 *
 * Created on June 25 of 2020, at 09:44 BRT
 * Last edited on December 01 of 2020, at 16:34 BRT */

#ifndef __ARCH_ARCH_HXX__
#define __ARCH_ARCH_HXX__

#include <arch.hxx>
#include <textout.hxx>

/* x86 (IA-32 and AMD64) implementation of the arch-specific functions. */

#define COM1_PORT 0x3F8

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
	UInt32 Es, Ds;
	UInt32 Edi, Esi, Ebp, Unused, Ebx, Edx, Ecx, Eax;
	UInt32 IntNum, ErrCode;
	UInt32 Eip, Cs, EFlags, Esp, Ss;
};
#endif

struct Context {
	UInt8 KernelStack[0x10000];
	UIntPtr StackPointer;
};

class ArchImpl : public IArch {
public:
	no_return Void Halt(Void) override;
	Void Sleep(UIntPtr) override;
	
	Status GetPhys(Void*, UIntPtr&) override;
	Status Query(Void*, UInt32&) override;
	Status MapTemp(UIntPtr, UInt32, Void*&) override;
	Status Map(Void*, UIntPtr, UInt32) override;
	Status Unmap(Void*) override;
	Status CreateDirectory(UIntPtr&) override;
	Status FreeDirectory(UIntPtr) override;
	UIntPtr GetCurrentDirectory(Void) override;
	Void SwitchDirectory(UIntPtr) override;
	
	Void *CreateContext(UIntPtr) override;
	Void FreeContext(Void*) override;
	
	Void SetKernelBoundaries(UIntPtr Start, UIntPtr End) { KernelStart = Start;
														   KernelEnd = End; }
	UIntPtr GetKernelStart(Void) const { return KernelStart; }
	UIntPtr GetKernelEnd(Void) const { return KernelEnd; }
	
	volatile UIntPtr Ticks = 0;
private:
	UIntPtr KernelStart, KernelEnd;
};

class DebugImpl : public TextOutput {
private:
	Void WriteInt(Char) override;
};

extern ArchImpl ArchImp;

extern "C" Void _init(Void);

Void InitArchInterface(Void);
Void InitDebugInterface(Void);
Void InitTimerInterface(Void);
Void InitProcessInterface(Void);

#endif
