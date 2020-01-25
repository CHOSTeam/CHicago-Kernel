// File author is Ítalo Lima Marconato Matias
//
// Created on June 28 of 2018, at 19:26 BRT
// Last edited on January 25 of 2020, at 12:06 BRT

#ifndef __CHICAGO_ARCH_VMM_H__
#define __CHICAGO_ARCH_VMM_H__

#include <chicago/types.h>

#define PAGE_PRESENT (1 << 0)
#define PAGE_WRITE (1 << 1)
#define PAGE_USER (1 << 2)
#define PAGE_WRITE_THROUGH (1 << 3)
#define PAGE_NO_CACHE (1 << 4)
#define PAGE_ACCESSED (1 << 5)
#define PAGE_DIRTY (1 << 6)
#define PAGE_HUGE (1 << 7)
#define PAGE_GLOBAL (1 << 8)
#define PAGE_AOR (1 << 9)
#define PAGE_COW (1 << 10)
#define PAGE_AVAL3 (1 << 11)

#define MmGetPDEInt(pd, i) ((PUInt32)(pd))[((i) & ~0xFFF) >> 22]
#define MmSetPDEInt(pd, i, p, f) ((PUInt32)(pd))[((i) & ~0xFFF) >> 22] = ((p) & ~0xFFF) | ((f) & 0xFFF)

#define MmGetPTELocInt(pt, i) (UIntPtr)((PUInt32)((pt) + ((((i) & ~0xFFF) >> 22) * 0x1000)))
#define MmGetPTEInt(pt, i) ((PUInt32)((pt) + ((((i) & ~0xFFF) >> 22) * 0x1000)))[(((i) & ~0xFFF) << 10) >> 22]
#define MmSetPTEInt(pt, i, p, f) ((PUInt32)((pt) + ((((i) & ~0xFFF) >> 22) * 0x1000)))[(((i) & ~0xFFF) << 10) >> 22] = ((p) & ~0xFFF) | ((f) & 0xFFF)

#define MmGetPDE(i) MmGetPDEInt(0xFFFFF000, i)
#define MmSetPDE(i, p, f) MmSetPDEInt(0xFFFFF000, i, p, f)

#define MmGetPTELoc(i) MmGetPTELocInt(0xFFC00000, i)
#define MmGetPTE(i) MmGetPTEInt(0xFFC00000, i)
#define MmSetPTE(i, p, f) MmSetPTEInt(0xFFC00000, i, p, f)

#define MmInvlpg(i) Asm Volatile("invlpg (%0)" :: "r"((i) & ~0xFFF) : "memory")

#endif		// __CHICAGO_ARCH_VMM_H__
