// File author is Ítalo Lima Marconato Matias
//
// Created on October 29 of 2018, at 18:10 BRT
// Last edited on February 02 of 2020, at 18:45 BRT

#include <chicago/arch/idt.h>
#include <chicago/arch/msr.h>
#include <chicago/sc.h>

extern Void SCEntry(Void);

Void ArchScHandler(PRegisters regs) {
	if (regs->rax >= SC_MAX) {
		regs->rax = STATUS_INVALID_SYSCALL;																									// Invalid syscall...
	} else {
		regs->rax = ((UIntPtr(*)())ScTable[regs->rax])(regs->rdx, regs->rdi, regs->rsi, regs->r8, regs->r9, regs->r10);						// Just redirect to the right function from our syscall table
	}
}

Void ArchInitSc(Void) {
	IDTRegisterInterruptHandler(0x3F, ArchScHandler);																						// Register the interrput handler
	MsrWrite(MSR_STAR, 0x13000800000000);																									// Set the selector for the SYSCALL instruction
	MsrWrite(MSR_LSTAR, (UInt64)SCEntry);																									// Set the entry point for the SYSCALL instruction
	MsrWrite(MSR_SFMASK, 0x600);																											// Set the flags that we should clear upon SYSCALL
	MsrWrite(MSR_EFER, MsrRead(MSR_EFER) | 0x01);																							// And enable the SYCALL instruction
}
