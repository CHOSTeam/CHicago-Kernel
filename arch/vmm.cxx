/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 12 of 2021, at 14:52 BRT
 * Last edited on February 14 of 2021 at 19:37 BRT */

#include <mm.hxx>
#include <string.hxx>

/* A bit of a not so common thing to do, but we actually expect whichever arch is going to use us, to #include this
 * file, while defining some arch specific static inline functions and some macros before. The FULL_CHECK/LAST_CHECK
 * macros are just so that we don't have as much repetition on the CheckDirectory function. */

#define FULL_CHECK(a, i) \
        if ((ret = CheckLevel((a), (i), Level, Entry, Clean)) < 0) { \
            return ret; \
        } \
        \
        Clean = False; \
        Level++

#define LAST_CHECK(a, i) \
        ret = CheckLevel((a), (i), Level, Entry, Clean); \
        break

static Int8 CheckLevel(UIntPtr Address, UIntPtr Index, UIntPtr Level, UIntPtr *&Entry, Boolean Clean) {
    /* If asked for, clean the table entry. */

    if (Clean) {
        UpdateTLB(Address);
        SetMemory(reinterpret_cast<Void*>(Address), 0, PAGE_SIZE);
        return -1;
    }

    /* Now, get/save the entry and check if it is present/huge. */

    return CheckEntry(*(Entry = &reinterpret_cast<UIntPtr*>(Address)[Index]), Level);
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

Status VirtMem::GetPhys(UIntPtr Address, UIntPtr &Out) {
    /* We can use the CheckDirectory function, it will error out if the page is not mapped, or return the entry if it
     * is valid/is huge. */

    Int8 res;
    UIntPtr *ent, lvl = 1;

    if ((res = CheckDirectory(Address, ent, lvl)) == -1) {
        return Status::NotMapped;
    }

    return Out = (*ent & ~PAGE_MASK) | GetOffset(Address, lvl), Status::Success;
}

Status VirtMem::Query(UIntPtr Address, UInt32 &Out) {
    /* Use CheckDirectory again (stopping at the first unallocated/huge entry, or going until the last level). But
     * this time we don't care about the physical address, but about the flags (that btw we need to use an arch
     * specific function to extract). */

    Int8 res;
    UIntPtr *ent, lvl = 1;

    if ((res = CheckDirectory(Address, ent, lvl)) == -1) {
        return Status::NotMapped;
    }

    return Out = ExtractFlags(*ent), Status::Success;
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

        *ent = phys | TABLE_FLAGS;

        lvl++;
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

    Int8 res;
    UIntPtr *ent, lvl = 1, dlvl = UNMAP_DEST_LEVEL;

    if ((res = CheckDirectory(Virtual, ent, lvl)) == -1) {
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
