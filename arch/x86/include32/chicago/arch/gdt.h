// File author is Ítalo Lima Marconato Matias
//
// Created on May 11 of 2018, at 13:23 BRT
// Last edited on February 02 of 2020, at 19:10 BRT

#ifndef __CHICAGO_ARCH_GDT_H__
#define __CHICAGO_ARCH_GDT_H__

#include <chicago/types.h>

Void GDTSetGate(UInt8 num, UInt32 base, UInt32 limit, UInt8 type, UInt8 gran);
Void GDTWriteTSS(UInt8 num, UInt32 ss0, UInt32 esp0);
Void GDTSetKernelStack(UInt32 stack);
Void GDTSetFS(UInt32 fs);
Void GDTSetGS(UInt32 gs);
UInt32 GDTGetFS(Void);
UInt32 GDTGetGS(Void);
Void GDTInit(Void);

#endif		// __CHICAGO_ARCH_GDT_H__
