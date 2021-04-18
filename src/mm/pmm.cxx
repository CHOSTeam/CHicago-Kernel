/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 01 of 2020, at 19:47 BRT
 * Last edited on April 14 of 2021, at 18:05 BRT */

#include <sys/mm.hxx>
#include <sys/panic.hxx>
#include <util/bitop.hxx>

using namespace CHicago;

/* All of the static private variables. */

UIntPtr PhysMem::KernelStart = 0, PhysMem::KernelEnd = 0, PhysMem::RegionCount = 0,
        PhysMem::MinAddress = 0, PhysMem::MaxAddress = 0, PhysMem::MaxBytes = 0, PhysMem::UsedBytes = 0;
PhysMem::Region *PhysMem::Regions = Null;
UInt8 *PhysMem::References = Null;
Boolean PhysMem::Initialized = False;

Void PhysMem::Initialize(BootInfo &Info) {
    /* This function should only be called once by the kernel entry. It is responsible for initializing the physical
     * memory manager (allocate/deallocate physical memory, and manage how many references the pages have around all
     * the tasks/CPUs/whatever). */

    ASSERT(!Initialized);
    ASSERT(Info.KernelStart && Info.KernelEnd);
    ASSERT(Info.MemoryMap.Count && Info.MemoryMap.Entries != Null);

    /* First, setup some of the basic fields, including the InInit field, that indicates that the we were called, but
     * the FinishInitialization functions isn't haven't been called (that is, we can call InitializeRegion). */

    auto start = reinterpret_cast<UInt8*>(Info.RegionsStart);

    ASSERT(start != Null);

    if (Info.MaxPhysicalAddress - Info.MinPhysicalAddress + PHYS_REGION_MASK < Info.MaxPhysicalAddress -
                                                                               Info.MinPhysicalAddress) {
        RegionCount = (UINTPTR_MAX >> PHYS_REGION_SHIFT) + 1;
    } else {
        RegionCount = ((Info.MaxPhysicalAddress - Info.MinPhysicalAddress + PHYS_REGION_MASK) & ~PHYS_REGION_MASK)
                                                                                              >> PHYS_REGION_SHIFT;
    }

    KernelStart = Info.KernelStart;
    KernelEnd = Info.KernelEnd;
    MinAddress = Info.MinPhysicalAddress;
    MaxAddress = Info.MaxPhysicalAddress;
    MaxBytes = Info.PhysicalMemorySize;
    UsedBytes = MaxBytes;

    Debug.Write("the kernel starts at 0x{:0*:16}, and ends at 0x{:0*:16}\n"
                "minimum physical address is 0x{:0*:16}, and max is 0x{:0*:16}\n"
                "physical memory size is 0x{:0:16}\n", KernelStart, KernelEnd, MinAddress, MaxAddress, MaxBytes);

    /* Now, we need to initialize the region list, and the reference list, we need to do this before setting the
     * kernel end, as we're going to put those lists after the current kernel end. There is one very important thing,
     * we expect the loader to already have mapped those lists onto virtual memory. */

    Regions = reinterpret_cast<PhysMem::Region*>(start);
    References = &start[RegionCount * sizeof(PhysMem::Region)];

    /* Now, let's clear the region list and the reference list (set all the entries on the region list to be already
     * used, and all the entries on the references list to have 0 references). */

    for (UIntPtr i = 0; i < RegionCount; i++) {
        Regions[i].Free = 0;
        Regions[i].Used = PHYS_REGION_PSIZE;
        SetMemory(Regions[i].Pages, 0xFF, sizeof(Regions[i].Pages));
    }

    SetMemory(References, 0, (Info.MaxPhysicalAddress - Info.MinPhysicalAddress) >> PAGE_SHIFT);

    /* Now using the boot memory map, we can free the free (duh) regions (those entries will be marked as
     * BOOT_INFO_MEM_FREE). Also, if we find some 0-base entry after the first entry, it is probably some entry that
     * goes beyond the UIntPtr size (like some PAE entry on x86), and we don't support those (yet). */

    for (UIntPtr i = 0; i < Info.MemoryMap.Count; i++) {
        BootInfoMemMap &ent = Info.MemoryMap.Entries[i];

        if (i && !ent.Base) {
            Debug.Write("memory map entry no. {}, possible extended entry, not printing info\n", i);
            continue;
        }

        Debug.Write("memory map entry no. {}, base = 0x{:0*:16}, size = 0x{:0:16}, type = {}\n", i, ent.Base,
                    ent.Count << 12, ent.Type);

        if (ent.Type == 0x06 && ent.Base) FreeInt(ent.Base, ent.Count);
        else if (ent.Type == 0x06 && ent.Count > 1) FreeInt(ent.Base + PAGE_SIZE, ent.Count - 1);
    }

    Debug.Write("0x{:0:16} bytes of physical memory are being used, and 0x{:0:16} are free\n", UsedBytes,
                MaxBytes - UsedBytes);
}

