// File author is Ítalo Lima Marconato Matias
//
// Created on November 16 of 2018, at 00:48 BRT
// Last edited on February 02 of 2020, at 20:58 BRT

#include <chicago/arch/idt.h>
#include <chicago/sc.h>

static Void ArchScHandler(PRegisters regs) {
	if (regs->eax >= SC_MAX) {
		regs->eax = STATUS_INVALID_SYSCALL;																									// Invalid syscall...
	} else {
		regs->eax = ((UIntPtr(*)())ScTable[regs->eax])(regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi, regs->ebp);					// Just redirect to the right function from our syscall table
	}
}

Void ArchInitSc(Void) {
	IDTRegisterInterruptHandler(0x3F, ArchScHandler);																						// Just redirect to the right function from our syscall table
}
