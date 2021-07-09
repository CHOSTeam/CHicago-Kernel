/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 01 of 2020, at 19:47 BRT
 * Last edited on July 09 of 2021, at 12:20 BRT */

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
    Initialized = True;

    Debug.Write("the kernel starts at 0x{:0*:16}, and ends at 0x{:0*:16}\n"
                "minimum physical address is 0x{:0*:16}, and max is 0x{:0*:16}\n"
                "physical memory size is 0x{:0:16}\n", KernelStart, KernelEnd, MinAddress, MaxAddress, MaxBytes);

    SetMemory(Pages = reinterpret_cast<Page*>(Info.PhysMgrStart), 0, PageCount * sizeof(Page));

    /* Now using the boot memory map, we can free the free (duh) regions (those entries will be marked as
     * BOOT_INFO_MEM_FREE). No need to search for the right place to put the new pages, as we're inserting them in
     * sorted order. */

    Page *group = Null;

    for (UIntPtr i = 0; i < Info.MemoryMap.Count; i++) {
        BootInfoMemMap &ent = Info.MemoryMap.Entries[i];
        UInt64 start = (ent.Base + (ent.Base ? 0 : PAGE_SIZE)) >> PAGE_SHIFT;

        Debug.Write("memory map entry no. {}, base = 0x{:0*:16}, size = 0x{:0:16}, type = {}\n", i, ent.Base,
                    ent.Count << PAGE_SHIFT, ent.Type);

        if (ent.Type != 0x05) continue;

        for (UInt64 cur = start; cur < start + ent.Count; cur++) {
            Page *page = &Pages[cur];

            /* The first entry is treated as a "group" entry, the ->Count field will tell the size of the whole region,
             * and ->NextGroup tells the start of the next group/single page that is not part of any group. */

            page->Count = cur == start ? ent.Count : 1;

            if (FreeList == Null) {
                FreeList = group = page->LastSingle = page;
                continue;
            } else if (cur == start) {
                group->NextGroup = group->LastSingle->NextSingle = page;
                group = page;
            } else if (group->LastSingle != Null) group->LastSingle->NextSingle = page;

            group->LastSingle = page;
        }

        UsedBytes -= (ent.Count - (ent.Base > 0)) << PAGE_SHIFT;
    }

    Debug.Write("0x{:0:16} bytes of physical memory are being used, and 0x{:0:16} are free\n", UsedBytes,
                MaxBytes - UsedBytes);
}

Status PhysMem::Allocate(UIntPtr Count, UInt64 &Out, UInt64 Align) {
    /* We need to check if all of the arguments are valid, and if the physical memory manager have already been
     * initialized. */

    if (!Count || !Align) {
        Debug.SetForeground(0xFFFF0000);
        Debug.Write("invalid PhysMem::Allocate arguments (count = {}, align = {})\n", Count, Align);
        Debug.RestoreForeground();
        return Status::InvalidArg;
    } else if (Pages == Null || UsedBytes + (Count << PAGE_SHIFT) > MaxBytes) {
        Debug.SetForeground(0xFFFF0000);
        Debug.Write("not enough free memory for PhysMem::Allocate (count = {})\n", Count);

        if (Pages != Null && (Heap::ReturnPhysical(), FreeWaitingPages(),
            UsedBytes + (Count << PAGE_SHIFT) <= MaxBytes)) {
            Debug.Write("enough memory seems to have been freed, continuing allocation\n");
            Debug.RestoreForeground();
        } else {
            Debug.RestoreForeground();
            return Status::OutOfMemory;
        }
    }

    Status status = AllocatePages(FreeList, Reverse, Count, Out, Align);
    if (status == Status::Success) UsedBytes += Count << PAGE_SHIFT;
    return status;
}

Status PhysMem::Allocate(UIntPtr Count, UInt64 *Out, UInt64 Align) {
    /* We need to manually check the two parameters here, as we're going to alloc page-by-page, and we're going to
     * return multiple addresses, instead of returning only a single one that points to the start of a bunch of
     * consecutive pages. */

    if (!Count || Out == Null) {
        Debug.SetForeground(0xFFFF0000);
        Debug.Write("invalid non-contig PhysMem::Allocate arguments (count = {}, out = 0x{:0*:16})\n", Count, Out);
        Debug.RestoreForeground();
        return Status::InvalidArg;
    }

    UInt64 addr;
    Status status;

    for (UIntPtr i = 0; i < Count; i++) {
        if ((status = Allocate(1, addr, Align)) != Status::Success) {
            Free(Out, i);
            return status;
        }

        Out[i] = addr;
    }

    return Status::Success;
}

