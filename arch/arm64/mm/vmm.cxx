/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 14 of 2021, at 16:13 BRT
 * Last edited on February 14 of 2021 at 19:18 BRT */

#include <arch/mm.hxx>
#include <mm.hxx>
#include <textout.hxx>

/* Those first defs are like the defs for PML4 on AMD64. */

#define LEVEL_COUNT 4
#define UNMAP_DEST_LEVEL (Huge ? 3 : 4)
#define MAP_DEST_LEVEL ((Flags & PAGE_TABLE) ? 4 : 3)

#define L1_ADDRESS (high ? 0xFFFFFFFFFFFFF000 : 0xFFFFFF7FBFDFE000)
#define L2_ADDRESS (high ? 0xFFFFFFFFFFE00000 : 0xFFFFFF7FBFC00000)
#define L3_ADDRESS (high ? 0xFFFFFFFFC0000000 : 0xFFFFFF7F80000000)
#define L4_ADDRESS (high ? 0xFFFFFF8000000000 : 0xFFFFFF0000000000)

#define GET_INDEXES() \
    Boolean high = Address >= 0xFFFF000000000000; \
    UIntPtr l1e = (Address >> 39) & 0x1FF, l2e = (Address >> 30) & 0x3FFFF, l3e = (Address >> 21) & 0x7FFFFFF, \
            l4e = (Address >> 12) & 0xFFFFFFFFF

#define TABLE_FLAGS (PAGE_PRESENT | PAGE_TABLE | PAGE_INNER | PAGE_ACCESS)

static inline Void UpdateTLB(UIntPtr Address) {
    asm volatile("dsb ishst; tlbi vaae1is, %0; dsb ish; isb" :: "r"((Address >> 12) & 0xFFFFFFFFFFF) : "memory");
}

static inline Int8 CheckEntry(UIntPtr Entry, UIntPtr) {
    return !(Entry & PAGE_PRESENT) ? -1 : (!(Entry & PAGE_TABLE) ? -2 : 0);
}

static inline UIntPtr GetOffset(UIntPtr Address, UIntPtr Level) {
    return Address & (Level == 1 ? PAGE_MASK : (Level == 2 ? HUGE_PAGE_MASK : 0x3FFFFFFF));
}

static inline UInt32 ExtractFlags(UIntPtr Entry) {
    /* It's like extracting the amd64 flags, but there is no WRITE flag on arm64, but there is a READ-ONLY though. */

    UInt32 ret = MAP_READ | MAP_KERNEL;

    if (!(Entry & PAGE_READ_ONLY)) {
        ret |= MAP_WRITE;
    }

    if (Entry & PAGE_USER) {
        ret |= MAP_USER;
    }

    if (!(Entry & PAGE_TABLE)) {
        ret |= MAP_HUGE;
    }

    if (Entry & PAGE_COW) {
        ret |= MAP_COW;
    }

    if (!(Entry & PAGE_NO_EXEC)) {
        ret |= MAP_EXEC;
    }

    return ret;
}

static inline UInt32 FromFlags(UInt32 Flags) {
    /* And just like on amd64/x86 FromFlags, this does the inverse of what ExtractFlags does. */

    UInt32 ret = (Flags & MAP_AOR) ? PAGE_AOR : PAGE_PRESENT;

    if (Flags & MAP_COW) {
        ret |= PAGE_COW | PAGE_READ_ONLY;
    } else if (!(Flags & MAP_WRITE)) {
        ret |= PAGE_READ_ONLY;
    }

    if (Flags & MAP_USER) {
        ret |= PAGE_USER;
    }

    /* Also, here we have a bit to set if this ISN'T a huge page (instead of if it is, like on x86/amd64). */

    if (!(Flags & MAP_HUGE)) {
        ret |= PAGE_TABLE;
    }

    if (!(Flags & MAP_EXEC)) {
        ret |= PAGE_NO_EXEC;
    }

    return ret;
}

#include "../../vmm.cxx"

Void VirtMem::Initialize(BootInfo&) { }
