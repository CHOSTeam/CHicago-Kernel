/* File author is Ítalo Lima Marconato Matias
 *
 * Created on June 25 of 2020, at 09:44 BRT
 * Last edited on August 22 of 2020, at 11:34 BRT */

#ifndef __CHICAGO_ARCH_ARCH_HXX___
#define __CHICAGO_ARCH_ARCH_HXX___

#include <chicago/arch.hxx>
#include <chicago/textout.hxx>

/* x86 (IA-32 and AMD64) implementation of the arch-specific functions. */

#define COM1_PORT 0x3F8

class ArchImpl : public IArch {
public:
	no_return Void Halt(Void) override;
	
	Status GetPhys(Void*, UIntPtr&) override;
	Status Query(Void*, UInt32&) override;
	Status MapTemp(UIntPtr, UInt32, Void*&) override;
	Status Map(Void*, UIntPtr, UInt32) override;
	Status Unmap(Void*) override;
	Status CreateDirectory(UIntPtr&) override;
	Status FreeDirectory(UIntPtr) override;
	UIntPtr GetCurrentDirectory(Void) override;
	Void SwitchDirectory(UIntPtr) override;
	
	Void SetKernelBoundaries(UIntPtr Start, UIntPtr End) { KernelStart = Start;
														   KernelEnd = End; }
	UIntPtr GetKernelStart(Void) const { return KernelStart; }
	UIntPtr GetKernelEnd(Void) const { return KernelEnd; }
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

#endif
