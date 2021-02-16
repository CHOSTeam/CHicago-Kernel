/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 14 of 2021, at 23:45 BRT
 * Last edited on February 16 of 2021, at 19:48 BRT */

#include <mm.hxx>
#include <panic.hxx>

using namespace CHicago;

Boolean Heap::Initialized = False;
AllocBlock *Heap::Base = Null, *Heap::Tail = Null;
UIntPtr Heap::Start = 0, Heap::End = 0, Heap::Current = 0, Heap::CurrentAligned = 0;

Void Heap::Initialize(UIntPtr Start, UIntPtr End) {
    ASSERT(!Initialized);
    ASSERT(Start != End);
    ASSERT(Start >= PhysMem::GetMinAddress() && End >= PhysMem::GetMinAddress());

    Heap::Start = Current = CurrentAligned = Start;
    Heap::End = End;
    Initialized = True;
}

Status Heap::Increment(UIntPtr Amount) {
	/* We need to check for two things: First, if the Amount is even valid (if it isn't, return InvalidArg), second, if
	 * we aren't trying to expand beyond the heap limit. */

    if (!Amount) {
        return Status::InvalidArg;
    } else if ((Current + Amount) < Current || (Current + Amount) >= End) {
        return Status::OutOfMemory;
    }

	/* Now, we need to expand the heap, but this is not just a matter of increasing the Heap::Current variable, as we
	 * need to make sure that the space we're expanding into is going to be mapped into the memory. */
	
	UIntPtr nw = Current + Amount, phys;
	Status status;

	for (; CurrentAligned < nw; CurrentAligned += PAGE_SIZE) {
		if ((status = PhysMem::ReferenceSingle(0, phys)) != Status::Success) {
			return status;
		} else if ((status = VirtMem::Map(CurrentAligned, phys, PAGE_SIZE, MAP_RW)) != Status::Success) {
			PhysMem::DereferenceSingle(phys);
			return status;
		}
	}

    return Current = nw, Status::Success;
}

Status Heap::Decrement(UIntPtr Amount) {
    if (!Amount || (Current - Amount) > Current || (Current - Amount) >= End) {
        return Status::InvalidArg;
    }

    /* Decrease all the variables, and free all the physical pages that we allocated. */

    Current -= Amount;

    for (; CurrentAligned - PAGE_SIZE >= Current;) {
        UIntPtr phys;
        UInt32 flags;

        CurrentAligned -= PAGE_SIZE;

        if (VirtMem::Query(CurrentAligned, phys, flags) == Status::Success) {
            PhysMem::DereferenceSingle(phys);
        }
    }

    return Status::Success;
}

Void Heap::AddFree(AllocBlock *Block) {
    /* Add the entry to the free list (for faster free block searching). We don't have a non free block list, as even
     * for getting the block size we don't need it (we would need it for dumping all the allocations though). It's
     * probably a good idea to find the right place for this block (not just randomly add it to the end). */

    if (Base == Null) {
        Base = Tail = Block;
        return;
    }

    AllocBlock *cur = Base;

    while (cur != Null && cur->Start > Block->Start) {
        cur = cur->Next;
    }

    if (cur != Null) {
        Block->Next = cur;

        if (cur->Prev != Null) {
            cur->Prev->Next = Block;
        } else {
            Base = Block;
        }
    } else {
        Tail->Next = Block;
        Block->Prev = Tail;
        Tail = Block;
    }
}

Void Heap::RemoveFree(AllocBlock *Block) {
    /* And here we do the inverse, remove the entry from the free list (nothing to do afterwards, as there is no other
     * list). */

    if (Block->Next != Null) {
        Block->Next->Prev = Block->Prev;
    } else {
        Tail = Block->Prev;

        if (Tail != Null) {
            Tail->Next = Null;
        } else {
            Base = Null;
        }
    }

    if (Block->Prev != Null) {
        Block->Prev->Next = Block->Next;
    } else if (Block->Next != Null) {
        Block->Next->Prev = Null;
        Base = Block->Next;
    }

    Block->Next = Block->Prev = Null;
}

