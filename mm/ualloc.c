// File author is Ítalo Lima Marconato Matias
//
// Created on September 21 of 2018, at 20:40 BRT
// Last edited on January 05 of 2020, at 21:01 BRT

#include <chicago/alloc-int.h>
#include <chicago/mm.h>
#include <chicago/process.h>
#include <chicago/string.h>
#include <chicago/virt.h>

static Void MmAllocUserMemorySplitBlock(PAllocBlock block, UIntPtr size) {
	if ((block == Null) || (block->size <= (size + sizeof(AllocBlock)))) {
		return;
	}
	
	PAllocBlock new = (PAllocBlock)(block->start + size);
	
	new->size = block->size - (size + sizeof(AllocBlock));
	new->start = block->start + size + sizeof(AllocBlock);
	new->free = True;
	new->next = block->next;
	new->prev = block;
	
	block->size = size;
	block->next = new;
	
	if (new->next != Null) {
		new->next->prev = new;
	}
}

static PAllocBlock MmAllocUserMemoryFuseBlock(PAllocBlock block) {
	if ((block->next != Null) && (((UIntPtr)block->next) == (block->start + block->size)) && (block->next->free)) {
		block->size += sizeof(AllocBlock) + block->next->size;
		block->next = block->next->next;
		
		if (block->next != Null) {
			block->next->prev = block;
		}
	}
	
	return block;
}

static PAllocBlock MmAllocUserMemoryFindBlock(PAllocBlock *last, UIntPtr size) {
	PAllocBlock block = PsCurrentProcess->alloc_base;
	
	while ((block != Null) && (!((block->free) && (block->size >= size)))) {													// Check if the block is free and if it's equal or greater than specified size
		*last = block;																											// YES!
		block = block->next;
	}
	
	return block;
}

static PAllocBlock MmAllocUserMemoryCreateBlock(PAllocBlock last, UIntPtr size) {
	if (!size) {																												// We need a size...
		return Null;
	}
	
	UIntPtr tsize = size + sizeof(AllocBlock);																					// Let's alloc aligned to the page size
	
	if ((tsize % MM_PAGE_SIZE) != 0) {
		tsize += MM_PAGE_SIZE - (size % MM_PAGE_SIZE);
	}
	
	PAllocBlock block = (PAllocBlock)VirtAllocAddress(0, tsize, VIRT_PROT_READ | VIRT_PROT_WRITE);								// Let's try to alloc some space!
	
	if (block == Null) {
		return Null;																											// Failed...
	}
	
	block->size = tsize - sizeof(AllocBlock);
	block->start = ((UIntPtr)block) + sizeof(AllocBlock);
	block->free = False;
	block->next = Null;
	block->prev = last;
	
	if (block->size > (size + sizeof(AllocBlock))) {																			// Split this block if we can
		MmAllocUserMemorySplitBlock(block, size);
	}
	
	if (last != Null) {
		last->next = block;
	}
	
	return block;
}

UIntPtr MmAllocUserMemory(UIntPtr size) {
	if (size == 0) {
		return 0;
	}
	
	PAllocBlock block = PsCurrentProcess->alloc_base;
	
	if ((size % sizeof(UIntPtr)) != 0) {																						// Align size to UIntPtr
		size += sizeof(UIntPtr) - (size % sizeof(UIntPtr));
	}
	
	if (PsCurrentProcess->alloc_base != Null) {
		PAllocBlock last = PsCurrentProcess->alloc_base;
		
		block = MmAllocUserMemoryFindBlock(&last, size);
		
		if (block != Null) {																									// Found?
			if (block->size > (size + sizeof(AllocBlock))) {																	// Yes! Let's try to split it (if we can)
				MmAllocUserMemorySplitBlock(block, size);
			}
			
			block->free = False;																								// And... NOW THIS BLOCK BELONG TO US
		} else {
			block = MmAllocUserMemoryCreateBlock(last, size);																	// No, so let's (try to) create a new block
			
			if (block == Null) {
				return 0;																										// Failed...
			}
		}
	} else {
		block = PsCurrentProcess->alloc_base = MmAllocUserMemoryCreateBlock(Null, size);										// Yes, so let's (try to) init
		
		if (block == Null) {
			return 0;
		}
	}
	
	return block->start;
}

UIntPtr MmAllocAlignedUserMemory(UIntPtr size, UIntPtr align) {
	if (size == 0) {																											// Some checks...
		return 0;
	} else if (align == 0) {
		return 0;
	} else if ((align & (align - 1)) != 0) {
		return 0;
	}
	
	UIntPtr p0 = MmAllocUserMemory(size + align);																				// Alloc the memory
	
	if (p0 == 0) {
		return 0;																												// Failed/Out of memory :(
	}
	
	MmFreeUserMemory(p0);																										// Free the allocated memory
	
	UIntPtr p1 = MmAllocUserMemory(size + align + ((p0 + (align - (p0 % align))) - p0));										// Now, let's alloc the real amount of bytes that we're going to use
	
	if (p1 == 0) {
		return 0;																												// Failed
	}
	
	PUIntPtr p2 = (PUIntPtr)(p1 + (align - (p1 % align)));
	p2[-1] = p1;
	
	return (UIntPtr)p2;
}

Void MmFreeUserMemory(UIntPtr addr) {
	if (addr == 0) {																											// Some checks...
		return;
	} else if ((addr == 0) || (addr >= MM_USER_END)) {
		return;
	}
	
	PAllocBlock blk = (PAllocBlock)(addr - sizeof(AllocBlock));																	// Let's get the block struct
	
	if (blk->free) {																											// Make sure that we haven't freed it before
		return;
	}
	
	blk->free = True;
	
	if ((blk->prev != Null) && (blk->prev->free)) {																				// Fuse with the prev?
		blk = MmAllocUserMemoryFuseBlock(blk->prev);																			// Yes!
	}
	
	if (blk->next != Null) {																									// Fuse with the next?
		MmAllocUserMemoryFuseBlock(blk);																						// Yes!
	} else {
		if (blk->prev != Null) {																								// No, so let's free the end of the heap
			blk->prev->next = Null;
		} else {
			PsCurrentProcess->alloc_base = Null;																				// No more blocks!
		}
		
		VirtFreeAddress((UIntPtr)blk, blk->size + sizeof(AllocBlock));															// Return the memory to the system!
	}
}

Void MmFreeAlignedUserMemory(UIntPtr block) {
	MmFreeUserMemory(((PUIntPtr)block)[-1]);
}

UIntPtr MmReallocUserMemory(UIntPtr addr, UIntPtr size) {
	if ((addr == 0) || (size == 0)) {
		return 0;
	} else if ((addr == 0) || (addr >= MM_USER_END)) {
		return 0;
	} else if ((size % sizeof(UIntPtr)) != 0) {
		size += sizeof(UIntPtr) - (size % sizeof(UIntPtr));
	}
	
	PAllocBlock blk = (PAllocBlock)(addr - sizeof(AllocBlock));
	UIntPtr new = MmAllocUserMemory(size);
	
	if (new == 0) {
		return 0;
	}
	
	StrCopyMemory((PUInt8)(((PAllocBlock)(new - sizeof(AllocBlock)))->start), (PUInt8)blk->start, (blk->size > size) ? size : blk->size);
	MmFreeUserMemory(addr);
	
	return new;
}

UIntPtr MmGetUserAllocSize(UIntPtr addr) {
	if ((addr == 0) || (addr >= MM_USER_END)) {																					// Sanity check	
		return 0;
	}
	
	return ((PAllocBlock)(addr - sizeof(AllocBlock)))->size;																	// Return the size
}
