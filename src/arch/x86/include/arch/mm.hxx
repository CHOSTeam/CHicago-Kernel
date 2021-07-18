/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 03 of 2020, at 17:28 BRT
 * Last edited on July 18 of 2021, at 15:34 BRT */

#pragma once

#include <base/types.hxx>

#define PAGE_PRESENT (1 << 0)
#define PAGE_WRITE (1 << 1)
#define PAGE_USER (1 << 2)
#define PAGE_HUGE (1 << 7)
#define PAGE_AOR (1 << 9)
#define PAGE_COW (1 << 10)
#define PAGE_NO_EXEC (1ull << 63)