/* Most of the alloc/free functions only redirect to the AllocInt function, the exception for this is the NonContig
 * functions, as the allocated memory pages DON'T need to be contiguous, so we can alloc one-by-one (which have
 * a lower chance of failing, as in the contig functions, the pages have to be on the same region). */

Status PhysMem::AllocSingle(UIntPtr &Out, UIntPtr Align) {
    return AllocInt(1, Out, Align);
}

Status PhysMem::AllocContig(UIntPtr Count, UIntPtr &Out, UIntPtr Align) {
    return AllocInt(Count, Out, Align);
}

Status PhysMem::AllocNonContig(UIntPtr Count, UIntPtr *Out, UIntPtr Align) {
    /* We need to manually check the two parameters here, as we're going to alloc page-by-page, and we're going to
     * return multiple addresses, instead of returning only a single one that points to the start of a bunch of
     * consecutive pages. */

    if (!Count || Out == Null) {
        Debug.SetForeground(0xFFFF0000);
        Debug.Write("invalid non-contig PhysMem::Alloc arguments (count = {}, out = 0x{:0*:16})\n", Count, Out);
        Debug.RestoreForeground();
        return Status::InvalidArg;
    }

    UIntPtr addr;
    Status status;

    for (UIntPtr i = 0; i < Count; i++) {	
        if ((status = AllocSingle(addr, Align)) != Status::Success) {
            FreeNonContig(Out, i);
            return status;
        }

        Out[i] = addr;
    }

    return Status::Success;
}

Status PhysMem::FreeSingle(UIntPtr Page) {
    return FreeInt(Page, 1);
}

Status PhysMem::FreeContig(UIntPtr Start, UIntPtr Count) {
    return FreeInt(Start, Count);
}

Status PhysMem::FreeNonContig(UIntPtr *Pages, UIntPtr Count) {
    /* This is the same as AllocNonContig, but instead of calling AllocSingle, we call FreeSingle, and we don't need
     * to save anything after doing that. */

    if (!Count || Pages == Null) {
        Debug.SetForeground(0xFFFF0000);
        Debug.Write("invalid non-contig PhysMem::Free arguments (pages = 0x{:0*:16}, count = {})\n", Pages, Count);
        Debug.RestoreForeground();
        return Status::InvalidArg;
    }

    Status status;

    for (UIntPtr i = 0; i < Count; i++) if ((status = FreeSingle(Pages[i])) != Status::Success) return status;

    return Status::Success;
}

Status PhysMem::ReferenceSingle(UIntPtr Page, UIntPtr &Out, UIntPtr Align) {
    /* For referencing single pages, we can just check if we need to alloc a new page (if we do we self call us with
     * said new allocated page), and increase the reference counter for the page. */

    if (!Page) {
        Status status = AllocSingle(Page, Align);
        if (status != Status::Success) return status;
        return ReferenceSingle(Page, Out, Align);
    } else if (References == Null || UsedBytes < PAGE_SIZE || (Page & PAGE_MASK) || Page < MinAddress ||
               Page >= MaxAddress) {
        Debug.SetForeground(0xFFFF0000);
        Debug.Write("invalid PhysMem::Reference arguments (page = 0x{:0*:16})\n", Page);
        Debug.RestoreForeground();
        return Status::InvalidArg;
    } else if (References[(Page - MinAddress) >> PAGE_SHIFT] < 0xFF) References[(Page - MinAddress) >> PAGE_SHIFT]++;

    Out = Page & ~PAGE_MASK;

    return Status::Success;
}

Status PhysMem::ReferenceContig(UIntPtr Start, UIntPtr Count, UIntPtr &Out, UIntPtr Align) {
    /* Referencing multiple contig pages is also pretty simple, same checks as the other function, but we need to
     * reference all the pages in a loop. */

    Status status;

    if (!Start) {
        if ((status = AllocContig(Count, Start, Align)) != Status::Success) return status;
        return ReferenceContig(Start, Count, Out, Align);
    } else if (References == Null || !Count || UsedBytes < (Count << PAGE_SHIFT) || (Start & PAGE_MASK) ||
               Start < MinAddress || Start + (Count << PAGE_SHIFT) > MaxAddress) {
        Debug.SetForeground(0xFFFF0000);
        Debug.Write("invalid contig PhysMem::Reference arguments (start = 0x{:0*:16}, count = {})\n", Start, Count);
        Debug.RestoreForeground();
        return Status::InvalidArg;
    }

    for (UIntPtr i = 0, discard = 0; i < Count; i++) {
        if ((status = ReferenceSingle(Start + (i << PAGE_SHIFT), discard)) != Status::Success) {
            DereferenceContig(Start, Count);
            return status;
        }
    }

    Out = Start;

    return Status::Success;
}

