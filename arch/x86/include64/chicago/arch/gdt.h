// File author is Ítalo Lima Marconato Matias
//
// Created on May 11 of 2018, at 13:23 BRT
// Last edited on January 19 of 2019, at 18:22 BRT

#ifndef __CHICAGO_ARCH_GDT_H__
#define __CHICAGO_ARCH_GDT_H__

#include <chicago/types.h>

Void GDTSetGate(UInt8 num, UInt64 base, UInt64 limit, UInt8 type, UInt8 gran);
Void GDTWriteTSS(UInt8 num, UInt64 rsp0);
Void GDTSetKernelStack(UInt64 stack);
Void GDTInit(Void);

#endif		// __CHICAGO_ARCH_GDT_H__
