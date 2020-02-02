// File author is Ítalo Lima Marconato Matias
//
// Created on October 29 of 2018, at 18:10 BRT
// Last edited on February 02 of 2020, at 14:10 BRT

#include <chicago/arch/idt.h>
#include <chicago/sc.h>

static Void ArchScHandler(PRegisters regs) {
	regs->rax = ((UIntPtr(*)())ScTable[regs->rax])(regs->rdx, regs->rdi, regs->rsi, regs->r8, regs->r9, regs->r10);
}

Void ArchInitSc(Void) {
	IDTRegisterInterruptHandler(0x3F, ArchScHandler);
}
