/* File author is Ítalo Lima Marconato Matias
 *
 * Created on June 25 of 2020, at 09:42 BRT
 * Last edited on November 30 of 2020, at 13:09 BRT */

#include <arch/arch.hxx>
#include <textout.hxx>

ArchImpl ArchImp;

Void ArchImpl::Halt(Void) {
	/* The halt instruction actually returns when we get some interrupt, so we need to keep on calling
	 * it in a loop, instead of just calling it once. */
	
	while (True) {
		asm volatile("hlt");
	}
}

Void ArchImpl::Sleep(UIntPtr MilliSeconds) {
	UIntPtr end = Ticks + MilliSeconds;
	
	while (Ticks < end) {
		asm volatile("pause");
	}
}

Void InitArchInterface(Void) {
	Arch = &ArchImp;
}