Status PhysMem::ReferenceNonContig(UIntPtr *Pages, UIntPtr Count, UIntPtr *Out, UIntPtr Align) {
    /* For non-contig pages, we just call ReferenceSingle on each of the pages. */

    if (References == Null || !Count || UsedBytes < (Count << PAGE_SHIFT) || Pages == Null || Out == Null) {
        Debug.SetForeground(0xFFFF0000);
        Debug.Write("invalid non-contig PhysMem::Reference arguments (pages = 0x{:0*:16}, count = {})\n", Pages, Count);
        Debug.RestoreForeground();
        return Status::InvalidArg;
    }

    Status status;

    for (UIntPtr i = 0; i < Count; i++) {
        if ((status = ReferenceSingle(Pages[i], Out[i], Align)) != Status::Success) {
            DereferenceNonContig(Pages, i);
            return status;
        }
    }

    return Status::Success;
}

Status PhysMem::DereferenceSingle(UIntPtr Page) {
    /* Dereferencing is pretty easy: Make sure that we referenced the address before, if we have, decrease the ref
     * count, and if we were the last ref to the address, dealloc it. */

    if (References == Null || UsedBytes < PAGE_SIZE || !Page || (Page & PAGE_MASK) || Page < MinAddress ||
        Page >= MaxAddress || !References[(Page - MinAddress) >> PAGE_SHIFT]) {
        Debug.SetForeground(0xFFFF0000);
        Debug.Write("invalid PhysMem::Dereference arguments (page = 0x{:0*:16})\n", Page);
        Debug.RestoreForeground();
        return Status::InvalidArg;
    }

    if (!(References[(Page - MinAddress) >> PAGE_SHIFT]--)) return FreeSingle(Page);
    return Status::Success;
}

/* Dereferencing contig pages and non-contig pages is pretty much the same process, but on contig pages, we can just
 * add 'i * PAGE_SIZE' to the start address to get the page, on non-contig, we just need to access Pages[i] to get
 * the page. */

Status PhysMem::DereferenceContig(UIntPtr Start, UIntPtr Count) {
    if (References == Null || !Count || UsedBytes < (Count << PAGE_SHIFT) || !Start || (Start & PAGE_MASK) ||
        Start < MinAddress || Start + (Count << PAGE_SHIFT) > MaxAddress) {
        Debug.SetForeground(0xFFFF0000);
        Debug.Write("Invalid contig PhysMem::Dereference arguments (start = 0x{:0*:16}, count = {})\n", Start, Count);
        Debug.RestoreForeground();
        return Status::InvalidArg;
    }

    Status status;

    for (UIntPtr i = 0; i < Count; i++) {
        if ((status = DereferenceSingle(Start + (i << PAGE_SHIFT))) != Status::Success) return status;
    }

    return Status::Success;
}

Status PhysMem::DereferenceNonContig(UIntPtr *Pages, UIntPtr Count) {
    if (References == Null || !Count || UsedBytes < (Count << PAGE_SHIFT) || Pages == Null) {
        Debug.SetForeground(0xFFFF0000);
        Debug.Write("invalid non-contig PhysMem::Dereference arguments (pages = 0x{:0*:16}, count = {})\n",
                    Pages, Count);
         Debug.SetForeground(0xFFFFFFF);
        return Status::InvalidArg;
    }

    Status status;
    for (UIntPtr i = 0; i < Count; i++) if ((status = DereferenceSingle(Pages[i])) != Status::Success) return status;
    return Status::Success;
}

UInt8 PhysMem::GetReferences(UIntPtr Page) {
    if (References == Null || !Page || Page < PAGE_SIZE || (Page & PAGE_MASK) || Page < MinAddress ||
        Page >= MaxAddress) {
        Debug.SetForeground(0xFFFF0000);
        Debug.Write("Invalid PhysMem::GetReference arguments (page = 0x{:0*:16})\n", Page);
        Debug.RestoreForeground();
        return 0;
    }

    return References[(Page - MinAddress) >> PAGE_SHIFT];
}

