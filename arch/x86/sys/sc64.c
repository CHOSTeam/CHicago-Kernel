// File author is Ítalo Lima Marconato Matias
//
// Created on October 29 of 2018, at 18:10 BRT
// Last edited on January 18 of 2020, at 10:17 BRT

#include <chicago/arch/idt.h>
#include <chicago/sc.h>

static Void ArchScHandler(PRegisters regs) {
	regs->rax = ((UIntPtr(*)())ScTable[regs->rax])(regs->rbx, regs->rcx, regs->rdx, regs->rsi, regs->rdi, regs->rbp);
}

Void ArchInitSc(Void) {
	IDTRegisterInterruptHandler(0x3F, ArchScHandler);
}
