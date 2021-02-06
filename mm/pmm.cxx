/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 01 of 2020, at 19:47 BRT
 * Last edited on February 06 of 2021, at 18:41 BRT */

#include <mm.hxx>

/* This is probably the only code (non-header) file with some type of macro definition on it lol. */

#ifdef _LP64
#define GET_FIRST_UNSET_BIT(bm) __builtin_ctzll(~(bm))
#else
#define GET_FIRST_UNSET_BIT(bm) __builtin_ctz(~(bm))
#endif

/* All of the static private variables. */

UIntPtr PhysMem::KernelStart = 0, PhysMem::KernelEnd = 0, PhysMem::RegionCount = 0;
UInt64 PhysMem::MinAddress = 0, PhysMem::MaxAddress = 0, PhysMem::MaxBytes = 0, PhysMem::UsedBytes = 0;
PhysMem::Region *PhysMem::Regions = Null;
UInt8 *PhysMem::References = Null;
Boolean PhysMem::Initialized = False;

Void PhysMem::Initialize(BootInfo *Info) {
    /* This function should only be called once by the kernel entry. It is responsible for initializing the physical
     * memory manager (allocate/deallocate physical memory, and manage how many references the pages have around all
     * the tasks/CPUs/whatever). */

    if (Initialized || Info == Null) {
        return;
    }

    /* First, setup some of the basic fields, including the InInit field, that indicates that the we were called, but
     * the FinishInitialization functions isn't haven't been called (that is, we can call InitializeRegion). */

    UInt8 *start = reinterpret_cast<UInt8*>(Info->RegionsStart);

    RegionCount = ((Info->MaxPhysicalAddress - Info->MinPhysicalAddress) >> MM_PAGE_SHIFT) >> 10;
    KernelStart = Info->KernelStart;
    KernelEnd = Info->KernelEnd;
    MinAddress = Info->MinPhysicalAddress;
    MaxAddress = Info->MaxPhysicalAddress;
    MaxBytes = Info->PhysicalMemorySize;
    UsedBytes = MaxBytes;

    /* Now, we need to initialize the region list, and the reference list, we need to do this before setting the
     * kernel end, as we're going to put those lists after the current kernel end. There is one very important thing,
     * we expect the loader to already have mapped those lists onto virtual memory. */

    Regions = reinterpret_cast<PhysMem::Region*>(start);
    References = &start[RegionCount * sizeof(PhysMem::Region)];

    /* Now, let's clear the region list and the reference list (set all the entries on the region list to be already
     * used, and all the entries on the references list to have 0 references). */

    for (UIntPtr i = 0; i < RegionCount; i++) {
        Regions[i].Free = 0;
        Regions[i].Used = MM_REGION_BITMAP_LEN;

        for (UIntPtr i = 0; i < sizeof(Regions[i].Pages); i++) {
            Regions[i].Pages[i] = 0xFF;
        }
    }

    for (UIntPtr i = 0; i < RegionCount << 10; i++) {
        References[i] = 0;
    }

    /* Now using the boot memory map, we can free the free (duh) regions (those entries will be marked as
     * BOOT_INFO_MEM_FREE). */

    for (UIntPtr i = 0; i < Info->MemoryMap.Count; i++) {
        BootInfoMemMap *ent = &Info->MemoryMap.Entries[i];

        if (ent->Type == BOOT_INFO_MEM_FREE) {
            FreeInt(ent->Base, ent->Count);
        }
    }
}

/* Most of the alloc/free functions only redirect to the AllocInt function, the exception for this is the NonContig
 * functions, as the allocated memory pages DON'T need to be contiguous, so we can alloc one-by-one (which have
 * a lower chance of failing, as in the contig functions, the pages have to be on the same region). */

Status PhysMem::AllocSingle(UIntPtr &Out) {
    return AllocInt(1, Out);
}

Status PhysMem::AllocContig(UIntPtr Count, UIntPtr &Out) {
    return AllocInt(Count, Out);
}

