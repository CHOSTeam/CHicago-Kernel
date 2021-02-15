/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 12 of 2021, at 14:52 BRT
 * Last edited on February 15 of 2021 at 00:01 BRT */

#include <mm.hxx>
#include <panic.hxx>

/* A bit of a not so common thing to do, but we actually expect whichever arch is going to use us, to #include this
 * file, while defining some arch specific static inline functions and some macros before. The FULL_CHECK/LAST_CHECK
 * macros are just so that we don't have as much repetition on the CheckDirectory function. */

#define FULL_CHECK(a, i) \
        if ((ret = CheckLevel((a), (i), Entry, Clean)) < 0) { \
            return ret; \
        } \
        \
        Clean = False; \
        Level++

#define LAST_CHECK(a, i) \
        ret = CheckLevel((a), (i), Entry, Clean); \
        break

/* Get offset for two of the valid level counts (may add a third one if we add L5 support to some arch). */

#if !defined(CUSTOM_VMM_GET_OFFSET) && LEVEL_COUNT == 2
static inline UIntPtr GetOffset(UIntPtr Address, UIntPtr Level) {
    return Address & (Level == 1 ? PAGE_MASK : HUGE_PAGE_MASK);
}
#elif !defined(CUSTOM_VMM_GET_OFFSET) && LEVEL_COUNT == 4
static inline UIntPtr GetOffset(UIntPtr Address, UIntPtr Level) {
    return Address & (Level == 1 ? PAGE_MASK : (Level == 2 ? HUGE_PAGE_MASK : 0x3FFFFFFF));
}
#endif

/* If the user just defined that we can use the PAGE_* macros for the flags, we can try using some extra generic
 * functions (other than the ones that will be defined afterwards). */

#ifndef CUSTOM_VMM_FLAGS
#ifndef IS_PRESENT
#define IS_PRESENT(e) (e & PAGE_PRESENT)
#endif

#ifndef IS_WRITE
#define IS_WRITE(e) (e & PAGE_WRITE)
#define SET_WRITE(f) f |= PAGE_WRITE
#endif

#ifndef IS_USER
#define IS_USER(e) (e & PAGE_USER)
#define SET_USER(f) f |= PAGE_USER
#endif

#ifndef IS_HUGE
#define IS_HUGE(e) (e & PAGE_HUGE)
#define SET_HUGE(f) f |= PAGE_HUGE
#endif

#ifndef IS_AOR
#define IS_AOR(e) (e & PAGE_AOR)
#endif

#ifndef IS_COW
#define IS_COW(e) (e & PAGE_COW)
#endif

#ifndef SET_COW
#define SET_COW(f) \
    f |= PAGE_COW; \
    f &= ~PAGE_WRITE
#endif

/* A bit out of the pattern of the others, but most archs will probably have a NX (never execute) bit instead of some
 * "allow-execute" bit. */

#ifndef IS_EXEC
#define IS_EXEC(e) (!(e & PAGE_NO_EXEC))
#define SET_EXEC(f) f &= ~PAGE_NO_EXEC
#endif

#ifndef DEFAULT_FLAGS
#define DEFAULT_FLAGS (((Flags & MAP_AOR) ? PAGE_AOR : PAGE_PRESENT) | PAGE_NO_EXEC)
#endif

static inline Int8 CheckEntry(UIntPtr Entry) {
    return !IS_PRESENT(Entry) ? -1 : (IS_HUGE(Entry) ? -2 : 0);
}

static inline UInt32 ToFlags(UIntPtr Entry) {
    /* Generic version of the ToFlags function. */

    UInt32 ret = MAP_READ | MAP_KERNEL;

    if (IS_WRITE(Entry)) {
        ret |= MAP_WRITE;
    }

    if (IS_USER(Entry)) {
        ret |= MAP_USER;
    }

    /* Let's not distinguish between differently sized huge pages here. */

    if (IS_HUGE(Entry)) {
        ret |= MAP_HUGE;
    }

    if (IS_EXEC(Entry)) {
        ret |= MAP_EXEC;
    }

    return ret;
}

static inline UInt32 FromFlags(UInt32 Flags) {
    /* Inverse of the FromFlags function. */

    UInt32 ret = DEFAULT_FLAGS;

    if (Flags & MAP_COW) {
        SET_COW(ret);
    } else if (Flags & MAP_WRITE) {
        SET_WRITE(ret);
    }

    if (Flags & MAP_USER) {
        SET_USER(ret);
    }

    if (Flags & MAP_HUGE) {
        SET_HUGE(ret);
    }

    if (Flags & MAP_EXEC) {
        SET_EXEC(ret);
    }

    return ret;
}
#endif

