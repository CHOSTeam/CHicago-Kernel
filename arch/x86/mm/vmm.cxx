/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 12 of 2021, at 14:54 BRT
 * Last edited on February 14 of 2021 at 19:37 BRT */

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

static inline Int8 CheckEntry(UIntPtr Entry, UIntPtr) {
    return !(Entry & PAGE_PRESENT) ? -1 : ((Entry & PAGE_HUGE) ? -2 : 0);
}

static inline UIntPtr GetOffset(UIntPtr Address, UIntPtr Level) {
#ifdef __i386__
    return Address & (Level == 1 ? PAGE_MASK : HUGE_PAGE_MASK);
#else
    return Address & (Level == 1 ? PAGE_MASK : (Level == 2 ? HUGE_PAGE_MASK : 0x3FFFFFFF));
#endif
}

static inline UInt32 ExtractFlags(UIntPtr Entry) {
    /* Both x86 and amd64 page flags are almost the same, but amd64 has one extra flag (NO_EXEC). */

#ifdef __i386__
    UInt32 ret = MAP_READ | MAP_KERNEL | MAP_EXEC;
#else
    UInt32 ret = MAP_READ | MAP_KERNEL;
#endif

    if (Entry & PAGE_WRITE) {
        ret |= MAP_WRITE;
    }

    if (Entry & PAGE_USER) {
        ret |= MAP_USER;
    }

    /* We don't need to distinguish between 2MiB/4MiB and 1GiB huge pages here, though VirtMem::Map only accepts 2MiB/
     * 4MiB huge pages. */

    if (Entry & PAGE_HUGE) {
        ret |= MAP_HUGE;
    }

    if (Entry & PAGE_COW) {
        ret |= MAP_COW;
    }

#ifndef __i386__
    if (!(Entry & PAGE_NO_EXEC)) {
        ret |= MAP_EXEC;
    }
#endif

    return ret;
}

static inline UInt32 FromFlags(UInt32 Flags) {
    /* Inverse of what we do on ExtractFlags lol (remembering that PAGE_COW and PAGE_WRITE can't be set at the same
     * time). */

    UInt32 ret = (Flags & MAP_AOR) ? PAGE_AOR : PAGE_PRESENT;

    if (Flags & MAP_COW) {
        ret |= PAGE_COW;
    } else if (Flags & MAP_WRITE) {
        ret |= PAGE_WRITE;
    }

    if (Flags & MAP_USER) {
        ret |= PAGE_USER;
    }

    if (Flags & MAP_HUGE) {
        ret |= PAGE_HUGE;
    }

#ifndef __i386__
    if (!(Flags & MAP_EXEC)) {
        ret |= PAGE_NO_EXEC;
    }
#endif

    return ret;
}

#include "../../vmm.cxx"

Void VirtMem::Initialize(BootInfo&) { }