Status PhysMem::FindFreePages(UIntPtr BitMap, UIntPtr Count, UIntPtr &Out, UIntPtr &Available) {
    /* Each unset bit is one free page, for finding consecutive free pages, we can iterate through the bits of the
     * bitmap and search for free bits. */

    for (UIntPtr bc = PHYS_REGION_BITMAP_PSIZE, i = 0; i < bc;) {
        if (BitMap == UINTPTR_MAX) return Status::OutOfMemory;
        else if (!BitMap) {
            Out = i;
            Available = bc - i >= Count ? 0 : bc - i;
            return Available ? Status::OutOfMemory : Status::Success;
        }

        /* Well, we need to check how many unset bits we have, first, get the location of the first unset bit here,
         * after that, count how many available bits we have. */

        UIntPtr bit = BitOp::ScanForward(~BitMap);
        UIntPtr aval = BitOp::ScanForward(BitOp::GetBits(BitMap, bit, bc - i - 1));

        if (aval >= Count) {
            Out = i + bit;
            return Status::Success;
        } else if (i + bit + aval == bc) {
            /* There might be enough pages, but we need to cross into the next bitmap, let's set Out so that AllocInt
             * knows that. */

            Out = i + bit;
            Available = aval;

            return Status::OutOfMemory;
        }

        BitMap >>= bit + aval;
        i += bit + aval;
    }

    return Status::OutOfMemory;
}

Status PhysMem::AllocInt(UIntPtr Count, UIntPtr &Out, UIntPtr Align) {
    /* We need to check if all of the arguments are valid, and if the physical memory manager have already been
     * initialized. */

    if (!Count || !Align) {
        Debug.SetForeground(0xFFFF0000);
        Debug.Write("invalid PhysMem::AllocInt arguments (count = {}, align = {})\n", Count, Align);
        Debug.RestoreForeground();
        return Status::InvalidArg;
    } else if (Regions == Null || UsedBytes + (Count << PAGE_SHIFT) > MaxBytes) {
        Debug.SetForeground(0xFFFF0000);
        Debug.Write("not enough free memory for PhysMem::AllocInt (count = {})\n", Count);

        if (Regions != Null && (Heap::ReturnPhysical(), UsedBytes + (Count << PAGE_SHIFT) <= MaxBytes)) {
            Debug.Write("enough memory seems to have been freed through Heap::ReturnPhysical\n");
            Debug.RestoreForeground();
        } else {
            Debug.RestoreForeground();
            return Status::OutOfMemory;
        }
    }

    Align -= 1;

    /* Now, we can just go through each region, and each bitmap in each region, and try to find the consecutive free
     * pages that we were asked for. */

    for (UIntPtr i = 0; i < RegionCount; i++) {
        if (!Regions[i].Free) continue;

        for (UIntPtr j = 0; j < PHYS_REGION_BITMAP_LEN; j++) {
            if (Regions[i].Pages[j] == UINTPTR_MAX) continue;

            UIntPtr bit = 0, aval = 0;

            if (FindFreePages(Regions[i].Pages[j], Count, bit, aval) == Status::Success &&
                !((Out = MinAddress + (i << PHYS_REGION_SHIFT) + (j << PHYS_REGION_PAGE_SHIFT) + (bit << PAGE_SHIFT))
                       & Align)) {
                /* Oh, we actually found enough free pages! we need to set all the bits that we're going to use,
                 * increase the used bytes variable, and calculate the return value.
                 * INFO: We can't use SetRange/UnsetRange as they take references (and we can't pass references to
                 * packed fields to it). */

                Regions[i].Pages[j] |= BitOp::GetMask(bit, bit + Count - 1);

                Regions[i].Free -= Count;
                Regions[i].Used += Count;
                UsedBytes += (Count << PAGE_SHIFT);

                return Status::Success;
            } else if (aval && !((Out = MinAddress + (i << PHYS_REGION_SHIFT) + (j << PHYS_REGION_PAGE_SHIFT) +
                                                     (bit << PAGE_SHIFT)) & Align)) {
                /* Differently from the normal error/out of memory in this region, if the output wasn't null it means
                 * that we should try crossing into the next region/bitmap. */

                UIntPtr ci = i, cj = j, cur = aval;

                while (True) {
                    UIntPtr cbit = 0, caval = 0;
                
                    if (++cj >= PHYS_REGION_BITMAP_LEN) {
                        if (++ci >= RegionCount) break;
                        cj = 0;
                    }

                    if ((FindFreePages(Regions[ci].Pages[cj], Count - cur, cbit, caval) != Status::Success &&
                        caval != PHYS_REGION_BITMAP_PSIZE) || cbit) {
                        break;
                    } else if (caval) {
                        cur += caval;
                        continue;
                    }

                    /* Finally, we know that we have enough free physical memory (though it is going through multiple
                     * region bitmaps lol)! First, update the first and last regions, as the might be only partially
                     * filled. */

                    Regions[i].Free -= aval;
                    Regions[i].Used += aval;
                    Regions[ci].Free -= Count - cur;
                    Regions[ci].Used += Count - cur;

                    Regions[i].Pages[j] |= BitOp::GetMask(bit, bit + aval - 1);
                    Regions[ci].Pages[cj] |= BitOp::GetMask(cbit, cbit + Count - cur - 1);

                    /* Now fill the remaining/middle regions (first for the first and last i values, and then for the
                     * middle ones, which we will be able to set to all used). */

                    for (cj++; cj < PHYS_REGION_BITMAP_LEN; cj++) {
                        Regions[ci].Free -= PHYS_REGION_BITMAP_PSIZE;
                        Regions[ci].Used += PHYS_REGION_BITMAP_PSIZE;
                        Regions[ci].Pages[cj] = UINTPTR_MAX;
                    }

                    for (cj = j + 1; cj < PHYS_REGION_BITMAP_LEN; cj++) {
                        Regions[i].Free -= PHYS_REGION_BITMAP_PSIZE;
                        Regions[i].Used += PHYS_REGION_BITMAP_PSIZE;
                        Regions[i].Pages[cj] = UINTPTR_MAX;
                    }

                    for (cj = i + 1; cj < ci; cj++) {
                        Regions[cj].Free = 0;
                        Regions[cj].Used = PHYS_REGION_BITMAP_PSIZE;
                        SetMemory(Regions[cj].Pages, 0xFF, sizeof(Regions[cj].Pages));
                    }

                    UsedBytes += Count << PAGE_SHIFT;

                    return Status::Success;
                }
            }
        }
    }

    return Status::OutOfMemory;
}

