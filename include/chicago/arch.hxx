/* File author is Ítalo Lima Marconato Matias
 *
 * Created on June 25 of 2020, at 09:26 BRT
 * Last edited on August 22 of 2020, at 11:35 BRT */

#ifndef __CHICAGO_ARCH_HXX__
#define __CHICAGO_ARCH_HXX__

#include <chicago/status.hxx>

/* Maybe we should later we should move all the kernel classes into a namespace... */

class IArch {
public:
	/* Functions that don't enter in any categories (like the one to jump into userspace, or the one
	 * to halt etc.) */
	
	virtual Void Halt(Void) = 0;
	
	/* virtual memory management. */
	
	virtual Status GetPhys(Void*, UIntPtr&) = 0;
	virtual Status Query(Void*, UInt32&) = 0;
	virtual Status MapTemp(UIntPtr, UInt32, Void*&) = 0;
	virtual Status Map(Void*, UIntPtr, UInt32) = 0;
	virtual Status Unmap(Void*) = 0;
	virtual Status CreateDirectory(UIntPtr&) = 0;
	virtual Status FreeDirectory(UIntPtr) = 0;
	virtual UIntPtr GetCurrentDirectory(Void) = 0;
	virtual Void SwitchDirectory(UIntPtr) = 0;
};

extern IArch *Arch;

Void KernelMain(Void);

#endif