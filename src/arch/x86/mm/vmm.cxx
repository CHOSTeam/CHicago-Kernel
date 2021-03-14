/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 12 of 2021, at 14:54 BRT
 * Last edited on March 14 of 2021, at 11:13 BRT */

#include <arch/mm.hxx>
#include <sys/mm.hxx>
#include <sys/panic.hxx>

using namespace CHicago;

/* Some convenience macros to extract the index that we need to pass to CheckLevel. */

#ifdef __i386__
#define L1_ADDRESS 0xFFFFF000
#define L2_ADDRESS 0xFFC00000
#define HEAP_END L2_ADDRESS

#define GET_INDEXES() \
    UIntPtr l1e = (Address >> 22) & 0x3FF, l2e = (Address >> 12) & 0xFFFFF
#else
#define L1_ADDRESS 0xFFFFFFFFFFFFF000
#define L2_ADDRESS 0xFFFFFFFFFFE00000
#define L3_ADDRESS 0xFFFFFFFFC0000000
#define L4_ADDRESS 0xFFFFFF8000000000
#define HEAP_END L4_ADDRESS

#define GET_INDEXES() \
    UIntPtr l1e = (Address >> 39) & 0x1FF, l2e = (Address >> 30) & 0x3FFFF, l3e = (Address >> 21) & 0x7FFFFFF, \
            l4e = (Address >> 12) & 0xFFFFFFFFF
#endif

/* The FULL_CHECK/LAST_CHECK macros are just so that we don't have as much repetition on the CheckDirectory function. */

#define FULL_CHECK(a, i) \
        if ((ret = CheckLevel((a), (i), Entry, Clean)) < 0) { \
            return ret; \
        } \
        \
        Level++

#define LAST_CHECK(a, i) \
        return CheckLevel((a), (i), Entry, Clean)

static inline Void UpdateTLB(UIntPtr Address) {
    asm volatile("invlpg (%0)" :: "r"(Address) : "memory");
}

static inline UIntPtr GetOffset(UIntPtr Address, UIntPtr Level) {
#ifdef __i386__
    return Address & (Level == 2 ? PAGE_MASK : HUGE_PAGE_MASK);
#else
    return Address & (Level == 4 ? PAGE_MASK : (Level == 3 ? HUGE_PAGE_MASK : 0x3FFFFFFF));
#endif
}

static inline UInt32 ToFlags(UIntPtr Entry) {
    UInt32 ret = MAP_READ | MAP_KERNEL;

    /* x86 is always MAP_EXEC (as we don't support PAE yet). */

#ifdef __i386__
    ret |= MAP_EXEC;
#endif

    if (Entry & PAGE_WRITE) {
        ret |= MAP_WRITE;
    }

    if (Entry & PAGE_USER) {
        ret |= MAP_USER;
    }

    /* Let's not distinguish between differently sized huge pages here. */

    if (Entry & PAGE_HUGE) {
        ret |= MAP_HUGE;
    }

#ifndef __i386__
    if (!(Entry & PAGE_NO_EXEC)) {
        ret |= MAP_EXEC;
    }
#endif

    return ret;
}

static inline UInt32 FromFlags(UInt32 Flags) {
    /* Inverse of the FromFlags function. */

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

static IntPtr CheckLevel(UIntPtr Address, UIntPtr Index, UIntPtr *&Entry, Boolean Clean) {
    /* If asked for, clean the table entry. Also, we don't need to update the TLB here (we actually only need to flush/
     * update it if we unmap something). */

    if (Clean) {
        SetMemory(reinterpret_cast<UIntPtr*>(Address) + Index, 0, PAGE_SIZE);
        return -1;
    }

    /* Now, get/save the entry and check if it is present/huge. */

    return Entry = reinterpret_cast<UIntPtr*>(Address) + Index, !(*Entry & PAGE_PRESENT) ? -1 :
                                                                ((*Entry & PAGE_HUGE) ? -2 : 0);
}

static IntPtr CheckDirectory(UIntPtr Address, UIntPtr *&Entry, UIntPtr &Level, Boolean Clean = False) {
    /* We need to check each level of the directory here, remembering that while amd64 has 4 levels (and supports 5),
     * x86 only has 2. */

    IntPtr ret = 0;

    GET_INDEXES();

    switch (Level) {
    case 1: {
        FULL_CHECK(L1_ADDRESS, l1e);
    }
    case 2: {
#ifdef __i386__
        LAST_CHECK(L2_ADDRESS, l2e);
    }
#else
        FULL_CHECK(L2_ADDRESS, l2e);
    }
    case 3: {
        FULL_CHECK(L3_ADDRESS, l3e);
    }
    case 4: {
        LAST_CHECK(L4_ADDRESS, l4e);
    }
#endif
    }

    return ret;
}

Status VirtMem::Query(UIntPtr Virtual, UIntPtr &Physical, UInt32 &Flags) {
    /* Use CheckDirectory (stopping at the first unallocated/huge entry, or going until the last level). Extract both
     * the physical address and the flags (at the same time). */

    UIntPtr *ent, lvl = 1;

    if (CheckDirectory(Virtual, ent, lvl) == -1) {
        return Status::NotMapped;
    }

    return Physical = (*ent & ~PAGE_MASK) | GetOffset(Virtual, lvl), Flags = ToFlags(*ent), Status::Success;
}

