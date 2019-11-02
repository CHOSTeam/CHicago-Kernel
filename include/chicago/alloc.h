// File author is Ítalo Lima Marconato Matias
//
// Created on July 13 of 2018, at 00:45 BRT
// Last edited on September 09 of 2019, at 15:48 BRT

#ifndef __CHICAGO_ALLOC_H__
#define __CHICAGO_ALLOC_H__

#include <chicago/types.h>

UIntPtr MemAllocate(UIntPtr size);
UIntPtr MemAAllocate(UIntPtr size, UIntPtr align);
Void MemFree(UIntPtr blockk);
Void MemAFree(UIntPtr block);
UIntPtr MemZAllocate(UIntPtr size);
UIntPtr MemReallocate(UIntPtr blockk, UIntPtr size);
UIntPtr MemGetAllocSize(UIntPtr block);

#endif		// __CHICAGO_ALLOC_H__
