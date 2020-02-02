// File author is Ítalo Lima Marconato Matias
//
// Created on January 26 of 2020, at 18:01 BRT
// Last edited on February 02 of 2020, at 11:06 BRT

#include <chicago/alloc.h>
#include <chicago/mm.h>
#include <chicago/string.h>

static PMmVirtualRange MmFindRegion(UIntPtr addr, UIntPtr size, Boolean highest) {
	for (PMmVirtualRange node = PsCurrentProcess->vaddresses_head; node != Null; node = node->next) {						// Let's search on the free ranges/regions list!
		if (!highest && (node->start == addr + size)) {																		// Check if this is the head/tail of the region...
			return node;
		} else if (highest && (node->start + node->size == addr)) {
			return node;
		}
	}
	
	return Null;
}

static PMmVirtualRange MmTraverse(PMmVirtualRange start, UIntPtr size, Boolean highest) {
	for (; start != Null; highest ? (start = start->prev) : (start = start->next)) {										// Let's traverse the list (in the right order lol)
		if (start->size >= size) {																							// Check if we have enough memory in the region
			return start;
		}
	}
	
	return Null;
}

static PMmVirtualRange MmCreateRegion(UIntPtr start, UIntPtr size) {
	PMmVirtualRange node = (PMmVirtualRange)MemAllocate(sizeof(MmVirtualRange));											// Alloc space for the entry
	
	if (node == Null) {
		return Null;																										// Failed, just return a null pointer
	}
	
	node->start = start;																									// Init all the fields
	node->size = size;
	node->next = Null;
	node->prev = Null;
	
	return node;
}

static Boolean MmInsertRegion(PProcess proc, PMmVirtualRange node) {
	if (node == Null) {																										// Sanity check
		return False;
	}
	
	PMmVirtualRange last = proc->vaddresses_head;																			// Let's search for the parent entry for node
	Boolean first = True;
	
	while (last != Null && last->start < node->start) {
		last = last->next;
		first = False;
	}
	
	if (!first && last == Null) {																							// Are we the last entry?
		node->prev = proc->vaddresses_tail;																					// Yes, set the our prev to the old tail
		node->prev->next = node;																							// And set the next of the old tail to us
		proc->vaddresses_tail = node;																						// And now we are the new tail
		return True;
	} else if (last == Null) {																								// Are we the first entry?
		if (proc->vaddresses_head != Null) {																				// Yeah, but are we just a new head, or the actual first entry?
			node->next = proc->vaddresses_head;																				// Just a new head, set our next to the old head
			node->next->prev = node;																						// Set the prev of the old tail to us
			proc->vaddresses_head = node;																					// And now we are the head
		} else {
			proc->vaddresses_head = node;																					// Yes, we are! Set us as the head and the tail
			proc->vaddresses_tail = node;
		}
		
		return True;
	}
	
	node->prev = last;																										// Set our prev to the parent node
	node->next = last->next;																								// Set our next to the parent node next value
	last->next->prev = node;																								// Set us as the prev node of the child of the parent node
	last->next = node;																										// Set us as the next node after the parent
	
	return True;
}

static Void MmRemoveRegion(PMmVirtualRange node, Boolean free) {
	if (node == PsCurrentProcess->vaddresses_head) {																		// Is this the head?
		PsCurrentProcess->vaddresses_head = node->next;																		// Yeah, we are not anymore...
	}
	
	if (node == PsCurrentProcess->vaddresses_tail) {																		// Is this the tail?
		PsCurrentProcess->vaddresses_tail = node->prev;																		// Yeah, we are not anymore...
	}
	
	if (node->prev != Null) {																								// Set the prev node's next node to our next node
		node->prev->next = node->next;
	}
	
	if (node->next != Null) {																								// And set the next node's prev node to our prev node
		node->next->prev = node->prev;
	}
	
	if (free) {																												// And free (if we need to)
		MemFree((UIntPtr)node);
	}
}

static Boolean MmIsMergable(PMmVirtualRange middle, PMmVirtualRange node, Boolean reverse) {
	return reverse ? (middle->start == node->start + node->size) : (node->start == middle->start + middle->size);			// Just check if we are the head/tail of the region
}

static Void MmMergeRegion(PMmVirtualRange middle, PMmVirtualRange node, Boolean reverse) {
	while (node != Null && MmIsMergable(middle, node, reverse)) {															// Here we go...
		if (reverse) {																										// Ok, we can merge this, should we change the start address?
			middle->start -= node->size;																					// Yeah
		}
		
		middle->size += node->size;																							// Increase the size
		
		MmRemoveRegion(node, True);																							// Remove the node
		
		node = reverse ? middle->prev : middle->next;																		// And let's see if we can keep on merging...
	}
}

static UIntPtr MmFindFreeAddress(UIntPtr size, Boolean highest) {
	PMmVirtualRange node = Null;
	
	if (highest) {																											// The user asked us for the highest address?
		node = MmTraverse(PsCurrentProcess->vaddresses_tail, size, True);													// Yeah!
	} else {
		node = MmTraverse(PsCurrentProcess->vaddresses_head, size, False);													// Nah!
	}
	
	if (node == Null) {																										// We found something?
		return 0;																											// No, so just return 0...
	}
	
	UIntPtr ret = highest ? node->start + node->size - size : node->start;													// Get the start address that we are going to return
	
	if (!highest) {																											// Should we increase the start address of the region?
		node->start += size;																								// Yes...
	}
	
	node->size -= size;																										// Decrease the size
	
	if (node->size == 0) {																									// Should we remove the region?
		MmRemoveRegion(node, True);																							// Yeah
	} else if (!highest) {																									// Should we move the location of it?
		MmRemoveRegion(node, False);																						// Yeah, remove it
		MmInsertRegion(PsCurrentProcess, node);																				// And reinsert it...
	}
	
	return ret;
}

