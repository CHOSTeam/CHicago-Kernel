/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 14 of 2021, at 16:39 BRT
 * Last edited on February 14 of 2021 at 18:24 BRT */

#pragma once

#include <types.hxx>

#define PAGE_PRESENT (1 << 0)
#define PAGE_TABLE (1 << 1)
#define PAGE_DEVICE (2 << 2)
#define PAGE_USER (1 << 6)
#define PAGE_READ_ONLY (1 << 7)
#define PAGE_OUTER (2 << 8)
#define PAGE_INNER (3 << 8)
#define PAGE_ACCESS (1 << 10) 
#define PAGE_NO_EXEC ((1ull << 53) | (1ull << 54))
#define PAGE_AOR (1ull << 55)
#define PAGE_COW (1ull << 56)
#define PAGE_AVAL3 (1ull << 57)
#define PAGE_AVAL4 (1ull << 58)
