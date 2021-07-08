/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 01 of 2020, at 19:47 BRT
 * Last edited on July 08 of 2021, at 09:14 BRT */

#include <sys/mm.hxx>
#include <sys/panic.hxx>
#include <util/bitop.hxx>

using namespace CHicago;

/* All of the static private variables. */

UInt64 PhysMem::MinAddress = 0, PhysMem::MaxAddress = 0, PhysMem::MaxBytes = 0, PhysMem::UsedBytes = 0;
UIntPtr PhysMem::PageCount = 0, PhysMem::KernelStart = 0, PhysMem::KernelEnd = 0;
PhysMem::Page *PhysMem::Pages = Null, *PhysMem::FreeList = Null, *PhysMem::WaitingList = Null;
Boolean PhysMem::Initialized = False;

Void PhysMem::Initialize(BootInfo &Info) {
    /* This function should only be called once by the kernel entry. It is responsible for initializing the physical
     * memory manager (allocate/deallocate physical memory, and manage how many references the pages have around all
     * the tasks/CPUs/whatever). */

    ASSERT(!Initialized);
    ASSERT(Info.PhysMgrStart);
    ASSERT(Info.KernelStart && Info.KernelEnd);
    ASSERT(Info.MemoryMap.Count && Info.MemoryMap.Entries != Null);

    MinAddress = Info.MinPhysicalAddress;
    MaxAddress = Info.MaxPhysicalAddress;
    MaxBytes = Info.PhysicalMemorySize;
    UsedBytes = MaxBytes;
    PageCount = (Info.MaxPhysicalAddress - Info.MinPhysicalAddress) >> PAGE_SHIFT;
    KernelStart = Info.KernelStart;
    KernelEnd = Info.KernelEnd;

    Debug.Write("the kernel starts at 0x{:0*:16}, and ends at 0x{:0*:16}\n"
                "minimum physical address is 0x{:0*:16}, and max is 0x{:0*:16}\n"
                "physical memory size is 0x{:0:16}\n", KernelStart, KernelEnd, MinAddress, MaxAddress, MaxBytes);

    SetMemory(Pages = reinterpret_cast<Page*>(Info.PhysMgrStart), 0, PageCount * sizeof(Page));

    /* Now using the boot memory map, we can free the free (duh) regions (those entries will be marked as
     * BOOT_INFO_MEM_FREE). No need to search for the right place to put the new pages, as we're inserting them in
     * sorted order. */

    Page *tail = Null, *group = Null;

    for (UIntPtr i = 0; i < Info.MemoryMap.Count; i++) {
        BootInfoMemMap &ent = Info.MemoryMap.Entries[i];
        UInt64 start = (ent.Base + (ent.Base ? 0 : PAGE_SIZE)) >> PAGE_SHIFT;

        Debug.Write("memory map entry no. {}, base = 0x{:0*:16}, size = 0x{:0:16}, type = {}\n", i, ent.Base,
                    ent.Count << PAGE_SHIFT, ent.Type);

        if (ent.Type != 0x05) continue;

        for (UInt64 cur = start; cur < start + ent.Count; cur++) {
            Page *page = &Pages[cur];

            /* The first entry is treated as a "group" entry, the ->Size field will tell the size of the whole region,
             * and ->NextGroup tells the start of the next group/single page that is not part of any group. */

            page->Size = cur == start ? ent.Count : 1;

            if (FreeList == Null) {
                FreeList = group = tail = page;
                continue;
            } else if (cur == start) {
                group->NextGroup = page;
                group = page;
            }

            tail->NextSingle = group->LastSingle = page;
            tail = page;
        }

        UsedBytes -= (ent.Count - (ent.Base > 0)) << PAGE_SHIFT;
    }

    Debug.Write("0x{:0:16} bytes of physical memory are being used, and 0x{:0:16} are free\n", UsedBytes,
                MaxBytes - UsedBytes);
}

/* Most of the alloc/free functions only redirect to the AllocInt function, the exception for this is the NonContig
 * functions, as the allocated memory pages DON'T need to be contiguous, so we can alloc one-by-one (which have
 * a lower chance of failing, as in the contig functions, the pages have to be on the same region). */

Status PhysMem::AllocSingle(UInt64 &Out, UInt64 Align) {
    return AllocInt(1, Out, Align);
}

Status PhysMem::AllocContig(UIntPtr Count, UInt64 &Out, UInt64 Align) {
    return AllocInt(Count, Out, Align);
}