Status PhysMem::Free(UInt64 Start, UIntPtr Count) {
    if (Pages == Null || UsedBytes < (Count << PAGE_SHIFT) || (Start & PAGE_MASK) || Start < MinAddress ||
        Start + (Count << PAGE_SHIFT) >= MaxAddress) {
        Debug.SetForeground(0xFFFF0000);
        Debug.Write("invalid PhysMem::Free arguments (start = 0x{:0*:16}, count = {})\n", Start, Count);
        Debug.RestoreForeground();
        return Status::InvalidArg;
    }

    for (UInt64 cur = Start; cur < Start + (Count << PAGE_SHIFT); cur += PAGE_SIZE) {
        UInt64 idx = (cur - MinAddress) >> PAGE_SHIFT;

        if (Pages[idx].Count) {
            Debug.SetForeground(0xFFFF0000);
            Debug.Write("invalid PhysMem::Free arguments (start = 0x{:0*:16}, count = {})\n", Start, Count);
            Debug.RestoreForeground();
            return Status::InvalidArg;
        }

        /* Freeing the page is simple: Just add it to the WaitingList (we're not going to handle actually searching the
         * right place in the FreeList and properly adding it there, instead, the AllocSingle/(Non)Contig caller (when
         * out of memory) or the special kernel thread should manually do that by calling FreeWaitingPages). */

        Pages[idx].Count = 1;

        if (WaitingList == Null) Pages[idx].LastSingle = Null, WaitingList = &Pages[idx];
        else if (WaitingList->LastSingle == Null) WaitingList->LastSingle = WaitingList->NextSingle = &Pages[idx];
        else WaitingList->LastSingle->NextSingle = &Pages[idx], WaitingList->LastSingle = &Pages[idx];
    }

    return Status::Success;
}

Status PhysMem::Free(UInt64 *Pages, UIntPtr Count) {
    if (!Count || Pages == Null) {
        Debug.SetForeground(0xFFFF0000);
        Debug.Write("invalid non-contig PhysMem::Free arguments (pages = 0x{:0*:16}, count = {})\n", Pages, Count);
        Debug.RestoreForeground();
        return Status::InvalidArg;
    }

    Status status;

    for (UIntPtr i = 0; i < Count; i++) if ((status = Free(Pages[i])) != Status::Success) return status;

    return Status::Success;
}

Status PhysMem::Reference(UInt64 Start, UIntPtr Count, UInt64 &Out, UInt64 Align) {
    /* Start being empty means that we need to allocate the page(s) (and then we can just call ourselves
     * recursively). */

    Status status;

    if (!Start) {
        if ((status = Allocate(Count, Start, Align)) != Status::Success) return status;
        return Reference(Start, Count, Out, Align);
    } else if (Pages == Null || !Count || UsedBytes < (Count << PAGE_SHIFT) || (Start & PAGE_MASK) ||
               Start < MinAddress || Start + (Count << PAGE_SHIFT) > MaxAddress) {
        Debug.SetForeground(0xFFFF0000);
        Debug.Write("invalid PhysMem::Reference arguments (start = 0x{:0*:16}, count = {})\n", Start, Count);
        Debug.RestoreForeground();
        return Status::InvalidArg;
    }

    for (UIntPtr i = 0; i < Count; i++) Pages[((Start - MinAddress) >> PAGE_SHIFT) + i].References++;

    return Out = Start, Status::Success;
}

Status PhysMem::Reference(UInt64 *Pages, UIntPtr Count, UInt64 *Out, UInt64 Align) {
    /* For non-contig pages, we just call ReferenceSingle on each of the pages. */

    if (Pages == Null || !Count || UsedBytes < (Count << PAGE_SHIFT) || Pages == Null || Out == Null) {
        Debug.SetForeground(0xFFFF0000);
        Debug.Write("invalid non-contig PhysMem::Reference arguments (pages = 0x{:0*:16}, count = {})\n", Pages, Count);
        Debug.RestoreForeground();
        return Status::InvalidArg;
    }

    Status status;

    for (UIntPtr i = 0; i < Count; i++) {
        if ((status = Reference(Pages[i], 1, Out[i], Align)) != Status::Success) {
            Dereference(Pages, i);
            return status;
        }
    }

    return Status::Success;
}

/* Dereferencing contig pages and non-contig pages is pretty much the same process, but on contig pages, we can just
 * add 'i * PAGE_SIZE' to the start address to get the page, on non-contig, we just need to access Pages[i] to get
 * the page. */

Status PhysMem::Dereference(UInt64 Start, UIntPtr Count) {
    if (Pages == Null || !Count || UsedBytes < (Count << PAGE_SHIFT) || !Start || (Start & PAGE_MASK) ||
        Start < MinAddress || Start + (Count << PAGE_SHIFT) > MaxAddress) {
        Debug.SetForeground(0xFFFF0000);
        Debug.Write("Invalid PhysMem::Dereference arguments (start = 0x{:0*:16}, count = {})\n", Start, Count);
        Debug.RestoreForeground();
        return Status::InvalidArg;
    }

    for (UIntPtr i = 0; i < Count; i++)
        if (!--Pages[((Start - MinAddress) >> PAGE_SHIFT) + i].References) Free(Start + (i << PAGE_SHIFT));

    return Status::Success;
}

Status PhysMem::Dereference(UInt64 *Pages, UIntPtr Count) {
    if (Pages == Null || !Count || UsedBytes < (Count << PAGE_SHIFT) || Pages == Null) {
        Debug.SetForeground(0xFFFF0000);
        Debug.Write("invalid non-contig PhysMem::DereferenceNonContig arguments (pages = 0x{:0*:16}, count = {})\n",
                    Pages, Count);
         Debug.SetForeground(0xFFFFFFF);
        return Status::InvalidArg;
    }

    Status status;
    for (UIntPtr i = 0; i < Count; i++) if ((status = Dereference(Pages[i])) != Status::Success) return status;
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
    UsedBytes -= ::FreeWaitingPages(FreeList, WaitingList, Reverse) << PAGE_SHIFT;
}

UInt64 PhysMem::Reverse(Page *Node) {
    return ((reinterpret_cast<UIntPtr>(Node) - reinterpret_cast<UIntPtr>(Pages)) / sizeof(Page)) << PAGE_SHIFT;
}
