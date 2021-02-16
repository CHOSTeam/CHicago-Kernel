/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 14 of 2021, at 23:45 BRT
 * Last edited on February 16 of 2021, at 12:29 BRT */

#include <mm.hxx>
#include <panic.hxx>

using namespace CHicago;

AllocBlock *Heap::Base = Null;
Boolean Heap::Initialized = False;
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
        UInt32 flags;
        UIntPtr phys;

        CurrentAligned -= PAGE_SIZE;

        if (VirtMem::Query(CurrentAligned, phys, flags) == Status::Success) {
            PhysMem::DereferenceSingle(phys);
        }
    }

    return Status::Success;
}

Void Heap::SplitBlock(AllocBlock *Block, UIntPtr Size) {
    /* We probably could skip the Null pointer checks, as the only ones that are going to call this (and the other
     * private functions) is ourselves, but let's do it anyways. We also need to check if the block is big enough to be
     * split, it needs at least the size of the new block we want to create + the size of the alloc header. */

    if (Block == Null || !Size || Block->Size <= Size + sizeof(AllocBlock)) {
        return;
    }

    /* Let's split the block so the current block have exactly the same that the caller asked for, and the new block is
     * the remainder. */

    auto nblk = reinterpret_cast<AllocBlock*>(Block->Start + Size);

    nblk->Magic = ALLOC_BLOCK_MAGIC;
    nblk->Free = False;
    nblk->Start = Block->Start + Size + sizeof(AllocBlock);
    nblk->Size = Block->Size - (Size + sizeof(AllocBlock));
    nblk->Next = Block->Next;
    nblk->Prev = Block;

    /* And we can't forget to link both blocks together. */

    Block->Size = Size;
    Block->Next = nblk;

    if (nblk->Next != Null) {
        nblk->Next->Prev = nblk;
    }
}

AllocBlock *Heap::FuseBlock(AllocBlock *Block) {
    /* We can only fuse blocks that are just after ourselves, to fuse blocks that are in the ->Prev field, the caller
     * should call the FuseBlock function on the ->Prev field, instead of the current block. */

    if (Block != Null && Block->Next != Null && reinterpret_cast<UIntPtr>(Block->Next) == Block->Start + Block->Size &&
        Block->Free) {
        Block->Size += sizeof(AllocBlock) + Block->Next->Size;
        Block->Next = Block->Next->Next;

        if (Block->Next != Null) {
            Block->Next->Prev = Block;
        }
    }

    return Block;
}

AllocBlock *Heap::FindBlock(AllocBlock *&Last, UIntPtr Size) {
    /* The block list always ends (or starts, if there are no blocks yet) on a Null pointer, so we can always just keep
     * on following the ->Next chain until we find a Null pointer. This time, we only have to check if there is at least
     * the size we were asked for, no need to check if there is space for the header. */

    if (!Size) {
        return Null;
    }

    AllocBlock *cur = Base;

    while (cur != Null && !(cur->Free && cur->Size >= Size)) {
        Last = cur;
        cur = cur->Next;
    }

    return cur;
}

AllocBlock *Heap::CreateBlock(AllocBlock *Last, UIntPtr Size) {
    /* The "Last" pointer is not required (for example, this may be the first time we're allocating memory, and this may
     * be the first allocation), but we do need to check if the Size is valid. */

    if (!Size) {
        return Null;
    }

    /* Here on the kernel, we can just increment the heap pointer (actually we also need to alloc and map, but the
     * Increment function takes care of that), a bit different from heap function of other OSes, we need to first get
     * the current location of the heap, and call the Increment function (that is going to return if the increment is
     * valid, and if it succeeded). */

    auto Block = reinterpret_cast<AllocBlock*>(Current);

    if (Increment(Size + sizeof(AllocBlock)) != Status::Success) {
        return Null;
    }

    Block->Magic = ALLOC_BLOCK_MAGIC;
    Block->Start = reinterpret_cast<UIntPtr>(Block) + sizeof(AllocBlock);
    Block->Size = Size;
    Block->Free = False;
    Block->Next = Null;
    Block->Prev = Last;

    if (Last != Null) {
        Last->Next = Block;
    }

    return Block;
}

Void *Heap::Allocate(UIntPtr Size) {
    /* With all the function that we wrote, now is just a question of checking if we need to create a new block, or
     * use/split an existing one. */

    Size = ((Size > 0 ? Size : 1) + 15) & -16;

    AllocBlock *last = Null, *blk = FindBlock(last, Size);

    if (blk != Null) {
        /* Let's not waste space, and split the block that we found in case it is too big. */

        if (blk->Size > Size + sizeof(AllocBlock)) {
            SplitBlock(blk, Size);
        }

        blk->Free = False;
    } else {
        /* There is one extra thing we may need to do after allocating the block: If this is the first allocation, we
         * need to set the Base pointer. */

        blk = CreateBlock(last, Size);

        if (blk != Null && Base == Null) {
            Base = blk;
        }
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

    AllocBlock *blk;
    auto ai = reinterpret_cast<UIntPtr>(Address), bi = reinterpret_cast<UIntPtr>(Base);

    ASSERT(Address != Null && ai >= bi + sizeof(AllocBlock) && ai <= Current);

    if (ai - 2 * sizeof(UIntPtr) >= bi + sizeof(AllocBlock) &&
        (reinterpret_cast<UIntPtr*>(Address))[-1] == ALLOC_BLOCK_MAGIC) {
        blk = reinterpret_cast<AllocBlock*>((reinterpret_cast<UIntPtr*>(Address))[-2]);
    } else {
        blk = reinterpret_cast<AllocBlock*>(ai - sizeof(AllocBlock));
    }

    /* Now, we need to check if this is a valid block, and if we haven't called Deallocate on it before (double
     * free). */

    ASSERT(blk->Magic == ALLOC_BLOCK_MAGIC);
    ASSERT(!blk->Free);

    /* Set that this is now a free block, and start fusing it with the free blocks that are around. */

    blk->Free = True;

    while (blk->Prev != Null && blk->Prev->Free) {
        blk = FuseBlock(blk->Prev);
    }

    while (blk->Next != Null && blk->Next->Free) {
        FuseBlock(blk);
    }

    /* If this is the end of the block list, AND the block is just at the end of the heap, let's free some space on the
     * heap (this will actually free the physical memory). */

    if (blk->Next == Null && reinterpret_cast<UIntPtr>(blk) == Current - (blk->Size + sizeof(AllocBlock))) {
        if (blk->Prev != Null) {
            blk->Prev->Next = Null;
        } else {
            Base = Null;
        }

        Decrement(blk->Size + sizeof(AllocBlock));
    }
}
