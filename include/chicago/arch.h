// File author is Ítalo Lima Marconato Matias
//
// Created on May 11 of 2018, at 13:15 BRT
// Last edited on November 15 of 2019, at 21:38 BRT

#ifndef __CHICAGO_ARCH_H__
#define __CHICAGO_ARCH_H__

#include <chicago/types.h>

#define BOOT_OPTIONS_VERBOSE 0x01

extern UIntPtr KernelStack;
extern UIntPtr ArchBootOptions;
extern UIntPtr KernelSymbolTable;
extern UIntPtr KernelSymbolTableSize;

Void ArchHalt(Void);
Void ArchInit(Void);
Void ArchInitSc(Void);
Void ArchInitFPU(Void);
Void ArchInitPMM(Void);
Void ArchInitVMM(Void);
Void ArchInitDebug(Void);
Void ArchInitDisplay(Void);
UIntPtr ArchGetSeconds(Void);
Void ArchPanic(UInt32 err, PVoid priv);
Void ArchUserJump(UIntPtr addr, UIntPtr stack);

#endif		// __CHICAGO_ARCH_H__
