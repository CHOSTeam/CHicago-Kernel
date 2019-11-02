// File author is Ítalo Lima Marconato Matias
//
// Created on May 26 of 2018, at 21:19 BRT
// Last edited on August 06 of 2018, at 15:16 BRT

#ifndef __CHICAGO_ARCH_GDT_INT_H__
#define __CHICAGO_ARCH_GDT_INT_H__

#include <chicago/types.h>

typedef struct {
	UInt32 prev;
	UInt32 esp0;
	UInt32 ss0;
	UInt32 esp1;
	UInt32 ss1;
	UInt32 esp2;
	UInt32 ss2;
	UInt32 cr3;
	UInt32 eip;
	UInt32 eflags;
	UInt32 eax;
	UInt32 ecx;
	UInt32 edx;
	UInt32 ebx;
	UInt32 esp;
	UInt32 ebp;
	UInt32 esi;
	UInt32 edi;
	UInt32 es;
	UInt32 cs;
	UInt32 ss;
	UInt32 ds;
	UInt32 fs;
	UInt32 gs;
	UInt32 ldt;
	UInt16 trap;
	UInt16 iomap_base;
} Packed TSSEntry, *PTSSEntry;

extern Void GDTLoad(UInt32 base, UInt16 limit);
extern Void TSSLoad(UInt32 sel);

#endif		// __CHICAGO_ARCH_GDT_INT_H__
