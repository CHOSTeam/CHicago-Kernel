// File author is Ítalo Lima Marconato Matias
//
// Created on May 26 of 2018, at 23:18 BRT
// Last edited on January 20 of 2019, at 14:05 BRT

#ifndef __CHICAGO_ARCH_REGISTERS_H__
#define __CHICAGO_ARCH_REGISTERS_H__

#include <chicago/types.h>

typedef struct {
	UInt64 es, ds;
	UInt64 r15, r14, r13, r12, r11, r10, r9, r8;
	UInt64 rbp, rdi, rsi, rdx, rcx, rbx, rax;
	UInt64 int_num, err_code;
	UInt64 rip, cs, rflags, rsp, ss;
} Registers, *PRegisters;

typedef Void (*PInterruptHandler)(PRegisters);

#endif		// __CHICAGO_ARCH_REGISTERS_H__