Void Heap::SplitBlock(AllocBlock *Block, UIntPtr Size) {
    /* We probably could skip the Null pointer checks, as the only ones that are going to call this (and the other
     * private functions) is ourselves, but let's do it anyways. We also need to check if the block is big enough to be
     * split, it needs at least the size of the new block we want to create + the size of the alloc header. */

    if (Block == Null || !Size || Block->Size <= Size + sizeof(AllocBlock)) {
        return;
    }

    /* Let's split the block so the current block have exactly the same that the caller asked for, and the new block is
     * the remainder (which will be set as free). */

    auto nblk = reinterpret_cast<AllocBlock*>(Block->Start + Size);

    SetMemory(nblk, 0, sizeof(AllocBlock));

    nblk->Magic = ALLOC_BLOCK_MAGIC;
    nblk->Start = Block->Start + Size + sizeof(AllocBlock);
    nblk->Size = Block->Size - (Size + sizeof(AllocBlock));
    Block->Size = Size;

    AddFree(nblk);
}

AllocBlock *Heap::FuseBlock(AllocBlock *Block) {
    /* We can only fuse blocks that are just after ourselves, to fuse blocks that are in the ->Prev field, the caller
     * should call the FuseBlock function on the ->Prev field, instead of the current block. */

    if (Block != Null && Block->Next != Null && reinterpret_cast<UIntPtr>(Block->Next) == Block->Start + Block->Size) {
        Block->Size += sizeof(AllocBlock) + Block->Next->Size;
        Block->Next = Block->Next->Next;

        if (Block->Next != Null) {
            Block->Next->Prev = Block;
        } else {
            Tail->Prev->Next = Block;
            Tail = Block;
        }

        return Block;
    }

    return Null;
}

AllocBlock *Heap::FindBlock(UIntPtr Size) {
    /* The block list always ends (or starts, if there are no blocks yet) on a Null pointer, so we can always just keep
     * on following the ->Next chain until we find a Null pointer. */

    if (!Size) {
        return Null;
    }

    AllocBlock *cur = Base, *best = Null;

    while (cur != Null) {
        if (cur->Size >= Size && (best == Null || cur->Size < best->Size)) {
            best = cur;
        }

        cur = cur->Next;
    }

    return best;
}

AllocBlock *Heap::CreateBlock(UIntPtr Size) {
    /* The "Last" pointer is not required (for example, this may be the first time we're allocating memory, and this may
     * be the first allocation), but we do need to check if the Size is valid. */

    if (!Size) {
        return Null;
    }

    /* Here on the kernel, we can just increment the heap pointer (actually we also need to alloc and map, but the
     * Increment function takes care of that), a bit different from heap function of other OSes, we need to first get
     * the current location of the heap, and call the Increment function (that is going to return if the increment is
     * valid, and if it succeeded). */

    auto blk = reinterpret_cast<AllocBlock*>(Current);

    if (Increment(Size + sizeof(AllocBlock)) != Status::Success) {
        return Null;
    }

    SetMemory(blk, 0, sizeof(AllocBlock));

    blk->Magic = ALLOC_BLOCK_MAGIC;
    blk->Start = reinterpret_cast<UIntPtr>(blk) + sizeof(AllocBlock);
    blk->Size = Size;

    return blk;
}