static Status DoMap(UIntPtr Virtual, UIntPtr Physical, UInt32 Flags) {
    /* The caller should handle error out if something is not aligned, and should also convert the map flags into page
     * flags, so we don't have to do those things here.
     * Let's just recursively allocate all levels, until we reach the last level (or the huge level). */

    IntPtr res;
    Status status;
    UIntPtr *ent = Null, phys, lvl = 1,
#ifdef __i386__
            dlvl = (Flags & PAGE_HUGE) ? 1 : 2;
#else
            dlvl = (Flags & PAGE_HUGE) ? 3 : 4;
#endif

    while ((res = CheckDirectory(Virtual, ent, lvl)) == -1) {
        /* The entry doesn't exist, and so we need to allocate this level (alloc a physical address, set it up, and
         * call CheckDirectory again). */

        if (lvl >= dlvl) {
            break;
        } else if ((status = PhysMem::ReferenceSingle(0, phys)) != Status::Success) {
            return status;
        }

        lvl++;
        *ent = phys | PAGE_PRESENT | PAGE_WRITE |
#ifdef __i386__
               (Virtual < 0xC0000000 ? PAGE_USER : 0);
#else
               (Virtual < 0xFFFF800000000000 ? PAGE_USER : 0);
#endif

        CheckDirectory(Virtual, ent, lvl, True);
    }

    /* If the address is already mapped, just error out (let's not even try remapping it). */

    if (res != -1 || lvl > dlvl) {
        return Status::AlreadyMapped;
    }

    *ent = Physical | Flags;

    return Status::Success;
}

Status VirtMem::Map(UIntPtr Virtual, UIntPtr Physical, UIntPtr Size, UInt32 Flags) {
    /* Check if everything is properly aligned (including the size). */

    if (((Flags & MAP_HUGE) && ((Virtual & HUGE_PAGE_MASK) || (Physical & HUGE_PAGE_MASK) || (Size & HUGE_PAGE_MASK)))
        || (!(Flags & MAP_HUGE) && ((Virtual & PAGE_MASK) || (Physical & PAGE_MASK) || (Size & PAGE_MASK)))) {
        return Status::InvalidArg;
    }

    /* Now we can just iterate over the size, while mapping everything (and breaking out if something goes wrong). */

    Status status;

    for (UIntPtr i = 0; i < Size; i += (Flags & MAP_HUGE) ? HUGE_PAGE_SIZE : PAGE_SIZE) {
        if ((status = DoMap(Virtual + i, Physical + i, FromFlags(Flags))) != Status::Success) {
            return status;
        }
    }

    return Status::Success;
}

static Status DoUnmap(UIntPtr Virtual, Boolean Huge) {
    /* Just go though the directory levels, searching for what we want to unmap (no need to check for alignment, as
     * the caller should have done it). */

    UIntPtr *ent, lvl = 1,
#ifdef __i386__
            dlvl = Huge ? 1 : 2;
#else
            dlvl = Huge ? 3 : 4;
#endif

    if (CheckDirectory(Virtual, ent, lvl) == -1) {
        return Status::NotMapped;
    } else if (lvl != dlvl) {
        return Status::InvalidArg;
    }

    *ent &= ~PAGE_PRESENT;
    UpdateTLB(Virtual);

    return Status::Success;
}

Status VirtMem::Unmap(UIntPtr Virtual, UIntPtr Size, Boolean Huge) {
    /* Check for the right alignment, and redirect to Unmap(UIntPtr, Boolean), while iterating over the size. */

    if ((Huge && (Virtual & HUGE_PAGE_MASK)) || (!Huge && (Virtual & PAGE_MASK))) {
        return Status::InvalidArg;
    }

    Status status;

    for (UIntPtr i = 0; i < Size; i += Huge ? HUGE_PAGE_SIZE : PAGE_SIZE) {
        if ((status = DoUnmap(Virtual + i, Huge)) != Status::Success) {
            return status;
        }
    }

    return Status::Success;
}

Void VirtMem::Initialize(BootInfo &Info) {
    /* Generic initialization function: We need to unmap the EFI jump function, and we need pre-alloc the first level of
     * the heap region (and we can't fail, if we do fail, panic, as the rest of the OS depends on us), and call the heap
     * init function. Also, we expect that adding HUGE_PAGE_MASK will be enough to make sure that we don't collide with
     * some huge mapping from the kernel/bootloader. */

    UIntPtr start = (Info.KernelEnd + HUGE_PAGE_MASK) & ~HUGE_PAGE_MASK;

    Unmap(Info.EfiTempAddress & ~PAGE_MASK, PAGE_SIZE);

#ifdef __i386__
    for (UIntPtr i = start & ~HUGE_PAGE_MASK; i < HEAP_END; i += HUGE_PAGE_SIZE) {
#else
    for (UIntPtr i = start & ~0x7FFFFFFFFF; i < HEAP_END; i += 0x8000000000) {
#endif
        UIntPtr *ent, lvl = 1, phys;

        if (CheckDirectory(i, ent, lvl) != -1 || lvl != 1) {
            continue;
        }

        ASSERT(PhysMem::ReferenceSingle(0, phys) == Status::Success);

        lvl++;
        *ent = phys | PAGE_PRESENT | PAGE_WRITE;
        CheckDirectory(i, ent, lvl, True);
    }

    Heap::Initialize(start, HEAP_END);
    Debug.Write("the kernel heap starts at 0x{:0*:16} and ends at 0x{:0*:16}\n", start, HEAP_END);
}
