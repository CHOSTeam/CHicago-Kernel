/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 14 of 2021, at 23:45 BRT
 * Last edited on July 18 of 2021, at 22:15 BRT */

#include <sys/mm.hxx>
#include <sys/panic.hxx>

using namespace CHicago;

Heap::Block *Heap::Head = Null, *Heap::Tail = Null;
SpinLock Heap::Lock {};

Void Heap::ReturnMemory(Void) {
    /* Just find any blocks that are free and have the size as a multiple of the page size (and the block header address
     * itself is aligned to the page size). */

    Boolean adv = True;

    for (Block *cur = Head; cur != Null; cur = adv ? cur->Next : cur, adv = True) {
        UIntPtr addr = reinterpret_cast<UIntPtr>(cur), size = cur->Size;

        if (!(addr & PAGE_MASK) && !(size & PAGE_MASK)) {
            Block *next = cur->Next;
            RemoveFree(cur);

            for (UIntPtr i = 0; i < size; i += PAGE_SIZE) {
                UInt64 phys;
                UInt32 flags;
                if (VirtMem::Query(addr + (i << PAGE_SHIFT), phys, flags) == Status::Success)
                    PhysMem::Dereference(phys);
            }

            VirtMem::Unmap(addr, size);
            cur = next;
            adv = False;
        }
    }
}

Void *Heap::Allocate(UIntPtr Size) {
    Lock.Acquire();
    Block *block = FindFree(Size += -Size & 0x0F);
    if (block != Null) RemoveFree(block);
    else if ((block = CreateBlock(Size)) == Null) return Lock.Release(), Null;
    return Split(block, Size), Lock.Release(), SetMemory(block->Data, 0, block->Size), block->Data;
}

Void *Heap::Allocate(UIntPtr Size, UIntPtr Align) {
    /* Allocate(Size) guarantees at least 16-byte alignment, so we can redirect to it if Align is too low. */

    if (!Align || Align & (Align - 1)) return Null;
    if (Align <= 16) return Allocate(Size);

    /* There are two different blocks that we can use: With the ->Data field perfectly aligned, and with enough size
     * to split it in a block with at least size 16 and an aligned block with the requested (16-byte aligned) size. */

    Lock.Acquire();

    Block *cur = Head, *best = Null;
    for (; cur != Null; cur = cur->Next) {
        UIntPtr size = Align - (reinterpret_cast<UIntPtr>(cur->Data) & (Align - 1)) - sizeof(Block) +
                                                                                      sizeof(Block::Free);
        if (size < 16) size += Align;
        if (!(reinterpret_cast<UIntPtr>(cur->Data) & (Align - 1)) && cur->Size >= Size) break;
        else if ((best == Null || cur->Size < best->Size) && cur->Size >= size + Size) best = cur;
    }

    UIntPtr size = Align - ((best == Null ? sizeof(Block::Free) : reinterpret_cast<UIntPtr>(best->Data)) & (Align - 1))
                         - sizeof(Block) + sizeof(Block::Free);
    if (cur == Null && size < 16) size += Align;

    if (cur != Null) RemoveFree(cur);
    else if (best != Null) cur = Split(best, size, False);
    else if ((cur = CreateBlock(size + Size)) == Null) return Lock.Release(), Null;
    else cur = Split(cur, size, False);

    return Split(cur, size), Lock.Release(), SetMemory(cur->Data, 0, cur->Size), cur->Data;
}

Void Heap::Free(Void *Data) {
    Lock.Acquire();

    auto addr = reinterpret_cast<UIntPtr>(Data);
    auto blk = reinterpret_cast<Block*>(addr - sizeof(Block) + sizeof(Block::Free));

    /* AddFree should also return if it finds that the block is already in the list, so there is no need to manually
     * check that. */

    ASSERT(addr);
    ASSERT(blk->Magic == ALLOC_BLOCK_MAGIC);
    ASSERT(AddFree(blk));

    /* Fuse the block in both directions (all in the same loop). */

    while (True) {
        Boolean fuse = False;

        if (blk->Prev != Null &&
            reinterpret_cast<UIntPtr>(blk->Prev->Data) + blk->Prev->Size == reinterpret_cast<UIntPtr>(blk)) {
            if (blk == Tail) Tail = blk->Prev;
            else blk->Next->Prev = blk->Prev;
            blk->Prev->Size += blk->Size + sizeof(Block) - sizeof(Block::Free);
            blk->Prev->Next = blk->Next;
            blk = blk->Prev;
            fuse = True;
        }

        if (blk->Next != Null &&
            reinterpret_cast<UIntPtr>(blk->Next) == reinterpret_cast<UIntPtr>(blk->Data) + blk->Size) {
            if (blk->Next == Tail) Tail = blk;
            else blk->Next->Next->Prev = blk;
            blk->Size += blk->Next->Size + sizeof(Block) - sizeof(Block::Free);
            blk->Next = blk->Next->Next;
            fuse = True;
        }

        if (!fuse) break;
    }

    Lock.Release();
}