Void *Heap::Allocate(UIntPtr Size) {
    /* With all the function that we wrote, now is just a question of checking if we need to create a new block, or
     * use/split an existing one. */

    Size = ((Size > 0 ? Size : 1) + 15) & -16;

    AllocBlock *blk = FindBlock(Size);

    if (blk != Null) {
        /* Let's not waste space, and split the block that we found in case it is too big. */

        if (blk->Size > Size + sizeof(AllocBlock)) {
            SplitBlock(blk, Size);
        }

        RemoveFree(blk);
    } else {
        blk = CreateBlock(Size);
    }

    /* If everything went well, we can zero the allocated memory (just to be safe) and return, else, we should return a
     * null pointer.
     * The ASSERT() is temp, as it's only here to make sure our allocator is properly working/always returning 16-byte
     * aligned buffers. */

    if (blk != Null) {
        ASSERT(!(blk->Start & 0x0F));
        auto ret = reinterpret_cast<Void*>(blk->Start);
        return SetMemory(ret, 0, Size), ret;
    }

    return Null;
}

Void *Heap::Allocate(UIntPtr Size, UIntPtr Align) {
    /* Just as in the other allocate function, the size needs to be valid and aligned, but now we also need to check if
     * the Align value is valid (it needs to be a power of 2). Also, ::Allocate already guarantees 16-byte alignment, so
     * for Align values lesser or equal to that, we don't need anything special. */

    if (!Size || !Align || (Align & (Align - 1))) {
        return Null;
    } else if (Align <= 16) {
        return Allocate(Size);
    }

    /* Let's over-alloc the space we need, so we can save that this is an aligned allocation, and the address of the
     * block header. */

    UIntPtr tsz = Size + 2 * sizeof(UIntPtr) + (Align - 1);
    Void *p0 = Allocate(tsz);

    if (p0 == Null) {
        return Null;
    }

    /* Now, we just need to get the actual aligned start (the part that we're going to return). We can access it like it
     * was a pointer, to save what we need. */

    auto p0i = reinterpret_cast<UIntPtr>(p0);
    auto p1 = reinterpret_cast<UIntPtr*>((p0i + tsz) & -Align);

    return p1[-1] = ALLOC_BLOCK_MAGIC, p1[-2] = p0i - sizeof(AllocBlock), p1;
}

Void Heap::Deallocate(Void *Address) {
    /* We need to check if the specified address is valid, and is inside of the kernel heap (from the start until the
     * current highest allocated address), after that, we need to check if this is an aligned allocation, if it is, we
     * need to convert the address into an UIntPtr pointer, and access ptr[-2] to get the header location, else, we just
     * subtract the size of the header from the address. */

    AllocBlock *blk, *fblk;
    auto addr = reinterpret_cast<UIntPtr>(Address);

    ASSERT(Address != Null && addr >= Start + sizeof(AllocBlock) && Start <= Current);

    if (addr - 2 * sizeof(UIntPtr) >= Start + sizeof(AllocBlock) &&
        (reinterpret_cast<UIntPtr*>(Address))[-1] == ALLOC_BLOCK_MAGIC) {
        blk = reinterpret_cast<AllocBlock*>((reinterpret_cast<UIntPtr*>(Address))[-2]);
    } else {
        blk = reinterpret_cast<AllocBlock*>(addr - sizeof(AllocBlock));
    }

    /* Now, we need to check if this is a valid block, and if we haven't called Deallocate on it before (double
     * free). */

    ASSERT(blk->Magic == ALLOC_BLOCK_MAGIC);
    ASSERT(blk != Base && blk->Next == Null && blk->Prev == Null);

    /* Add this block to the free list, and start fusing it with other block around it. */

    AddFree(blk);

    while ((fblk = FuseBlock(blk->Prev)) != Null) {
        blk = fblk;
    }

    while (FuseBlock(blk) != Null) ;

    /* If this is the end of the block list, AND the block is just at the end of the heap, let's free some space on the
     * heap (this will actually free the physical memory). */

    if (blk == Tail && reinterpret_cast<UIntPtr>(blk) == Current - (blk->Size + sizeof(AllocBlock))) {
        if (blk->Prev != Null) {
            Tail = blk->Prev;
            Tail->Next = Null;
        } else {
            Base = Tail = Null;
        }

        Decrement(blk->Size + sizeof(AllocBlock));
    }
}
