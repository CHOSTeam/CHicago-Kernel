// File author is Ítalo Lima Marconato Matias
//
// Created on November 15 of 2019, at 20:07 BRT
// Last edited on November 16 of 2019, at 11:33 BRT

#ifndef __CHICAGO_DRV_H__
#define __CHICAGO_DRV_H__

#include <chicago/exec.h>

typedef struct {
	PList symbols;
	PList deps;
	PWChar name;
	UIntPtr base;
	PThread thread;
} DrvHandle, *PDrvHandle;

PLibHandle DrvLoadLibCHKrnl(Void);
PDrvHandle DrvLoad(PWChar name);

#endif		// __CHICAGO_DRV_H__
