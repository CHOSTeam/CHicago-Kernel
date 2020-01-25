// File author is Ítalo Lima Marconato Matias
//
// Created on January 25 of 2020, at 13:35 BRT
// Last edited on January 25 of 2020, at 14:13 BRT

#include <chicago/alloc.h>
#include <chicago/mm.h>
#include <chicago/string.h>

PAvlNode MmGetMapping(UIntPtr addr, UIntPtr size, Boolean exact) {
	MmKey key = { addr, size };																													// Create the key struct on the stack
	PAvlNode node = AvlSearch(PsCurrentThread == Null ? Null : &PsCurrentProcess->mappings, &key);												// And search for a mapping that overlaps the specified region
	
	if (node == Null) {
		return Null;																															// Not found...
	}
	
	return exact && ((PMmKey)node->key)->start != addr ? Null : node;																			// If the user said that only want a exact match, we need to check if this is a exact match
}

static UInt32 MmConvertFlags(UInt32 flags) {
	if (flags == 0) {																															// Invalid?
		return MM_MAP_USER;																														// Yes, just return MM_MAP_USER...
	}
	
	UInt32 ret = MM_MAP_USER;																													// Let's convert the flags!
	
	if ((flags & MM_FLAGS_READ) == MM_FLAGS_READ) {
		ret |= MM_MAP_READ;
	}
	
	if ((flags & MM_FLAGS_WRITE) == MM_FLAGS_WRITE) {
		ret |= MM_MAP_WRITE;
	}
	
	if ((flags & MM_FLAGS_EXEC) == MM_FLAGS_EXEC) {
		ret |= MM_MAP_EXEC;
	}
				  
	if ((flags & MM_FLAGS_ALLOC_NOW) != MM_FLAGS_ALLOC_NOW) {
		ret |= MM_MAP_AOR;
	}
	
	return ret;
}

Status MmMapMemory(PWChar name, UIntPtr virt, PUIntPtr phys, UIntPtr size, UInt32 flags) {
	if (virt == 0 || virt + size >= MM_USER_END || (phys == Null && (flags & MM_FLAGS_ALLOC_NOW) == MM_FLAGS_ALLOC_NOW)) {						// The virtual address should NEVER be zero/outside of the userspace, and the physical address array shouldn't be a Null pointer (unless we're going to AOR the virt)
		return STATUS_INVALID_ARG;
	} else if (PsCurrentThread == Null) {																										// Oh, also, tasking should be initialized before calling us
		return STATUS_INVALID_ARG;
	} else if ((virt & ~MM_PAGE_MASK) != 0 || (size & ~MM_PAGE_MASK) != 0) {																	// Check if the virtual address and the size are page aligned
		return STATUS_INVALID_ARG;
	} else if (MmGetMapping(virt, size, False) != Null) {																						// Check if we don't overlap with some other allocation
		return STATUS_ALREADY_MAPPED;
	}
	
	for (UIntPtr i = 0; phys != Null && i < size >> MM_PAGE_SIZE_SHIFT; i++) {																	// Now, let's check all the physical addresses
		if ((phys[i] == 0 || (phys[i] & ~MM_PAGE_MASK) != 0) && (flags & MM_FLAGS_ALLOC_NOW) != MM_FLAGS_ALLOC_NOW) {							// The physical address can't be zero on alloc now, and need to be page aligned...
			return STATUS_INVALID_ARG;
		} else if (phys[i] != 0 && (flags & MM_FLAGS_ALLOC_NOW) != MM_FLAGS_ALLOC_NOW) {														// And it NEEDS to be zero on alloc now...
			return STATUS_INVALID_ARG;
		}
	}
	
	PMmRegion region = (PMmRegion)MemAllocate(sizeof(MmRegion));																				// Create the region struct
	
	if (region == Null) {
		return STATUS_OUT_OF_MEMORY;
	}
	
	PWChar nam = name == Null ? Null : StrDuplicate(name);																						// Duplicate the name
	
	if (name != Null && nam == Null) {
		MemFree((UIntPtr)region);																												// And it failed...
		return STATUS_OUT_OF_MEMORY;
	}
	
	UInt32 flgs = MmConvertFlags(flags);																										// Convert the flags
	
	for (UIntPtr i = 0; i < size; i += MM_PAGE_SIZE) {																							// Now, let's try mapping all the addresses
		Status status = MmMap(virt + i, phys == Null ? 0 : phys[i >> MM_PAGE_SIZE_SHIFT], flgs);
		
		if (status != STATUS_SUCCESS) {
			for (UIntPtr j = 0; j < i; j += MM_PAGE_SIZE) {																						// Oh, it failed... unmap everything...
				MmUnmap(virt + j);
			}
			
			MemFree((UIntPtr)nam);
			MemFree((UIntPtr)region);
			
			return status;
		}
	}
	
	region->key.start = virt;																													// Set the start and the end of the region
	region->key.size = size;
	region->name = nam;																															// Set the name
	region->file = Null;																														// No file is mapped yet
	region->prot = flags & MM_FLAGS_PROT;																										// Save the protection
	
	if (!AvlInsert(&PsCurrentProcess->mappings, &region->key, region)) {																		// And insert the region
		for (UIntPtr i = 0; i < size; i += MM_PAGE_SIZE) {																						// Oh, it failed... unmap everything...
			MmUnmap(virt + i);
		}
		
		MemFree((UIntPtr)nam);
		MemFree((UIntPtr)region);
		
		return STATUS_OUT_OF_MEMORY;
	}
	
	return STATUS_SUCCESS;
}

static Int MmMapCompare(PVoid a, PVoid b) {
	PMmKey ka = a;																																// Convert the void pointers into mm key pointers
	PMmKey kb = b;
	
	if (kb->size == 0) {																														// Ok, are we just checking if the address kb->start is inside of the region ka?
		if (kb->start >= ka->start && kb->start < ka->start + ka->size) {																		// Yeah... is it inside?
			return 0;																															// Yup!
		}
		
		return kb->start < ka->start ? -1 : 1;																									// No... return kb->start is greater or lower than the ka region
	} else if (ka->start < kb->start && ka->start + ka->size <= kb->start) {																	// Is the ka region before the kb region?
		return -1;
	} else if (ka->start >= kb->start + kb->size) {																								// After the kb region?
		return 1;
	}
	
	return 0;																																	// Oh, THEY OVERLAP!
}

Void MmInitMappingTree(PProcess proc) {
	StrSetMemory((PUInt8)&proc->mappings, 0, sizeof(AvlTree));																					// Clean the avl tree
	proc->mappings.compare = MmMapCompare;																										// And set the compare function
}
