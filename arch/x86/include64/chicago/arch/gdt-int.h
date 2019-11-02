// File author is Ítalo Lima Marconato Matias
//
// Created on January 19 of 2019, at 18:14 BRT
// Last edited on January 19 of 2019, at 18:22 BRT

#ifndef __CHICAGO_ARCH_GDT_INT_H__
#define __CHICAGO_ARCH_GDT_INT_H__

#include <chicago/types.h>

typedef struct {
	UInt32 res0;
	UInt64 rsp0;
	UInt64 rsp1;
	UInt64 rsp2;
	UInt64 res1;
	UInt64 ist1;
	UInt64 ist2;
	UInt64 ist3;
	UInt64 ist4;
	UInt64 ist5;
	UInt64 ist6;
	UInt64 ist7;
	UInt64 res2;
	UInt16 res3;
	UInt16 iomap_base;
} Packed TSSEntry, *PTSSEntry;

extern Void GDTLoad(UInt64 base, UInt16 limit);
extern Void TSSLoad(UInt64 sel);

#endif		// __CHICAGO_ARCH_GDT_INT_H__
