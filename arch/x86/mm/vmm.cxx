/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 12 of 2021, at 14:54 BRT
 * Last edited on February 14 of 2021 at 21:32 BRT */

/* A few arch-specific functions (and macros) for the not so arch-specific generic VMM. */

#include <arch/mm.hxx>
#include <mm.hxx>
#include <textout.hxx>

#ifdef __i386__
#define LEVEL_COUNT 2
#define UNMAP_DEST_LEVEL (Huge ? 1 : 2)
#define MAP_DEST_LEVEL ((Flags & PAGE_HUGE) ? 1 : 2)

#define L1_ADDRESS 0xFFFFF000
#define L2_ADDRESS 0xFFC00000

#define GET_INDEXES() \
    UIntPtr l1e = (Address >> 22) & 0x3FF, l2e = (Address >> 12) & 0xFFFFF

#define DEFAULT_FLAGS ((Flags & MAP_AOR) ? PAGE_AOR : PAGE_PRESENT)
#define IS_EXEC(e) True
#define SET_EXEC(f)

#define TABLE_FLAGS (PAGE_PRESENT | PAGE_WRITE | (Virtual < 0xC0000000 ? PAGE_USER : 0))
#else
#define LEVEL_COUNT 4
#define UNMAP_DEST_LEVEL (Huge ? 3 : 4)
#define MAP_DEST_LEVEL ((Flags & PAGE_HUGE) ? 3 : 4)

#define L1_ADDRESS 0xFFFFFFFFFFFFF000
#define L2_ADDRESS 0xFFFFFFFFFFE00000
#define L3_ADDRESS 0xFFFFFFFFC0000000
#define L4_ADDRESS 0xFFFFFF8000000000

#define GET_INDEXES() \
    UIntPtr l1e = (Address >> 39) & 0x1FF, l2e = (Address >> 30) & 0x3FFFF, l3e = (Address >> 21) & 0x7FFFFFF, \
            l4e = (Address >> 12) & 0xFFFFFFFFF

#define TABLE_FLAGS (PAGE_PRESENT | PAGE_WRITE | (Virtual < 0xFFFF800000000000 ? PAGE_USER : 0))
#endif

static inline Void UpdateTLB(UIntPtr Address) {
    asm volatile("invlpg (%0)" :: "r"(Address) : "memory");
}

#include "../../vmm.cxx"

Void VirtMem::Initialize(BootInfo&) { }
