/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 04 of 2020, at 23:14 BRT
 * Last edited on August 02 of 2020, at 16:44 BRT */

#include <chicago/arch.hxx>
#include <chicago/mm.hxx>
#include <chicago/textout.hxx>

/* We need to init the static variables. */

UIntPtr Heap::Start = 0, Heap::End = 0, Heap::Current = 0, Heap::CurrentAligned = 0;
AllocBlock *Heap::AllocBase = Null;

Void Heap::Initialize(UIntPtr Start, UIntPtr End) {
	/* Just setup all the variables, but remember that we have to page align everything, as Arch::Map expects
	 * everything to be aligned (well, it is actually going to manually align it, but whatever). */
	
	Heap::Start = Current = CurrentAligned = Start & MM_PAGE_MASK;
	Heap::End = End & MM_PAGE_MASK;
}

Status Heap::Increment(UIntPtr Amount) {
	/* We need to check for two things: First, if the Amount is even valid (if it isn't, return InvalidArg),
	 * second, if we aren't trying to expand beyond the heap limit. */
	
	if (Amount == 0) {
		return Status::InvalidArg;
	} else if ((Current + Amount) < Start || (Current + Amount) >= End) {
		return Status::OutOfMemory;
	}
	
	/* Now, we need to expand the heap, but this is not just a matter of increasing the Heap::Current variable,
	 * we need to make sure that the space we're expanding into is going to be mapped into the memory. */
	
	UIntPtr nw = Current + Amount;
	Status status;
	
	for (; CurrentAligned < nw; CurrentAligned += MM_PAGE_SIZE) {
		UIntPtr phys;
		
		/* The process to expand one page is: Alloc a new physical page, if it fails, free everything that we
		 * allocated in this session/call and return what the error was. Try to map said physical page, if it
		 * fails, free everything just as before, and make sure that the physical page will also be freed. */
		
		if ((status = PhysMem::ReferenceSingle(0, phys)) != Status::Success) {
			return status;
		} else if ((status = Arch->Map((Void*)CurrentAligned, phys, MM_MAP_KDEF)) != Status::Success) {
			PhysMem::DereferenceSingle(phys);
			return status;
		}
	}
	
	Current = nw;
	
	return Status::Success;
}

Status Heap::Decrement(UIntPtr Amount) {
	/* Again, let's do the exact same checks as we did in the Increment function, but this time, we need to
	 * check if the Amount big enough to underflow (and go before the heap start). */
	
	if (Amount == 0 || (Current - Amount) < Start || (Current - Amount) >= End) {
		return Status::InvalidArg;
	}
	
	/* Now, we need to decrease all the variables, and free the physical pages that we used. */
	
	Current -= Amount;
	
	for (UIntPtr p; CurrentAligned - MM_PAGE_SIZE >= Current;) {
		CurrentAligned -= MM_PAGE_SIZE;
		
		if (Arch->GetPhys((Void*)CurrentAligned, p) == Status::Success) {
			PhysMem::DereferenceSingle(p);
		}
		
		Arch->Unmap((Void*)CurrentAligned);
	}
	
	return Status::Success;
}

Void Heap::SplitBlock(AllocBlock *Block, UIntPtr Size) {
	/* We probably could skip the Null pointer checks, as the only ones that are going to call this (and the other
	 * private functions) is ourselves, but let's do it anyways. We also need to check if the block is big enough
	 * to be split, it needs at least the size of the new block we want to create + the size of the alloc header. */
	
	if (Block == Null || Size == 0 || Block->Size <= Size + sizeof(AllocBlock)) {
		return;
	}
	
	/* Let's split the block so the current block have exactly the same that the caller asked for, and the new
	 * block is the remainder. */
	
	AllocBlock *nblk = (AllocBlock*)(Block->Start + Size);
	
	nblk->Magic = ALLOC_BLOCK_MAGIC;
	nblk->Size = Block->Size - (Size + sizeof(AllocBlock));
	nblk->Start = Block->Start + Size + sizeof(AllocBlock);
	nblk->Free = True;
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
	/* We can only fuse blocks that are just after ourselves, to fuse blocks that are in the ->Prev field, the
	 * caller should call the FuseBlock function on the ->Prev field, instead of the current block. */
	
	if (Block != Null && Block->Next != Null &&
		(UIntPtr)Block->Next == Block->Start + Block->Size && Block->Next->Free) {
		Block->Size += sizeof(AllocBlock) + Block->Next->Size;
		Block->Next = Block->Next->Next;
		
		if (Block->Next != Null) {
			Block->Next->Prev = Block;
		}
	}
	
	return Block;
}

AllocBlock *Heap::FindBlock(AllocBlock *&Last, UIntPtr Size) {
	/* The block list always ends (or starts, if there are no blocks yet) on a Null pointer, so we can always
	 * just keep on following the ->Next chain untill we find a Null pointer. This time, we only have to check
	 * if there is at least the size we were asked for, no need to check if there is space for the header. */
	
	if (Size == 0) {
		return Null;
	}
	
	AllocBlock *cur = AllocBase;
	
	while (cur != Null && !(cur->Free && cur->Size >= Size)) {
		Last = cur;
		cur = cur->Next;
	}
	
	return cur;
}

