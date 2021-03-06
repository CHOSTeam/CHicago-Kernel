/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 09 of 2021, at 16:14 BRT
 * Last edited on July 19 of 2021, at 20:02 BRT */

#include <vid/console.hxx>

using namespace CHicago;

UIntPtr VirtMem::Start = 0, VirtMem::End = 0, VirtMem::Current = 0, VirtMem::FreeCount = 0;
VirtMem::Page *VirtMem::FreeList = Null;
SpinLock VirtMem::Lock {};

Status VirtMem::Allocate(UIntPtr Count, UIntPtr &Out, UIntPtr Align) {
    Status status;

    if (!Start || !Count || Count >= PAGE_SIZE / sizeof(Page) || !Align || Align & (Align - 1)) {
        Debug.Write("{}invalid VirtMem::Allocate arguments (count = {}, align = {}){}\n", SetForeground { 0xFFFF0000 },
                    Count, Align, RestoreForeground{});
        return Status::InvalidArg;
    } else if (FreeCount < Count && Current >= End) {
        Debug.Write("{}not enough free memory for VirtMem::Allocate (count = {}){}\n", SetForeground { 0xFFFF0000 },
                    Count, RestoreForeground{});
        return Status::OutOfMemory;
    } else if (FreeCount < Count && (Heap::ReturnMemory(), FreeCount < Count)) {
        if ((Lock.Acquire(), status = ExpandPool()) != Status::Success) return Lock.Release(), status;
        Lock.Release();
    }

    Lock.Acquire();

    if ((status = AllocatePages(FreeList, Reverse, Count, Out, Align)) == Status::Success ||
        ((status = ExpandPool()) == Status::Success &&
         (status = AllocatePages(FreeList, Reverse, Count, Out, Align)) == Status::Success)) FreeCount -= Count;

    return Lock.Release(), status;
}

Status VirtMem::Free(UIntPtr Start, UIntPtr Count) {
    if (!Start || (Start & PAGE_MASK) || Start < VirtMem::Start || Start + (Count << PAGE_SHIFT) > Current) {
        Debug.Write("{}invalid VirtMem::Free arguments (start = 0x{:0*:16}, count = {}){}\n",
                    SetForeground { 0xFFFF0000 }, Start, Count, RestoreForeground{});
        return Status::InvalidArg;
    }

    Lock.Acquire();
    FreePages(FreeList, Reverse, [](UIntPtr Address) {
        auto group = reinterpret_cast<Page*>(Address & ~(VIRT_GROUP_RANGE - 1));
        return &group[(Address - reinterpret_cast<UIntPtr>(group)) >> PAGE_SHIFT];
    }, Start, Count);
    return Lock.Release(), Status::Success;
}

Status VirtMem::MapIo(UInt64 Physical, UIntPtr &Size, UIntPtr &Out) {
    /* MMIO addresses are all physical, but we of course always have paging/virtual memory on, so we need to remap
     * them into virtual memory (VirtMem::Allocate makes it very easy to grab a large enough virtual address). */

    Status status;
    UIntPtr saves = Size;
    UInt64 savep = Physical;

    if (!Physical || !Size || (Size & PAGE_MASK)) return Status::InvalidArg;
    else if (Physical & PAGE_MASK) Physical &= ~PAGE_MASK, Size += -Size & PAGE_MASK;

    /* Make the size cross into the next page if it would cross a page in the old unaligned values. */

    if (((savep + saves) & ~PAGE_MASK) > (savep & ~PAGE_MASK)) Size += PAGE_SIZE;

    return (status = Allocate(Size >> PAGE_SHIFT, Out)) != Status::Success ||
           (status = Map(Out, Physical, Size, MAP_KERNEL | MAP_RW)) != Status::Success ? status :
           (Out += savep & PAGE_MASK, Status::Success);
}

Status VirtMem::ExpandPool(Void) {
    /* First allocation or all the pages in all the allocated blocks are being used, we need to alloc a new block. */

    UInt64 phys;
    Status status = PhysMem::Reference(0, 1, phys);
    auto group = reinterpret_cast<Page*>(Current);

    if (status != Status::Success ||
        (status = VirtMem::Map(Current, phys, PAGE_SIZE, MAP_KERNEL | MAP_RW)) != Status::Success) return status;

    /* Default state: Everything other than the first page (the page info array) is free, so we can add everything to
     * the free list (already in sorted order). The process is the same as the one in the PMM (but the tail is
     * global). */

    SetMemory(group, 0, PAGE_SIZE);

    Page *tail = FreeList;
    while (tail != Null && tail->NextGroup != Null) tail = tail->NextGroup;

    for (UIntPtr i = 1; i < VIRT_GROUP_PAGE_COUNT; i++) {
        Page *page = &group[i];

        page->Count = i == 1 ? VIRT_GROUP_PAGE_COUNT - 1 : 1;

        if (FreeList == Null) {
            FreeList = page->LastSingle = tail = page;
            continue;
        } else if (i == 1) {
            tail->NextGroup = tail->LastSingle->NextSingle = page;
            tail = page;
        } else if (tail->LastSingle != Null) tail->LastSingle->NextSingle = page;

        tail->LastSingle = page;
    }

    Current += VIRT_GROUP_RANGE;
    FreeCount += VIRT_GROUP_PAGE_COUNT - 1;

    return Status::Success;
}

UIntPtr VirtMem::Reverse(Page *Node) {
    UIntPtr page = reinterpret_cast<UIntPtr>(Node), group = page & ~(VIRT_GROUP_RANGE - 1);
    return group + (page - group) / sizeof(Page) * PAGE_SIZE;
}