static Int8 CheckLevel(UIntPtr Address, UIntPtr Index, UIntPtr *&Entry, Boolean Clean) {
    /* If asked for, clean the table entry. */

    if (Clean) {
        UIntPtr start = (Address + (Index * sizeof(UIntPtr))) & ~PAGE_MASK;
        UpdateTLB(start);
        SetMemory(reinterpret_cast<Void*>(start), 0, PAGE_SIZE);
        return -1;
    }

    /* Now, get/save the entry and check if it is present/huge. */

    return CheckEntry(*(Entry = &reinterpret_cast<UIntPtr*>(Address)[Index]));
}

static Int8 CheckDirectory(UIntPtr Address, UIntPtr *&Entry, UIntPtr &Level, Boolean Clean = False) {
    /* We need to check each level of the directory, the amount of levels in each arch will depend (x86 has 2, amd64
     * and arm64 have 4), and remember to break early if we find some unallocated entry, or a huge entry. Yes, I know
     * that this code is probably very ugly/unreadable, but well, it works. */

    Int8 ret = -1;

    GET_INDEXES();

    switch (Level) {
    case 1: {
#if LEVEL_COUNT > 1
        FULL_CHECK(L1_ADDRESS, l1e);
    }
    case 2: {
#if LEVEL_COUNT > 2
        FULL_CHECK(L2_ADDRESS, l2e);
    }
    case 3: {
#if LEVEL_COUNT > 3
        FULL_CHECK(L3_ADDRESS, l3e);
    }
    case 4: {
#if LEVEL_COUNT > 4
        FULL_CHECK(L4_ADDRESS, l4e);
    }
    case 5: {
        LAST_CHECK(L5_ADDRESS, l5e);
    }
#else
        LAST_CHECK(L4_ADDRESS, l4e);
    }
#endif
#else
        LAST_CHECK(L3_ADDRESS, l3e);
    }
#endif
#else
        LAST_CHECK(L2_ADDRESS, l2e);
    }
#endif
#else
        LAST_CHECK(L1_ADDRESS, l1e);
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

    Int8 res;
    Status status;
    UIntPtr *ent, phys, lvl = 1, dlvl = MAP_DEST_LEVEL;

    while ((res = CheckDirectory(Virtual, ent, lvl)) == -1) {
        /* The entry doesn't exist, and so we need to allocate this level (alloc a physical address, set it up, and
         * call CheckDirectory again). */

        if (lvl >= dlvl) {
            break;
        } else if ((status = PhysMem::ReferenceSingle(0, phys)) != Status::Success) {
            return status;
        }

        lvl++;
        *ent = phys | TABLE_FLAGS(Virtual, IS_USER_TABLE(Virtual));
        CheckDirectory(Virtual, ent, lvl, True);
    }

    /* If the address is already mapped, just error out (let's not even try remapping it). */

    if (res != -1 || lvl > dlvl) {
        return Status::AlreadyMapped;
    }

    *ent = Physical | Flags;
    UpdateTLB(Virtual);

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

    UIntPtr *ent, lvl = 1, dlvl = UNMAP_DEST_LEVEL;

    if (CheckDirectory(Virtual, ent, lvl) == -1) {
        return Status::NotMapped;
    } else if (lvl != dlvl) {
        return Status::InvalidArg;
    }

    /* Also, not mentioned before, but we expect PAGE_PRESENT to be defined, and to be whatever flag we need to set or
     * unset to make one address mapped/unmapped. */

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

#ifndef CUSTOM_VMM_INIT
Void VirtMem::Initialize(BootInfo &Info) {
    /* Generic initialization function: We need to unmap the EFI jump function, and we need pre-alloc the first level of
     * the heap region (and we can't fail, if we do fail, panic, as the rest of the OS depends on us), and call the heap
     * init function. Also, we expect that adding HUGE_PAGE_MASK will be enough to make sure that we don't collide with
     * some huge mapping from the kernel/bootloader. */

    UIntPtr start = (Info.KernelEnd + HUGE_PAGE_MASK) & ~HUGE_PAGE_MASK;

    Unmap(Info.EfiTempAddress & ~PAGE_MASK, PAGE_SIZE);

    for (UIntPtr i = start & ~(LAST_LEVEL_SIZE - 1); i < HEAP_END; i += LAST_LEVEL_SIZE) {
        UIntPtr *ent, lvl = 1, phys;

        if (CheckDirectory(i, ent, lvl) != -1 || lvl != 1) {
            continue;
        }

        ASSERT(PhysMem::ReferenceSingle(0, phys) == Status::Success);

        lvl++;
        *ent = phys | TABLE_FLAGS(i, False);
        CheckDirectory(i, ent, lvl, True);
    }

    Heap::Initialize(start, HEAP_END);
    Debug.Write("the kernel heap starts at " UINTPTR_MAX_HEX " and ends at " UINTPTR_MAX_HEX "\n", start, HEAP_END);
}
#endif
