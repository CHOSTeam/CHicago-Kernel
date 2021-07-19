/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 15 of 2021, at 23:28 BRT
 * Last edited on July 18 of 2021, at 23:58 BRT */

static Status MoveInto(UIntPtr Virtual, UIntPtr &CurLevel, UIntPtr DestLevel, Boolean Allocate = False) {
    /* This works in a similar way to MoveInto from the bootloader, but as we expect to use recursive paging, we just
     * need the destination level and if we should allocate all the missing levels or error out on them. */

    UInt64 phys;
    Status status;

    for (; CurLevel < DestLevel; CurLevel++) {
        if (MMU_SKIP_LEVEL(CurLevel)) continue;

        auto cur = reinterpret_cast<MMU_TYPE*>(MMU_INDEX(Virtual, CurLevel));

        if (!MMU_IS_PRESENT(*cur) && Allocate) {
            if ((status = PhysMem::Reference(0, 1, phys)) != Status::Success) return status;
            *cur = MMU_MAKE_TABLE(Virtual, phys, i);
            SetMemory(reinterpret_cast<Void*>(MMU_INDEX(Virtual, CurLevel + 1) & ~PAGE_MASK), 0, PAGE_SIZE);
        } else if (!MMU_IS_PRESENT(*cur)) return Status::NotMapped;
        else if (MMU_IS_HUGE(*cur)) return Status::AlreadyMapped;
    }

    return Status::Success;
}

Status VirtMem::Query(UIntPtr Virtual, UInt64 &Physical, UInt32 &Flags) {
    /* MoveInto will return AlreadyMapped for huge pages, so we don't need to worry about them. */

    UIntPtr lvl = 0;
    Status status = MoveInto(Virtual, lvl, MMU_DEST_LEVEL(False) + 1);
    if (status != Status::AlreadyMapped && status != Status::Success) return status;

    MMU_TYPE ent = *reinterpret_cast<MMU_TYPE*>(MMU_INDEX(Virtual, lvl));

    Flags = MAP_READ | MAP_KERNEL;
    Physical = (ent & ~PAGE_MASK) | (Virtual & MMU_ENTRY_MASK(lvl));

    if (MMU_IS_WRITE(ent)) Flags |= MAP_WRITE;
    if (MMU_IS_HUGE(ent)) Flags |= MAP_HUGE;
    if (MMU_IS_USER(ent)) Flags |= MAP_USER;
    if (MMU_IS_EXEC(ent)) Flags |= MAP_EXEC;

    return Status::Success;
}

Status VirtMem::Map(UIntPtr Virtual, UInt64 Physical, UIntPtr Size, UInt32 Flags) {
    /* What we do here is very similar to MapAddress (also from the bootloader), but instead of determining if we
     * should use huge pages (and when to start/end using them), the caller needs to do that (and pass an aligned size+
     * virtual address+physical address). */

    if (((Flags & MAP_HUGE) && ((Virtual & HUGE_PAGE_MASK) || (Physical & HUGE_PAGE_MASK) || (Size & HUGE_PAGE_MASK)))
        || (!(Flags & MAP_HUGE) && ((Virtual & PAGE_MASK) || (Physical & PAGE_MASK) || (Size & PAGE_MASK))))
        return Status::InvalidArg;

    UIntPtr lvl = 0, dlvl = MMU_DEST_LEVEL(Flags & MAP_HUGE);
    UInt64 mask = (Flags & MAP_HUGE) ? HUGE_PAGE_MASK : PAGE_MASK;
    UIntPtr size = (Flags & MAP_HUGE) ? HUGE_PAGE_SIZE : PAGE_SIZE;
    UInt32 flags = MMU_BASE_FLAGS | MMU_WRITE_FLAG(Flags & MAP_WRITE) | MMU_HUGE_FLAG(Flags & MAP_HUGE) |
                   MMU_USER_FLAG(Flags & MAP_USER) | MMU_EXEC_FLAG(Flags & MAP_EXEC);
    Status status = MoveInto(Virtual, lvl, dlvl, True);

    if (status != Status::Success) return status;

    UIntPtr ent = MMU_INDEX(Virtual, lvl), old = ent;

    for (; Size; Size -= size, Virtual += size, Physical += size, old = ent, ent += sizeof(MMU_TYPE)) {
        if ((ent & ~PAGE_MASK) != (old & ~PAGE_MASK) &&
            (lvl = 0, status = MoveInto(Virtual, lvl, dlvl, True)) != Status::Success) return status;
        else if (MMU_IS_PRESENT(*reinterpret_cast<MMU_TYPE*>(ent))) return Status::AlreadyMapped;
        *reinterpret_cast<MMU_TYPE*>(ent) = (Physical & ~mask) | flags;
    }

    return Status::Success;
}

Status VirtMem::Unmap(UIntPtr Virtual, UIntPtr Size, Boolean Huge) {
    /* Unmapping needs to invalidate the TLB/paging structures (and that uses an arch-specific macro), but other than
     * that, we just need to unset the present bit/whatever indicates that we have a valid page. */

    if ((Huge && (Virtual & HUGE_PAGE_MASK)) || (!Huge && (Virtual & PAGE_MASK))) return Status::InvalidArg;

    UInt64 size = Huge ? HUGE_PAGE_SIZE : PAGE_SIZE;
    UIntPtr lvl = 0, dlvl = MMU_DEST_LEVEL(Huge);
    Status status = MoveInto(Virtual, lvl, dlvl);

    if (status != Status::Success) return status;

    UIntPtr ent = MMU_INDEX(Virtual, lvl), old = ent;

    for (UIntPtr i = 0; i < Size; i += size, old = ent, ent += sizeof(MMU_TYPE)) {
        if ((ent & ~PAGE_MASK) != (old & ~PAGE_MASK) &&
            (lvl = 0, status = MoveInto(Virtual + i, lvl, dlvl)) != Status::Success) return status;
        else if (!MMU_IS_PRESENT(*reinterpret_cast<MMU_TYPE*>(ent))) return Status::NotMapped;
        MMU_UNSET_PRESENT(*reinterpret_cast<MMU_TYPE*>(ent));
        MMU_UPDATE(Virtual + i);
    }

    MMU_SHOOTDOWN(Virtual, Size);

    return Status::Success;
}

Void VirtMem::Initialize(const BootInfo &Info) {
    /* Generic initialization function: We need to unmap the EFI jump function, and we need pre-alloc the first level
     * of the heap region (and we can't fail, if we do fail, panic, as the rest of the OS depends on us). Also, we
     * expect that adding HUGE_PAGE_MASK will be enough to make sure that we don't collide with some huge mapping from
     * the kernel/bootloader. */

    UIntPtr start = (Info.KernelEnd + VIRT_GROUP_RANGE - 1) & ~(VIRT_GROUP_RANGE - 1);

    Unmap(Info.EfiTempAddress & ~PAGE_MASK, PAGE_SIZE);

    Start = Current = start;
    End = HEAP_END & ~(VIRT_GROUP_RANGE - 1);

    if (!MMU_SKIP_LEVEL(0)) {
        for (; start < End; start += MMU_ENTRY_SIZE(0)) {
            UIntPtr lvl = 0;
            ASSERT(MoveInto(start, lvl, 1, True) == Status::Success);
        }
    }

    Debug.Write("the kernel virtual address allocator starts at 0x{:0*:16} and ends at 0x{:0*:16}\n", Start, End);
}
