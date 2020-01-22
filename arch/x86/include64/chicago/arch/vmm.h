// File author is Ítalo Lima Marconato Matias
//
// Created on June 28 of 2018, at 19:26 BRT
// Last edited on January 21 of 2020, at 11:40 BRT

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
#define PAGE_OTHER (1 << 11)
#define PAGE_AVAL3 (1ull << 52)
#define PAGE_AVAL4 (1ull << 53)
#define PAGE_AVAL5 (1ull << 54)
#define PAGE_AVAL6 (1ull << 55)
#define PAGE_AVAL7 (1ull << 56)
#define PAGE_AVAL8 (1ull << 57)
#define PAGE_AVAL9 (1ull << 58)
#define PAGE_AVAL10 (1ull << 59)
#define PAGE_AVAL11 (1ull << 60)
#define PAGE_AVAL12 (1ull << 61)
#define PAGE_AVAL13 (1ull << 62)
#define PAGE_NOEXEC (1ull << 63)
#define PAGE_MASK 0xFFFFFFFFFF000

#define MmGetP4Idx(i) ((((i) & ~0xFFF) >> 39) & 0x1FF)
#define MmGetP4TIdx(i) ((((i) & ~0xFFF) >> 27) & 0x1FF000)
#define MmGetP3Idx(i) ((((i) & ~0xFFF) >> 30) & 0x1FF)
#define MmGetP3TIdx(i) ((((i) & ~0xFFF) >> 18) & 0x3FFFF000)
#define MmGetP2Idx(i) ((((i) & ~0xFFF) >> 21) & 0x1FF)
#define MmGetP2TIdx(i) ((((i) & ~0xFFF) >> 9) & 0x7FFFFFF000)
#define MmGetP1Idx(i) ((((i) & ~0xFFF) >> 12) & 0x1FF)

#define MmGetP4Int(p4, i) ((PUInt64)(p4))[MmGetP4Idx(i)]
#define MmSetP4Int(p4, i, p, f) ((PUInt64)(p4))[MmGetP4Idx(i)] = ((p) & PAGE_MASK) | ((f) & 0xFFF)

#define MmGetP3LocInt(p3, i) (UIntPtr)((PUInt64)((p3) + MmGetP4TIdx(i)))
#define MmGetP3Int(p3, i) ((PUInt64)((p3) + MmGetP4TIdx(i)))[MmGetP3Idx(i)]
#define MmSetP3Int(p3, i, p, f) ((PUInt64)((p3) + MmGetP4TIdx(i)))[MmGetP3Idx(i)] = ((p) & PAGE_MASK) | ((f) & 0xFFF)

#define MmGetP2LocInt(p2, i) (UIntPtr)((PUInt64)((p2) + MmGetP3TIdx(i)))
#define MmGetP2Int(p2, i) ((PUInt64)((p2) + MmGetP3TIdx(i)))[MmGetP2Idx(i)]
#define MmSetP2Int(p2, i, p, f) ((PUInt64)((p2) + MmGetP3TIdx(i)))[MmGetP2Idx(i)] = ((p) & PAGE_MASK) | ((f) & 0xFFF)

#define MmGetP1LocInt(p1, i) (UIntPtr)((PUInt64)((p1) + MmGetP2TIdx(i)))
#define MmGetP1Int(p1, i) ((PUInt64)((p1) + MmGetP2TIdx(i)))[MmGetP1Idx(i)]
#define MmSetP1Int(p1, i, p, f) ((PUInt64)((p1) + MmGetP2TIdx(i)))[MmGetP1Idx(i)] = ((p) & PAGE_MASK) | ((f) & 0xFFF)

#define MmGetP4(i) MmGetP4Int(0xFFFFFFFFFFFFF000, i)
#define MmSetP4(i, p, f) MmSetP4Int(0xFFFFFFFFFFFFF000, i, p, f)

#define MmGetP3Loc(i) MmGetP3LocInt(0xFFFFFFFFFFE00000, i)
#define MmGetP3(i) MmGetP3Int(0xFFFFFFFFFFE00000, i)
#define MmSetP3(i, p, f) MmSetP3Int(0xFFFFFFFFFFE00000, i, p, f)

#define MmGetP2Loc(i) MmGetP2LocInt(0xFFFFFFFFC0000000, i)
#define MmGetP2(i) MmGetP2Int(0xFFFFFFFFC0000000, i)
#define MmSetP2(i, p, f) MmSetP2Int(0xFFFFFFFFC0000000, i, p, f)

#define MmGetP1Loc(i) MmGetP1LocInt(0xFFFFFF8000000000, i)
#define MmGetP1(i) MmGetP1Int(0xFFFFFF8000000000, i)
#define MmSetP1(i, p, f) MmSetP1Int(0xFFFFFF8000000000, i, p, f)

#define MmInvlpg(i) Asm Volatile("invlpg %0" :: "m"(*((PChar)((i) & ~0xFFF))))

#endif		// __CHICAGO_ARCH_VMM_H__