Status PhysMem::FreeInt(UIntPtr Start, UIntPtr Count) {
    if (!Start || !Count || UsedBytes < (Count << PAGE_SHIFT) || (Start & PAGE_MASK) || Start < MinAddress ||
        Start + (Count << PAGE_SHIFT) > MaxAddress) {
        Debug.SetForeground(0xFFFF0000);
        Debug.Write("invalid PhysMem::FreeInt arguments (start = 0x{:0*:16}, count = {})\n",
                    Start, Count);
        Debug.RestoreForeground();
        return Status::InvalidArg;
    }

    Start -= MinAddress;

    while (Count) {
        /* Save the indexes of this page into the region bitmap. */
    
        UIntPtr i = Start >> PHYS_REGION_SHIFT, j = (Start >> PHYS_REGION_PAGE_SHIFT) & (PHYS_REGION_BITMAP_LEN - 1),
                k = (Start >> PAGE_SHIFT) & (PHYS_REGION_BITMAP_PSIZE - 1);

        /* Now check if we can just free a whole bitmap or region (those cases are easier to handle). */

        if (!j && !k && Count >= PHYS_REGION_PSIZE && !Regions[i].Free) {
            Regions[i].Free = PHYS_REGION_PSIZE;
            Regions[i].Used = 0;
            UsedBytes -= PHYS_REGION_BSIZE;
            Start += PHYS_REGION_BSIZE;
            Count -= PHYS_REGION_PSIZE;
            SetMemory(Regions[i].Pages, 0, sizeof(Regions[i].Pages));
            continue;
        } else if (!k && Count >= PHYS_REGION_BITMAP_PSIZE && Regions[i].Used >= PHYS_REGION_BITMAP_PSIZE) {
            Regions[i].Free += PHYS_REGION_BITMAP_PSIZE;
            Regions[i].Used -= PHYS_REGION_BITMAP_PSIZE;
            Regions[i].Pages[j] = 0;
            UsedBytes -= PHYS_REGION_BITMAP_BSIZE;
            Start += PHYS_REGION_BITMAP_BSIZE;
            Count -= PHYS_REGION_BITMAP_PSIZE;
            continue;
        }

        /* If none of the above, first do error checking, and free as many pages using a single operation.. */

        ASSERT(Regions[i].Used);

        UIntPtr end = Count >= PHYS_REGION_BITMAP_PSIZE - k ? (PHYS_REGION_BITMAP_PSIZE - 1)
                                                            : k + Count - 1, cnt = end - k + 1;

        Regions[i].Pages[j] &= ~BitOp::GetMask(k, end);
        Regions[i].Free += cnt;
        Regions[i].Used -= cnt;
        UsedBytes -= PAGE_SIZE * cnt;
        Start += PAGE_SIZE * cnt;
        Count -= cnt;
    }

    return Status::Success;
}
