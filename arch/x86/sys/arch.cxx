/* File author is Ítalo Lima Marconato Matias
 *
 * Created on June 25 of 2020, at 09:42 BRT
 * Last edited on August 07 of 2020, at 19:46 BRT */

#include <chicago/arch/arch.hxx>

ArchImpl ArchImp;

Void ArchImpl::Halt(Void) {
	/* The halt instruction actually returns when we get some interrupt, so we need to keep on calling it in a loop, instead
	 * of just calling it once. */
	
	while (True) {
		asm volatile("hlt");
	}
}

Void InitArchInterface(Void) {
	Arch = &ArchImp;
}