AllocBlock *Heap::CreateBlock(AllocBlock *Last, UIntPtr Size) {
	/* The "Last" pointer is not required (for example, this may be the first time we're allocating memory,
	 * and this may be the first allocation), but we do need to check if the Size is valid. */
	
	if (Size == 0) {
		return Null;
	}
	
	/* Here on the kernel, we can just increment the heap pointer (actually we also need to alloc and map,
	 * but the Increment function takes care of that), a bit different from heap function of other OSes,
	 * we need to first get the current location of the heap, and call the Increment function (that is
	 * going to return if the increment is valid, and if it succedded). */
	
	AllocBlock *Block = (AllocBlock*)Current;
	
	if (Increment(Size + sizeof(AllocBlock)) != Status::Success) {
		return Null;
	}
	
	Block->Magic = ALLOC_BLOCK_MAGIC;
	Block->Size = Size;
	Block->Start = (UIntPtr)Block + sizeof(AllocBlock);
	Block->Free = False;
	Block->Next = Null;
	Block->Prev = Last;
	
	if (Last != Null) {
		Last->Next = Block;
	}
	
	return Block;
}

Void *Heap::Allocate(UIntPtr Size) {
	/* With all the function that we wrote, now is just a question of checking if we need to create a new
	 * block, or use/split an existing one. */
	
	Size = ((Size > 0 ? Size : 1) + 15) & -16;
	
	AllocBlock *last = Null, *blk = FindBlock(last, Size);
	
	if (blk != Null) {
		/* Let's not waste space, and split the block that we found in case it is too big. */
		
		if (blk->Size > Size + sizeof(AllocBlock)) {
			SplitBlock(blk, Size);
		}
		
		blk->Free = False;
	} else {
		/* There is one extra thing we may need to do after allocating the block: If this is the first
		 * allocation, we need to set the AllocBase. */
		
		blk = CreateBlock(last, Size);
		
		if (blk != Null && AllocBase == Null) {
			AllocBase = blk;
		}
	}
	
	/* If everything went well, we can zero the allocated memory (just to be safe) and return, else, we
	 * should return a null pointer. */
	
	if (blk != Null) {
		StrSetMemory((Void*)blk->Start, 0, Size);
		return (Void*)blk->Start;
	}
	
	return Null;
}

Void *Heap::Allocate(UIntPtr Size, UIntPtr Align) {
	/* Just as in the other allocate function, the size needs to be valid and aligned, but now we also
	 * need to check if the Align value is valid (it needs to be a power of 2). */
	
	if (Size == 0 || Align == 0 || (Align & (Align - 1))) {
		return Null;
	}
	
	/* Let's over-alloc the space we need, so we can save that this is an aligned allocation, and the
	 * address of the block header. */
	
	UIntPtr tsz = Size + 2 * sizeof(UIntPtr) + (Align - 1);
	
	Void *p0 = Allocate(tsz);
	
	if (p0 == Null) {
		return Null;
	}
	
	/* Now, we just need to get the actual aligned start (the part that we're going to return). We can
	 * access it like it was a pointer, to save what we need. */
	
	UIntPtr *p1 = (UIntPtr*)(((UIntPtr)p0 + tsz) & -Align);
	
	p1[-1] = ALLOC_BLOCK_MAGIC;
	p1[-2] = (UIntPtr)p0 - sizeof(AllocBlock);
	
	return p1;
}

Void Heap::Deallocate(Void *Address) {
	/* We need to check if the specfied address is valid, and is inside of the kernel heap (from the
	 * start until the current highest allocated address), after that, we need to check if this is
	 * an aligned allocation, if it is, we need to convert the address into an UIntPtr pointer, and
	 * access ptr[-2] to get the header location, else, we just subtract the size of the header from
	 * the address. */
	
	if (Address == Null ||
		(UIntPtr)Address < (UIntPtr)AllocBase + sizeof(AllocBlock) || (UIntPtr)Address >= Current) {
#ifdef DBG
		Debug->Write("[Heap] Tried to ::Deallocate invalid address 0x%x\n", Address);
#endif
		return;
	}
	
	AllocBlock *blk;
	
	if ((UIntPtr)Address - 2 * sizeof(UIntPtr) >= (UIntPtr)AllocBase + sizeof(AllocBlock) &&
		((UIntPtr*)Address)[-1] == ALLOC_BLOCK_MAGIC) {
		blk = (AllocBlock*)(((UIntPtr*)Address)[-2]);
	} else {
		blk = (AllocBlock*)((UIntPtr)Address - sizeof(AllocBlock));
	}
	
	/* Now, we need to check if this is a valid block, and if we haven't called Deallocate on it
	 * before (double free). */
	
#ifdef DBG
	if (blk->Magic != ALLOC_BLOCK_MAGIC) {
		Debug->Write("[Heap] Invalid allocation block for the address (got the magic number 0x%x)\n",
					 Address, blk->Magic);
		return;
	} else if (blk->Free) {
		Debug->Write("[Heap] Tried to double ::Deallocate the address 0x%x\n", Address);
		return;
	}
#else
	if (blk->Magic != ALLOC_BLOCK_MAGIC || blk->Free) {
		return;
	}
#endif
	
	/* Set that this is now a free block, and start fusing it with the free blocks that are
	 * around. */
	
	blk->Free = True;
	
	while (blk->Prev != Null && blk->Prev->Free) {
		blk = FuseBlock(blk->Prev);
	}
	
	while (blk->Next != Null && blk->Next->Free) {
		FuseBlock(blk);
	}
	
	/* If this is the end of the block list, AND the block is just at the end of the heap,
	 * let's free some space on the heap (this will actually free the physical memory). */
	
	if (blk->Next == Null &&
		(UIntPtr)blk == Current - (blk->Size + sizeof(AllocBlock))) {
		if (blk->Prev != Null) {
			blk->Prev->Next = Null;
		} else {
			AllocBase = Null;
		}
		
		Decrement(blk->Size + sizeof(AllocBlock));
	}
}
