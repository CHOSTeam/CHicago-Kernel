/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 03 of 2020, at 17:28 BRT
 * Last edited on March 05 of 2021, at 13:22 BRT */

#pragma once

#include <base/types.hxx>

/* There are some page flags that are global to both x86-32 and x86-64. x86-64 also have some extra flags, as instead
 * of each entry being 32-bits, each entry is 64-bits (well, x86-32 with PAE also would have 64-bits per entry, but we
 * don't support PAE yet). */

#define PAGE_PRESENT (1 << 0)
#define PAGE_WRITE (1 << 1)
#define PAGE_USER (1 << 2)
#define PAGE_HUGE (1 << 7)
#define PAGE_AOR (1 << 9)
#define PAGE_COW (1 << 10)
#ifndef __i386__
#define PAGE_NO_EXEC (1ull << 63)
#endif
