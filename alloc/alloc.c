// File author is Ítalo Lima Marconato Matias
//
// Created on July 13 of 2018, at 00:44 BRT
// Last edited on February 02 of 2020, at 13:02 BRT

#include <chicago/alloc.h>
#include <chicago/alloc-int.h>
#include <chicago/heap.h>
#include <chicago/mm.h>
#include <chicago/string.h>

PAllocBlock MemAllocateBase = Null;

static Void MemAllocateSplitBlock(PAllocBlock block, UIntPtr size) {
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

static PAllocBlock MemAllocateFuseBlock(PAllocBlock block) {
	if ((block->next != Null) && (((UIntPtr)block->next) == (block->start + block->size)) && (block->next->free)) {
		block->size += sizeof(AllocBlock) + block->next->size;
		block->next = block->next->next;
		
		if (block->next != Null) {
			block->next->prev = block;
		}
	}
	
	return block;
}

static PAllocBlock MemAllocateFindBlock(PAllocBlock *last, UIntPtr size) {
	PAllocBlock block = MemAllocateBase;
	
	while ((block != Null) && (!((block->free) && (block->size >= size)))) {													// Check if the block is free and if it's equal or greater than specified size
		*last = block;																											// YES!
		block = block->next;
	}
	
	return block;
}

static PAllocBlock MemAllocateCreateBlock(PAllocBlock last, UIntPtr size) {
	if (!size) {																												// We need a size...
		return Null;
	}
	
	PAllocBlock block = (PAllocBlock)HeapGetCurrent();																			// Let's try to expand the heap!
	
	if (!HeapIncrement(size + sizeof(AllocBlock))) {
		return Null;																											// Failed...
	}
	
	block->size = size;
	block->start = ((UIntPtr)block) + sizeof(AllocBlock);
	block->free = False;
	block->next = Null;
	block->prev = last;
	
	if (block->size > (size + sizeof(AllocBlock))) {																			// Split this block if we can
		MemAllocateSplitBlock(block, size);
	}
	
	if (last != Null) {
		last->next = block;
	}
	
	return block;
}

UIntPtr MemAllocate(UIntPtr size) {
	if (size == 0) {
		return 0;
	}
	
	PsLockTaskSwitch(old);																										// Lock task switching
	
	PAllocBlock block = Null;
	
	if ((size % sizeof(UIntPtr)) != 0) {																						// Align size to UIntPtr
		size += sizeof(UIntPtr) - (size % sizeof(UIntPtr));
	}
	
	if (MemAllocateBase != Null) {
		PAllocBlock last = MemAllocateBase;
		
		block = MemAllocateFindBlock(&last, size);
		
		if (block != Null) {																									// Found?
			if (block->size > (size + sizeof(AllocBlock))) {																	// Yes! Let's try to split it (if we can)
				MemAllocateSplitBlock(block, size);
			}
			
			block->free = False;																								// And... NOW THIS BLOCK BELONG TO US
		} else {
			block = MemAllocateCreateBlock(last, size);																			// No, so let's (try to) create a new block
			
			if (block == Null) {
				PsUnlockTaskSwitch(old);																						// Failed...
				return 0;
			}
		}
	} else {
		block = MemAllocateBase = MemAllocateCreateBlock(Null, size);															// Yes, so let's (try to) init
		
		if (block == Null) {
			PsUnlockTaskSwitch(old);																							// Failed...
			return 0;
		}
	}
	
	PsUnlockTaskSwitch(old);																									// Unlock task switch
	
	return block->start;
}

UIntPtr MemAAllocate(UIntPtr size, UIntPtr align) {
	if (size == 0) {																											// Some checks...
		return 0;
	} else if (align == 0) {
		return 0;
	} else if ((align & (align - 1)) != 0) {
		return 0;
	}
	
	PsLockTaskSwitch(old);																										// Lock task switching
	
	UIntPtr p0 = MemAllocate(size + align);																						// Alloc the memory
	
	if (p0 == 0) {
		PsUnlockTaskSwitch(old);																								// Failed/Out of memory :(
		return 0;
	}
	
	MemFree(p0);																												// Free the allocated memory
	
	UIntPtr p1 = MemAllocate(size + align + ((p0 + (align - (p0 % align))) - p0));												// Now, let's alloc the real amount of bytes that we're going to use
	
	if (p1 == 0) {
		PsUnlockTaskSwitch(old);																								// Failed
		return 0;
	}
	
	PUIntPtr p2 = (PUIntPtr)(p1 + (align - (p1 % align)));
	p2[-1] = p1;
	
	PsUnlockTaskSwitch(old);																									// Unlock task switching
	
	return (UIntPtr)p2;
}

Void MemFree(UIntPtr block) {
	if (block == 0) {																											// Some checks...
		return;
	} else if (block < ((UIntPtr)MemAllocateBase) + sizeof(AllocBlock)) {
		return;
	} else if (block >= HeapGetCurrent()) {
		return;
	}
	
	PsLockTaskSwitch(old);																										// Lock task switching
	
	PAllocBlock blk = (PAllocBlock)(block - sizeof(AllocBlock));																// Let's get the block struct
	
	if (blk->free) {																											// Make sure that we haven't freed it before
		PsUnlockTaskSwitch(old);
		return;
	}
	
	blk->free = True;
	
	if ((blk->prev != Null) && (blk->prev->free)) {																				// Fuse with the prev?
		blk = MemAllocateFuseBlock(blk->prev);																					// Yes!
	}
	
	if (blk->next != Null) {																									// Fuse with the next?
		MemAllocateFuseBlock(blk);																								// Yes!
	} else {
		if (blk->prev != Null) {																								// No, so let's free the end of the heap
			blk->prev->next = Null;
		} else {
			MemAllocateBase = Null;																								// No more blocks!
		}
		
		HeapDecrement(blk->size + sizeof(AllocBlock));																			// Now let's decrement the heap!
	}
	
	PsUnlockTaskSwitch(old);																									// Unlock task switching
}

Void MemAFree(UIntPtr block) {
	MemFree(((PUIntPtr)block)[-1]);
}

UIntPtr MemZAllocate(UIntPtr size) {
	UIntPtr ret = MemAllocate(size);
	
	if (ret) {
		StrSetMemory((PVoid)ret, 0, size);
	}
	
	return ret;
}

UIntPtr MemReallocate(UIntPtr block, UIntPtr size) {
	if ((block == 0) || (size == 0)) {
		return 0;
	} else if (block < ((UIntPtr)MemAllocateBase) + sizeof(AllocBlock)) {
		return 0;
	} else if (block >= HeapGetCurrent()) {
		return 0;
	} else if ((size % sizeof(UIntPtr)) != 0) {
		size += sizeof(UIntPtr) - (size % sizeof(UIntPtr));
	}
	
	PAllocBlock blk = (PAllocBlock)(block - sizeof(AllocBlock));
	UIntPtr new = MemAllocate(size);
	
	if (new == 0) {
		return 0;
	}
	
	StrCopyMemory((PUInt8)(((PAllocBlock)(new - sizeof(AllocBlock)))->start), (PUInt8)blk->start, (blk->size > size) ? size : blk->size);
	MemFree(block);
	
	return new;
}

UIntPtr MemGetAllocSize(UIntPtr block) {
	if (block == 0) {																											// Sanity check	
		return 0;
	}
	
	return ((PAllocBlock)(block - sizeof(AllocBlock)))->size;																	// Return the size
}
