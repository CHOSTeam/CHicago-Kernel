/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 14 of 2021, at 16:13 BRT
 * Last edited on February 14 of 2021 at 21:27 BRT */

#include <arch/mm.hxx>
#include <mm.hxx>
#include <textout.hxx>

/* Those first defs are like the defs for PML4 on AMD64. */

#define LEVEL_COUNT 4
#define UNMAP_DEST_LEVEL (Huge ? 3 : 4)
#define MAP_DEST_LEVEL ((Flags & PAGE_TABLE) ? 4 : 3)

#define L1_ADDRESS (high ? 0xFFFFFFFFFFFFF000 : 0xFFFFFFFFF000)
#define L2_ADDRESS (high ? 0xFFFFFFFFFFE00000 : 0xFFFFFFE00000)
#define L3_ADDRESS (high ? 0xFFFFFFFFC0000000 : 0xFFFFC0000000)
#define L4_ADDRESS (high ? 0xFFFFFF8000000000 : 0xFF8000000000)

#define GET_INDEXES() \
    Boolean high = Address >= 0xFFFF000000000000; \
    UIntPtr l1e = (Address >> 39) & 0x1FF, l2e = (Address >> 30) & 0x3FFFF, l3e = (Address >> 21) & 0x7FFFFFF, \
            l4e = (Address >> 12) & 0xFFFFFFFFF

#define DEFAULT_FLAGS (((Flags & MAP_AOR) ? PAGE_AOR : PAGE_PRESENT) | PAGE_TABLE | PAGE_INNER | PAGE_ACCESS | \
                                                                       PAGE_READ_ONLY | PAGE_TABLE)
#define IS_WRITE(e) (!(e & PAGE_READ_ONLY))
#define SET_WRITE(f) f &= ~PAGE_READ_ONLY
#define SET_COW(f) f |= PAGE_COW
#define IS_HUGE(e) (!(e & PAGE_TABLE))
#define SET_HUGE(f) f &= ~PAGE_TABLE

#define TABLE_FLAGS (PAGE_PRESENT | PAGE_TABLE | PAGE_INNER | PAGE_ACCESS)

static inline Void UpdateTLB(UIntPtr Address) {
    asm volatile("dsb ishst; tlbi vaae1is, %0; dsb ish; isb" :: "r"((Address >> 12) & 0xFFFFFFFFFFF) : "memory");
}

#include "../../vmm.cxx"

Void VirtMem::Initialize(BootInfo&) { }