Status PhysMem::AllocNonContig(UIntPtr Count, UInt64 *Out, UInt64 Align) {
    /* We need to manually check the two parameters here, as we're going to alloc page-by-page, and we're going to
     * return multiple addresses, instead of returning only a single one that points to the start of a bunch of
     * consecutive pages. */

    if (!Count || Out == Null) {
        Debug.SetForeground(0xFFFF0000);
        Debug.Write("invalid PhysMem::AllocNonContig arguments (count = {}, out = 0x{:0*:16})\n", Count, Out);
        Debug.RestoreForeground();
        return Status::InvalidArg;
    }

    UInt64 addr;
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

Status PhysMem::FreeSingle(UInt64 Page) {
    UInt64 idx = (Page - MinAddress) >> PAGE_SHIFT;

    if (Pages == Null || UsedBytes < PAGE_SIZE || (Page & PAGE_MASK) || Page < MinAddress || Page >= MaxAddress ||
        Pages[idx].Size) {
        Debug.SetForeground(0xFFFF0000);
        Debug.Write("invalid PhysMem::FreeSingle arguments (page = 0x{:0*:16})\n", Page);
        Debug.RestoreForeground();
        return Status::InvalidArg;
    }

    /* Freeing the page is simple: Just add it to the WaitingList (we're not going to handle actually searching the
     * right place in the FreeList and properly adding it there, instead, the AllocSingle/(Non)Contig caller (when out
     * of memory) or the special kernel thread should manually do that by calling FreeWaitingPages). */

    Pages[idx].Size = 1;

    if (WaitingList == Null) Pages[idx].LastSingle = Null, WaitingList = &Pages[idx];
    else if (WaitingList->LastSingle == Null) WaitingList->LastSingle = WaitingList->NextSingle = &Pages[idx];
    else WaitingList->LastSingle->NextSingle = &Pages[idx], WaitingList->LastSingle = &Pages[idx];

    return Status::Success;
}

Status PhysMem::FreeContig(UInt64 Start, UIntPtr Count) {
    for (UInt64 cur = Start; cur < Start + (Count << PAGE_SHIFT); cur += PAGE_SIZE) {
        Status status = FreeSingle(cur);
        if (status != Status::Success) return status;
    }

    return Status::Success;
}

Status PhysMem::FreeNonContig(UInt64 *Pages, UIntPtr Count) {
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

Status PhysMem::ReferenceSingle(UInt64 Page, UInt64 &Out, UInt64 Align) {
    /* For referencing single pages, we can just check if we need to alloc a new page (if we do we self call us with
     * said new allocated page), and increase the reference counter for the page. */

    if (!Page) {
        Status status = AllocSingle(Page, Align);
        if (status != Status::Success) return status;
        return ReferenceSingle(Page, Out, Align);
    } else if (Pages == Null || UsedBytes < PAGE_SIZE || (Page & PAGE_MASK) || Page < MinAddress ||
               Page >= MaxAddress) {
        Debug.SetForeground(0xFFFF0000);
        Debug.Write("invalid PhysMem::ReferenceSingle arguments (page = 0x{:0*:16})\n", Page);
        Debug.RestoreForeground();
        return Status::InvalidArg;
    }

    Pages[(Page - MinAddress) >> PAGE_SHIFT].References++;
    Out = Page & ~PAGE_MASK;

    return Status::Success;
}

Status PhysMem::ReferenceContig(UInt64 Start, UIntPtr Count, UInt64 &Out, UInt64 Align) {
    /* Referencing multiple contig pages is also pretty simple, same checks as the other function, but we need to
     * reference all the pages in a loop. */

    Status status;

    if (!Start) {
        if ((status = AllocContig(Count, Start, Align)) != Status::Success) return status;
        return ReferenceContig(Start, Count, Out, Align);
    } else if (Pages == Null || !Count || UsedBytes < (Count << PAGE_SHIFT) || (Start & PAGE_MASK) ||
               Start < MinAddress || Start + (Count << PAGE_SHIFT) > MaxAddress) {
        Debug.SetForeground(0xFFFF0000);
        Debug.Write("invalid contig PhysMem::ReferenceContig arguments (start = 0x{:0*:16}, count = {})\n",
                    Start, Count);
        Debug.RestoreForeground();
        return Status::InvalidArg;
    }

    UInt64 discard = 0;

    for (UIntPtr i = 0; i < Count; i++) {
        if ((status = ReferenceSingle(Start + (i << PAGE_SHIFT), discard)) != Status::Success) {
            DereferenceContig(Start, Count);
            return status;
        }
    }

    Out = Start;

    return Status::Success;
}

Status PhysMem::ReferenceNonContig(UInt64 *Pages, UIntPtr Count, UInt64 *Out, UInt64 Align) {
    /* For non-contig pages, we just call ReferenceSingle on each of the pages. */

    if (Pages == Null || !Count || UsedBytes < (Count << PAGE_SHIFT) || Pages == Null || Out == Null) {
        Debug.SetForeground(0xFFFF0000);
        Debug.Write("invalid PhysMem::ReferenceNonContig arguments (pages = 0x{:0*:16}, count = {})\n", Pages, Count);
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

Status PhysMem::DereferenceSingle(UInt64 Page) {
    /* Dereferencing is pretty easy: Make sure that we referenced the address before, if we have, decrease the ref
     * count, and if we were the last ref to the address, dealloc it. */

    if (Pages == Null || UsedBytes < PAGE_SIZE || !Page || (Page & PAGE_MASK) || Page < MinAddress ||
        Page >= MaxAddress || !Pages[(Page - MinAddress) >> PAGE_SHIFT].References) {
        Debug.SetForeground(0xFFFF0000);
        Debug.Write("invalid PhysMem::DereferenceSingle arguments (page = 0x{:0*:16})\n", Page);
        Debug.RestoreForeground();
        return Status::InvalidArg;
    }

    if (!(--Pages[(Page - MinAddress) >> PAGE_SHIFT].References)) return FreeSingle(Page);
    return Status::Success;
}

/* Dereferencing contig pages and non-contig pages is pretty much the same process, but on contig pages, we can just
 * add 'i * PAGE_SIZE' to the start address to get the page, on non-contig, we just need to access Pages[i] to get
 * the page. */

Status PhysMem::DereferenceContig(UInt64 Start, UIntPtr Count) {
    if (Pages == Null || !Count || UsedBytes < (Count << PAGE_SHIFT) || !Start || (Start & PAGE_MASK) ||
        Start < MinAddress || Start + (Count << PAGE_SHIFT) > MaxAddress) {
        Debug.SetForeground(0xFFFF0000);
        Debug.Write("Invalid PhysMem::DereferenceContig arguments (start = 0x{:0*:16}, count = {})\n", Start, Count);
        Debug.RestoreForeground();
        return Status::InvalidArg;
    }

    Status status;

    for (UIntPtr i = 0; i < Count; i++)
        if ((status = DereferenceSingle(Start + (i << PAGE_SHIFT))) != Status::Success) return status;

    return Status::Success;
}

Status PhysMem::DereferenceNonContig(UInt64 *Pages, UIntPtr Count) {
    if (Pages == Null || !Count || UsedBytes < (Count << PAGE_SHIFT) || Pages == Null) {
        Debug.SetForeground(0xFFFF0000);
        Debug.Write("invalid PhysMem::DereferenceNonContig arguments (pages = 0x{:0*:16}, count = {})\n",
                    Pages, Count);
         Debug.SetForeground(0xFFFFFFF);
        return Status::InvalidArg;
    }

    Status status;
    for (UIntPtr i = 0; i < Count; i++) if ((status = DereferenceSingle(Pages[i])) != Status::Success) return status;
    return Status::Success;
}

UIntPtr PhysMem::GetReferences(UInt64 Page) {
    if (Pages == Null || Page < PAGE_SIZE || (Page & PAGE_MASK) || Page < MinAddress || Page >= MaxAddress) {
        Debug.SetForeground(0xFFFF0000);
        Debug.Write("Invalid PhysMem::GetReferences arguments (page = 0x{:0*:16})\n", Page);
        Debug.RestoreForeground();
        return 0;
    }

    return Pages[(Page - MinAddress) >> PAGE_SHIFT].References;
}

Void PhysMem::FreeWaitingPages(Void) {
    while (WaitingList != Null) {
        /* Easiest case to handle is when this is a whole new entry (doesn't continue any other), as we just need to add
         * it to the right place. Other than that, we might need to increase the size of one region, and merge multiple
         * regions after increasing the size. */

        Page *cur = FreeList, *prev = Null, *next = WaitingList->NextSingle;

        for (; cur != Null && Reverse(cur) < Reverse(WaitingList) &&
               Reverse(WaitingList) != Reverse(cur) + (cur->Size << PAGE_SHIFT); prev = cur, cur = cur->NextGroup) ;

        UsedBytes -= PAGE_SIZE;
        WaitingList->LastSingle = Null;

        if (cur == Null && prev == Null) {
            WaitingList->NextSingle = Null;
            FreeList = WaitingList;
        } else if (cur == Null) {
            WaitingList->NextSingle = Null;
            prev->NextGroup = WaitingList;
        } else if (Reverse(cur) >= Reverse(WaitingList)) {
            WaitingList->NextSingle = WaitingList->NextGroup = cur;
            prev->NextGroup = prev->LastSingle->NextSingle = WaitingList;
        } else {
            WaitingList->NextSingle = cur->LastSingle->NextSingle;
            cur->LastSingle->NextSingle = WaitingList;
            cur->LastSingle = WaitingList;
            cur->Size++;

            while (cur->NextGroup != Null && Reverse(cur) + (cur->Size << PAGE_SHIFT) == Reverse(cur->NextGroup)) {
                cur->Size += cur->NextGroup->Size;
                cur->NextGroup->Size = 1;
                cur->LastSingle = cur->NextGroup->LastSingle;
                cur->NextGroup = cur->NextGroup->NextGroup;
            }
        }

        WaitingList = next;
    }
}

UInt64 PhysMem::Reverse(Page *Node) {
    return ((reinterpret_cast<UIntPtr>(Node) - reinterpret_cast<UIntPtr>(Pages)) / sizeof(Page)) << PAGE_SHIFT;
}

Status PhysMem::AllocInt(UIntPtr Count, UInt64 &Out, UInt64 Align) {
    /* We need to check if all of the arguments are valid, and if the physical memory manager have already been
     * initialized. */

    if (!Count || !Align) {
        Debug.SetForeground(0xFFFF0000);
        Debug.Write("invalid PhysMem::AllocInt arguments (count = {}, align = {})\n", Count, Align);
        Debug.RestoreForeground();
        return Status::InvalidArg;
    } else if (Pages == Null || UsedBytes + (Count << PAGE_SHIFT) > MaxBytes) {
        Debug.SetForeground(0xFFFF0000);
        Debug.Write("not enough free memory for PhysMem::AllocInt (count = {})\n", Count);

        if (Pages != Null && (Heap::ReturnPhysical(), FreeWaitingPages(),
                              UsedBytes + (Count << PAGE_SHIFT) <= MaxBytes)) {
            Debug.Write("enough memory seems to have been freed, continuing allocation\n");
            Debug.RestoreForeground();
        } else {
            Debug.RestoreForeground();
            return Status::OutOfMemory;
        }
    }

    Align -= 1;

    /* If everything is seemingly alright, we can check if there is any group with right size (and right alignment,
     * though taking later pages of the group might help with that). */

    Page *prev = Null;

    for (Page *cur = FreeList; cur != Null; prev = cur, cur = cur->NextGroup) {
        if (cur->Size < Count) continue;
        else if (Reverse(cur) & Align) {
            UIntPtr size = cur->Size - 1;
            Page *prev2 = cur, *cur2 = cur->NextSingle;
            UInt64 end = Reverse(cur) + (cur->Size << PAGE_SHIFT);

            for (; cur2 != Null && Reverse(cur2) < end && size >= Count; prev2 = cur2, cur2 = cur2->NextSingle, size--)
                if (!(Reverse(cur2) & Align)) break;
            if (cur2 == Null || Reverse(cur2) >= end || size < Count) continue;

            /* Two things we might have to do here: Or we need to just decrease the cur->Size, or we need to split the
             * block in two (which later might be rejoined in the FreeWaitingList func). */

            Out = Reverse(cur2);
            UsedBytes += Count << PAGE_SHIFT;

            for (UIntPtr i = 0; i < Count; i++) {
                Page *next = cur2->NextSingle;
                cur2->Size = 0;
                cur2->References = 1;
                cur2->NextSingle = Null;
                cur2 = next;
            }

            if (Reverse(cur2) >= end) cur->Size -= Count;
            else {
                cur2->Size = size - Count;
                cur2->NextGroup = cur->NextGroup;
                cur->Size -= size;
                cur->NextGroup = cur2;
            }

            return prev2->NextSingle = cur2, Status::Success;
        }

        /* Taking the pages from the start is already enough (alignment is OK), we just need to move the group start
         * (or remove the group). */

        Out = Reverse(cur);
        UsedBytes += Count << PAGE_SHIFT;

        UIntPtr nsize = cur->Size - Count;
        Page *group = cur->NextGroup, *last = cur->LastSingle;

        while (Count--) {
            Page *next = cur->NextSingle;
            cur->Size = 0;
            cur->References = 1;
            cur->NextSingle = cur->NextGroup = cur->LastSingle = Null;
            cur = next;
        }

        if (prev == Null) FreeList = cur;
        else prev->NextSingle = prev->NextGroup = cur;

        if (nsize) cur->NextGroup = group, cur->LastSingle = last, cur->Size = nsize;
        else if (prev != Null) prev->NextGroup = group;

        return Status::Success;
    }

    return Status::OutOfMemory;
}