Status PhysMem::AllocNonContig(UIntPtr Count, UIntPtr *Out) {
    /* We need to manually check the two paramenter here, as we're going to alloc page-by-page, and we're going to
     * return multiple addresses, instead of returning only a single one that points to the start of a bunch of
     * consecutive pages. */

    if (!Count || Out == Null) {
        return Status::InvalidArg;
    }

    UIntPtr addr;
    Status status;

    for (UIntPtr i = 0; i < Count; i++) {	
        if ((status = AllocSingle(addr)) != Status::Success) {
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
        return Status::InvalidArg;
    }

    Status status;

    for (UIntPtr i = 0; i < Count; i++) {
        if ((status = FreeSingle(Pages[i])) != Status::Success) {
            return status;
        }
    }

    return Status::Success;
}

Status PhysMem::ReferenceSingle(UIntPtr Page, UIntPtr &Out) {
    /* For referencing single pages, we can just check if we need to alloc a new page (if we do we self call us with
     * said new allocated page), and increase the reference counter for the page. */

    if (!Page) {
        Status status = AllocSingle(Page);
        
        if (status != Status::Success) {
            return status;
        }
        
        return ReferenceSingle(Page, Out);
    } else if (References == Null || UsedBytes < MM_PAGE_SIZE || (Page & MM_PAGE_MASK) || Page < MinAddress ||
               Page >= MaxAddress) {
        return Status::InvalidArg;
    } else if (References[(Page - MinAddress) >> MM_PAGE_SHIFT] < 0xFF) {
        References[(Page - MinAddress) >> MM_PAGE_SHIFT]++;
    }

    Out = Page & ~MM_PAGE_MASK;

    return Status::Success;
}

Status PhysMem::ReferenceContig(UIntPtr Start, UIntPtr Count, UIntPtr &Out) {
    /* Referencing multiple contig pages is also pretty simple, same checks as the other function, but we need to
     * reference all the pages in a loop. */

    Status status;

    if (!Start) {
        if ((status = AllocContig(Count, Start)) != Status::Success) {
            return status;
        }

        return ReferenceContig(Start, Count, Out);
    } else if (References == Null || !Count || UsedBytes < Count * MM_PAGE_SIZE || (Start & MM_PAGE_MASK) ||
               Start < MinAddress || Start + Count * MM_PAGE_SIZE >= MaxAddress) {
        return Status::InvalidArg;
    }

    for (UIntPtr i = 0, discard = 0; i < Count; i++) {
        if ((status = ReferenceSingle(Start + i * MM_PAGE_SIZE, discard)) != Status::Success) {
            DereferenceContig(Start, Count);
            return status;
        }
    }

    Out = Start;

    return Status::Success;
}

Status PhysMem::ReferenceNonContig(UIntPtr *Pages, UIntPtr Count, UIntPtr *Out) {
    /* For non-contig pages, we just call ReferenceSingle on each of the pages. */

    if (References == Null || !Count || UsedBytes < Count * MM_PAGE_SIZE || Pages == Null || Out == Null) {
        return Status::InvalidArg;
    }

    Status status;

    for (UIntPtr i = 0; i < Count; i++) {
        if ((status = ReferenceSingle(Pages[i], Out[i])) != Status::Success) {
            DereferenceNonContig(Pages, i);
            return status;
        }
    }

    return Status::Success;
}

Status PhysMem::DereferenceSingle(UIntPtr Page) {
    /* Dereferencing is pretty easy: Make sure that we referenced the address before, if we have, decrease the ref
     * count, and if we were the last ref to the address, dealloc it. */

    if (References == Null || UsedBytes < MM_PAGE_SIZE || !Page || (Page & MM_PAGE_MASK) || Page < MinAddress ||
        Page >= MaxAddress || !References[(Page - MinAddress) >> MM_PAGE_SHIFT]) {
        return Status::InvalidArg;
    }

    if (!(References[(Page - MinAddress) >> MM_PAGE_SHIFT]--)) {
        return FreeSingle(Page);
    }

    return Status::Success;
}

/* Dereferencing contig pages and non-contig pages is pretty much the same process, but on contig pages, we can just
 * add 'i * PAGE_SIZE' to the start address to get the page, on non-contig, we just need to access Pages[i] to get
 * the page. */

Status PhysMem::DereferenceContig(UIntPtr Start, UIntPtr Count) {
    if (References == Null || !Count || UsedBytes < Count * MM_PAGE_SIZE || !Start || (Start & MM_PAGE_MASK) ||
        Start < MinAddress || Start + Count * MM_PAGE_SIZE >= MaxAddress) {
        return Status::InvalidArg;
    }

    Status status;

    for (UIntPtr i = 0; i < Count; i++) {
        if ((status = DereferenceSingle(Start + i * MM_PAGE_SIZE)) != Status::Success) {
            return status;
        }
    }

    return Status::Success;
}

Status PhysMem::DereferenceNonContig(UIntPtr *Pages, UIntPtr Count) {
    if (References == Null || !Count || UsedBytes < Count * MM_PAGE_SIZE || Pages == Null) {
        return Status::InvalidArg;
    }

    Status status;

    for (UIntPtr i = 0; i < Count; i++) {
        if ((status = DereferenceSingle(Pages[i])) != Status::Success) {
            return status;
        }
    }

    return Status::Success;
}

UInt8 PhysMem::GetReferences(UIntPtr Page) {
    if (References == Null || !Page || Page < MM_PAGE_SIZE || (Page & MM_PAGE_MASK) || Page < MinAddress ||
        Page >= MaxAddress) {
        return 0;
    }

    return References[(Page - MinAddress) >> MM_PAGE_SHIFT];
}

UIntPtr PhysMem::CountFreePages(UIntPtr BitMap, UIntPtr Start, UIntPtr End) {
    /* As each bit of the bitmap is one physical memory page, to count the amount of free pages we just need to count
     * how many (consecutive) bits are NOT set. */

    UIntPtr ret = 0;

    for (; Start + ret <= End && !(BitMap & (1 << (Start + ret))); ret++) ;

    return ret;
}

Status PhysMem::FindFreePages(UIntPtr BitMap, UIntPtr Count, UIntPtr &Out, UIntPtr &Avaliable) {
    /* Each unset bit is one free page, for finding consective free pages, we can iterate through the bits of the
     * bitmap and search for free bits. */

    UIntPtr bc = sizeof(UIntPtr) * 8;

    for (UIntPtr i = 0; i < bc;) {
        if (BitMap == UINTPTR_MAX || bc - i < Count) {
            /* If all the remaining bits are set, or if there aren't enough bits left, there is really nothing we can
             * do, we're out of memory (in this bitmap). */

            return Status::OutOfMemory;
        } else if (!BitMap) {
            /* If all the remaining bytes are free, we can just return the current position, as we already checked if
             * we have enough bits! */

            Out = i;

            return Status::Success;
        }

        /* Well, we need to check how many unset bits we have, first, get the location of the first unset bit
         * here, after that, count how many avaliable bits we have. */

        UIntPtr bit = GET_FIRST_UNSET_BIT(BitMap);
        UIntPtr aval = CountFreePages(BitMap, bit, bc - i);

        if (aval >= Count) {
            Out = i + bit;
            return Status::Success;
        } else if (i + bit + aval >= bc) {
            /* There might be enough pages, but we need to cross into the next bitmap, let's set Out so that AllocInt
             * knows that. */

            Out = i + bit;
            Avaliable = aval;

            return Status::OutOfMemory;
        }

        BitMap >>= bit + aval;
        i += bit + aval;
    }

    return Status::OutOfMemory;
}

Status PhysMem::AllocInt(UIntPtr Count, UIntPtr &Out) {
    /* We need to check if all of the arguments are valid, and if the physical memory manager have already been
     * initialized. */

    if (!Count) {
        return Status::InvalidArg;
    } else if (Regions == Null || UsedBytes + Count * MM_PAGE_SIZE >= MaxBytes) {
        return Status::OutOfMemory;
    }

    /* Now, we can just go through each region, and each bitmap in each region, and try to find the consecutive free
     * pages that we were asked for. */

    for (UIntPtr i = 0; i < RegionCount; i++) {
        if (Regions[i].Free < Count) {
            /* This variable exists so we don't need to check a region that WILL NOT have enough bits in any of the
             * bitmaps. */

            continue;
        }

        for (UIntPtr j = 0; j < MM_REGION_BITMAP_LEN; j++) {
            UIntPtr bit = 0, aval;

            if (FindFreePages(Regions[i].Pages[j], Count, bit, aval) == Status::Success) {
                /* Oh, we actually found enough free pages! we need to set all the bits that we're going to use,
                 * increase the used bytes variable, and calculate the return value. */

                for (UIntPtr k = bit; k < bit + Count; k++) {
                    Regions[i].Pages[j] |= (UIntPtr)1 << k;
                }

                Regions[i].Free -= Count;
                Regions[i].Used += Count;
                UsedBytes += Count * MM_PAGE_SIZE;
                Out = MinAddress + (i * (1 << MM_REGION_SHIFT)) + (j * (1 << MM_REGION_PAGE_SHIFT)) +
                      (bit * MM_PAGE_SIZE);

                return Status::Success;
            } else if (bit) {
                /* Differently from the normal error/out of memory in this region, if the output wasn't null it means
                 * that we should try crossing into the next region/bitmap. */

                UIntPtr ci = i, cj = j, cur = aval;

                while (True) {
                    UIntPtr cbit = 0, caval = 0;
                
                    if (++cj >= MM_REGION_BITMAP_LEN && ++ci >= RegionCount) {
                        break;
                    } else if (FindFreePages(Regions[ci].Pages[cj], Count - cur, cbit, caval) != Status::Success &&
                               (cbit || !caval)) {
                        break;
                    } else if (!cbit && caval) {
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

                    for (UIntPtr k = bit; k < bit + aval; k++) {
                        Regions[i].Pages[j] |= (UIntPtr)1 << k;
                    }

                    for (UIntPtr k = cbit; k < cbit + Count - cur; k++) {
                        Regions[ci].Pages[cj] |= (UIntPtr)1 << k;
                    }

                    /* Now fill the remaining/middle regions (first for the first and last i values, and then for the
                     * middle ones, which we will be able to set to 0xFF/max). */

                    for (cj++; cj < MM_REGION_BITMAP_LEN; cj++) {
                        Regions[ci].Free -= sizeof(UIntPtr) * 8;
                        Regions[ci].Used += sizeof(UIntPtr) * 8;
                        Regions[ci].Pages[cj] = UINTPTR_MAX;
                    }

                    for (cj = j + 1; cj < MM_REGION_BITMAP_LEN; cj++) {
                        Regions[i].Free -= sizeof(UIntPtr) * 8;
                        Regions[i].Used += sizeof(UIntPtr) * 8;
                        Regions[i].Pages[cj] = UINTPTR_MAX;
                    }

                    for (cj = i + 1; cj < ci; cj++) {
                        Regions[cj].Free = 0;
                        Regions[cj].Used = MM_REGION_BITMAP_LEN;

                        for (UIntPtr k = 0; k < MM_REGION_BITMAP_LEN; k++) {
                            Regions[cj].Pages[k] = 0xFF;
                        }
                    }

                    UsedBytes += Count * MM_PAGE_SIZE;
                    Out = MinAddress + (i * (1 << MM_REGION_SHIFT)) + (j * (1 << MM_REGION_PAGE_SHIFT)) +
                          (bit * MM_PAGE_SIZE);
                }
            }
        }
    }

    return Status::OutOfMemory;
}

Status PhysMem::FreeInt(UIntPtr Start, UIntPtr Count) {
    if (!Start || !Count || UsedBytes < Count * MM_PAGE_SIZE || (Start & MM_PAGE_MASK) || Start < MinAddress ||
        Start + Count * MM_PAGE_SIZE >= MaxAddress) {
        return Status::InvalidArg;
    }

    while (Count) {
        /* Save the indexes of this page into the region bitmap. */
    
        UIntPtr i = Start >> MM_REGION_SHIFT, j = (Start - (i << MM_REGION_SHIFT)) >> MM_REGION_PAGE_SHIFT,
                k = (Start - (i << MM_REGION_SHIFT) - (j << MM_REGION_PAGE_SHIFT)) >> MM_PAGE_SHIFT;

        /* Now check if we can just free a whole bitmap or region (those cases are easier to handle). */

        if (j == 0 && k == 0 && Count >= MM_REGION_BITMAP_LEN * sizeof(UIntPtr) * 8 &&
            Regions[i].Used == 0) {
            Regions[i].Free = MM_REGION_BITMAP_LEN * sizeof(UIntPtr) * 8;
            Regions[i].Used = 0;
            UsedBytes -= MM_PAGE_SIZE * MM_REGION_BITMAP_LEN * sizeof(UIntPtr) * 8;
            Start += MM_PAGE_SIZE * MM_REGION_BITMAP_LEN * sizeof(UIntPtr) * 8;
            Count -= MM_REGION_BITMAP_LEN * sizeof(UIntPtr) * 8;

            for (; j < MM_REGION_BITMAP_LEN; j++) {
                Regions[i].Pages[j] = 0xFF;
            }
            
            continue;
        } else if (k == 0 && Count >= sizeof(UIntPtr) * 8 && Regions[i].Used >= sizeof(UIntPtr) * 8) {
            Regions[i].Free += sizeof(UIntPtr) * 8;
            Regions[i].Used -= sizeof(UIntPtr) * 8;
            Regions[i].Pages[j] = 0xFF;
            UsedBytes += MM_PAGE_SIZE * sizeof(UIntPtr) * 8;
            Start += MM_PAGE_SIZE * sizeof(UIntPtr) * 8;
            Count -= sizeof(UIntPtr) * 8;
            continue;
        }

        /* If none of the above, first do error checking, and then free a single page. */

        if (!Regions[i].Used) {
            return Status::InvalidArg;
        }

        Regions[i].Pages[j] &= ~((UIntPtr)1 << k);
        Regions[i].Free--;
        Regions[i].Used--;
        UsedBytes -= MM_PAGE_SIZE;
        Start += MM_PAGE_SIZE;
        Count--;
    }

    return Status::Success;
}
