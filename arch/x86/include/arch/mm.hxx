/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 03 of 2020, at 17:28 BRT
 * Last edited on October 24 of 2020, at 14:38 BRT */

#ifndef __ARCH_MM_HXX__
#define __ARCH_MM_HXX__

#include <mm.hxx>

/* Thre are some page flags that are global to both x86-32 and x86-64. x86-64 also have some extra
 * flags, as instead of each entry being 32-bits, each entry is 64-bits (well, x86-32 with PAE also
 * would have 64-bits per entry, but we don't support PAE yet). */

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
#define PAGE_AVAL1 (1 << 11)
#ifdef ARCH_64
#define PAGE_AVAL2 (1ull << 52)
#define PAGE_AVAL3 (1ull << 53)
#define PAGE_AVAL4 (1ull << 54)
#define PAGE_AVAL5 (1ull << 55)
#define PAGE_AVAL6 (1ull << 56)
#define PAGE_AVAL7 (1ull << 57)
#define PAGE_AVAL8 (1ull << 58)
#define PAGE_AVAL9 (1ull << 59)
#define PAGE_AVAL10 (1ull << 60)
#define PAGE_AVAL11 (1ull << 61)
#define PAGE_AVAL12 (1ull << 62)
#define PAGE_NOEXEC (1ull << 63)
#endif

Void PmmInit(Void);
Void VmmInit(Void);

#endif