Heap::Block *Heap::Split(Block *Block, UIntPtr Size, Boolean Free) {
    if (Block->Size < sizeof(Heap::Block) - sizeof(Block::Free) + 16) return Null;

    /* We can calculate the new start while ignoring the Next and Prev fields of the old block, as they are part of
     * the block data (unlike the magic number and size). */

    auto nblk = reinterpret_cast<Heap::Block*>(&Block->Data[Size]);

    nblk->Magic = ALLOC_BLOCK_MAGIC;
    nblk->Size = Block->Size - Size - sizeof(Heap::Block) + sizeof(Block::Free);
    Block->Size = Size;

    if (Free) AddFree(nblk);

    return nblk;
}

Heap::Block *Heap::CreateBlock(UIntPtr Size) {
    /* No more bump heap in the kernel, now we need to manually allocate a valid virtual address and then a physical
     * address. */

    UIntPtr virt;
    UInt64 phys[(Size += sizeof(Block) - sizeof(Block::Free), (Size = (Size + (-Size & PAGE_MASK)) >> PAGE_SHIFT))];

    SetMemory(phys, 0, sizeof(phys));

    if (PhysMem::Reference(phys, Size, phys) != Status::Success) return Null;
    else if (VirtMem::Allocate(Size, virt) != Status::Success) return PhysMem::Dereference(phys, Size), Null;

    for (UIntPtr i = 0; i < Size; i++) {
        if (VirtMem::Map(virt + (i << PAGE_SHIFT), phys[i], PAGE_SIZE, MAP_KERNEL | MAP_RW) != Status::Success) {
            VirtMem::Unmap(virt, i << PAGE_SHIFT);
            VirtMem::Free(virt, Size);
            PhysMem::Free(phys, Size);
        }
    }

    auto blk = reinterpret_cast<Block*>(virt);

    blk->Magic = ALLOC_BLOCK_MAGIC;
    blk->Size = (Size << PAGE_SHIFT) - sizeof(Block) + sizeof(Block::Free);

    return blk;
}

Heap::Block *Heap::FindFree(UIntPtr Size) {
    for (Block *cur = Head; cur != Null; cur = cur->Next) if (cur->Size >= Size) return cur;
    return Null;
}

Void Heap::RemoveFree(Block *Block) {
    /* Removing is VERY simple, unlink the block while remembering to update the Head and Tail values (if
     * necessary). */

    if (Block == Null) return;

    if (Block == Head) Head = Block->Next;
    else Block->Prev->Next = Block->Next;
    if (Block == Tail) Tail = Block->Prev;
    else Block->Next->Prev = Block->Prev;
}

Boolean Heap::AddFree(Block *Block) {
    /* Adding a new block is almost as simple, but we have to search for the right place to add it (so that we have a
     * pretty much priceless best fit search). */

    if (Block == Null || Block == Head || Block == Tail) return False;

    if (Head == Null || reinterpret_cast<UIntPtr>(Block) < reinterpret_cast<UIntPtr>(Head)) {
        if (Head != Null) Head->Prev = Block;
        else Tail = Block;
        Block->Next = Head;
        Block->Prev = Null;
        Head = Block;
        return True;
    } else if (reinterpret_cast<UIntPtr>(Block) > reinterpret_cast<UIntPtr>(Tail)) {
        Tail->Next = Block;
        Block->Next = Null;
        Block->Prev = Tail;
        Tail = Block;
        return True;
    }

    Heap::Block *cur = Head;
    while (reinterpret_cast<UIntPtr>(Block) > reinterpret_cast<UIntPtr>(cur)) cur = cur->Next;
    if (Block == cur) return False;

    Block->Next = cur;
    Block->Prev = cur->Prev;
    cur->Prev->Next = Block;
    cur->Prev = Block;

    return True;
}