static Status MmFreeRegion(UIntPtr addr, UIntPtr size, Boolean highest) {
	PMmVirtualRange node = MmFindRegion(addr, size, highest);																// Try to see if this region already exists
	
	if (node == Null) {
		return MmInsertRegion(PsCurrentProcess, MmCreateRegion(addr, size)) ? STATUS_SUCCESS : STATUS_OUT_OF_MEMORY;		// It doesn't, create and insert a new region
	} else if (!highest) {																									// Should we change the start address of the region?
		node->start -= size;																								// Yeah
	}
	
	node->size += size;																										// Increase the size
	
	MmMergeRegion(node, node->prev, True);																					// Merge the node with the prev nodes (if possible)
	MmMergeRegion(node, node->next, False);																					// And with the next nodes (if possible)
	
	return STATUS_SUCCESS;
}

Status MmAllocAddress(PWChar name, UIntPtr size, UInt32 flags, PUIntPtr ret) {
	if (size == 0 || ret == Null || (size & ~MM_PAGE_MASK) != 0) {															// Make sure that we don't have a zero size, nor null pointers, and the size is page aligned
		return STATUS_INVALID_ARG;
	} else if (PsCurrentThread == Null) {																					// Oh, also, tasking should be initialized before calling us
		return STATUS_INVALID_ARG;
	}
	
	UIntPtr addr = MmFindFreeAddress(size, (flags & MM_FLAGS_HIGHEST) == MM_FLAGS_HIGHEST);									// Try to find a free address
	
	if (addr == 0) {
		return STATUS_OUT_OF_MEMORY;
	}
	
	UIntPtr phys[size >> MM_PAGE_SIZE_SHIFT];																				// Now, let's try to alloc the physical addresses (if we need to)
	
	if ((flags & MM_FLAGS_ALLOC_NOW) == MM_FLAGS_ALLOC_NOW) {
		StrSetMemory((PUInt8)phys, 0, (size >> MM_PAGE_SIZE_SHIFT) * sizeof(UIntPtr));										// Clean it
		
		Status status = MmReferenceNonContigPages(phys, size >> MM_PAGE_SIZE_SHIFT, phys);									// And alloc
		
		if (status != STATUS_SUCCESS) {
			MmFreeRegion(addr, size, (flags & MM_FLAGS_HIGHEST) == MM_FLAGS_HIGHEST);										// Failed, free everything and return
			return status;
		}
	}
	
	Status status = MmMapMemory(name, addr, (flags & MM_FLAGS_ALLOC_NOW) == MM_FLAGS_ALLOC_NOW ? phys : Null, size, flags);	// Map the memory
	
	if (status != STATUS_SUCCESS) {
		if ((flags & MM_FLAGS_ALLOC_NOW) == MM_FLAGS_ALLOC_NOW) {															// Failed, free everything
			MmDereferenceNonContigPages(phys, size >> MM_PAGE_SIZE_SHIFT);
		}
		
		MmFreeRegion(addr, size, (flags & MM_FLAGS_HIGHEST) == MM_FLAGS_HIGHEST);
		
		return status;
	}
	
	*ret = addr;																											// Success!
	
	return STATUS_SUCCESS;
}

Status MmFreeAddress(UIntPtr start) {
	if (start == 0 || start >= MM_USER_END || (start & ~MM_PAGE_MASK) != 0) {												// The virtual address should NEVER be zero/outside of the userspace, and it should be page aligned
		return STATUS_INVALID_ARG;
	} else if (PsCurrentThread == Null) {																					// Oh, also, tasking should be initialized before calling us
		return STATUS_INVALID_ARG;
	}
	
	PAvlNode node = MmGetMapping(start, 0, True);																			// First, let's try to get the mapping
	
	if (node == Null) {
		return STATUS_NOT_MAPPED;																							// It's not mapped...
	}
	
	UIntPtr size = ((PMmKey)node->key)->size;																				// It is! Get the size of the mapping
	Boolean highest = (((PMmRegion)node->value)->flgs & MM_FLAGS_HIGHEST) == MM_FLAGS_HIGHEST;								// And get if we should traverse our region list backwards
	
	Status status = MmUnmapMemory(start);																					// Unmap the memory
	
	if (status != STATUS_SUCCESS) {
		return status;																										// Failed...
	}
	
	return MmFreeRegion(start, size, highest);																				// Free the region on our list
}

Status MmInitVirtualAddressAllocTree(PProcess proc) {
	proc->vaddresses_head = Null;																							// Just set everything to Null
	proc->vaddresses_tail = Null;
	
#if MM_USER_START == 0																										// Is the start of the userspace at 0?
	Boolean res = MmInsertRegion(proc, MmCreateRegion(MM_PAGE_SIZE, MM_USER_END - MM_PAGE_SIZE));							// Yeah, but the first page is reserved...
#else
	Boolean res = MmInsertRegion(proc, MmCreateRegion(MM_USER_START, MM_USER_END - MM_USER_START));							// Nope, so just use the start as the actual start...
#endif
	
	return res ? STATUS_SUCCESS : STATUS_OUT_OF_MEMORY;
}
