// File author is Ítalo Lima Marconato Matias
//
// Created on September 15 of 2018, at 12:48 BRT
// Last edited on January 18 of 2020, at 16:53 BRT

#ifndef __CHICAGO_VIRT_H__
#define __CHICAGO_VIRT_H__

#include <chicago/status.h>

#ifndef MM_PAGE_SIZE
#define MM_PAGE_SIZE 0x1000
#endif

#define VIRT_FLAGS_HIGHEST 0x01
#define VIRT_FLAGS_ALLOCNOW 0x02

#define VIRT_PROT_READ 0x04
#define VIRT_PROT_WRITE 0x08
#define VIRT_PROT_EXEC 0x10

Status VirtAllocAddress(UIntPtr addr, UIntPtr size, UInt32 flags, PUIntPtr ret);
Status VirtFreeAddress(UIntPtr addr, UIntPtr size);
UInt32 VirtQueryProtection(UIntPtr addr);
Status VirtChangeProtection(UIntPtr addr, UIntPtr size, UInt32 flags);
UIntPtr VirtGetUsage(Void);

#endif		// __CHICAGO_VIRT_H__
