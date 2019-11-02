// File author is Ítalo Lima Marconato Matias
//
// Created on May 27 of 2018, at 13:38 BRT
// Last edited on October 28 of 2019, at 17:03 BRT

#ifndef __CHICAGO_ARCH_BOOTMGR_H__
#define __CHICAGO_ARCH_BOOTMGR_H__

#include <chicago/types.h>

typedef struct {
	UIntPtr base;
	UIntPtr length;
	UIntPtr type;
} Packed BootmgrMemoryMap, *PBootmgrMemoryMap;

extern PWChar BootmgrBootDev;
extern UIntPtr BootmgrDispBpp;
extern UIntPtr BootmgrDispWidth;
extern UIntPtr BootmgrDispHeight;
extern UIntPtr BootmgrMemMapCount;
extern UIntPtr BootmgrDispPhysAddr;
extern PBootmgrMemoryMap BootmgrMemMap;

#endif		// __CHICAGO_ARCH_BOOTMGR_H__
