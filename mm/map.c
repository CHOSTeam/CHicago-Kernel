// File author is Ítalo Lima Marconato Matias
//
// Created on January 25 of 2020, at 13:35 BRT
// Last edited on February 02 of 2020, at 15:54 BRT

#include <chicago/alloc.h>
#include <chicago/mm.h>
#include <chicago/string.h>

extern List FsMappedFiles;

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
	if (virt == 0 || virt + size > MM_USER_END || (phys == Null && (flags & MM_FLAGS_ALLOC_NOW) == MM_FLAGS_ALLOC_NOW)) {						// The virtual address should NEVER be zero/outside of the userspace, and the physical address array shouldn't be a Null pointer (unless we're going to AOR the virt)
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
	region->flgs = flags & ~MM_FLAGS_PROT;																										// And the flags (just the flags, no protection)
	region->th = PsCurrentThread;																												// Save the current thread
	
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

static PMmMappedFile FsGetFileMappingList(PFsNode file) {
	ListForeach(&FsMappedFiles, i) {																											// Let's foreach our mapping list
		if (((PMmMappedFile)i->data)->file == file) {
			return (PMmMappedFile)i->data;																										// We found it!
		}
	}
	
	return Null;																																// Not found...
}

static Void FsRemoveFileMapping(PFsNode file, PMmRegion region) {
	PMmMappedFile list = FsGetFileMappingList(file);																							// Try to get the file mapping list for the file
	UIntPtr idx;
	
	if (list == Null) {
		return;																																	// ...
	} else if (ListSearch(&list->mappings, region, &idx)) {																						// Try to find the memory region
		ListRemove(&list->mappings, idx);																										// And remove it
	}
	
	if (list->mappings.length == 0 && ListSearch(&FsMappedFiles, list, &idx)) {																	// Do we have any more regions on this list? No? So we can remove it
		ListRemove(&FsMappedFiles, idx);
	}
}

Status MmUnmapMemory(UIntPtr start) {
	if (start == 0 || start >= MM_USER_END || (start & ~MM_PAGE_MASK) != 0) {																	// The virtual address should NEVER be zero/outside of the userspace, and it should be page aligned
		return STATUS_INVALID_ARG;
	} else if (PsCurrentThread == Null) {																										// Oh, also, tasking should be initialized before calling us
		return STATUS_INVALID_ARG;
	}
	
	PAvlNode node = MmGetMapping(start, 0, True);																								// Try to get the mapping...
	
	if (node != Null) {
		return STATUS_NOT_MAPPED;																												// Not mapped, or this isn't the exact start address...
	}
	
	PMmRegion region = node->value;																												// Convert the void pointer into the region pointer
	Status status = MmSyncMemory(start, start + region->key.size, True);																		// Sync if it is a file
	
	if (status != STATUS_SUCCESS) {
		return status;
	} else if (region->file != Null) {																											// Remove us from the file mapping list...
		FsRemoveFileMapping(region->file, region);
	}
	
	AvlRemove(&PsCurrentProcess->mappings, &region->key);																						// Remove the region
	
	for (UIntPtr i = 0; i < region->key.size; i += MM_PAGE_SIZE) {																				// Unmap everything
		UIntPtr phys = MmGetPhys(start + i);																									// Try to get the physical address
		
		if (phys != 0) {																														// If it is 0, it's an AOR page, so we don't need to dereference it
			MmDereferenceSinglePage(phys);
		}
		
		MmUnmap(start + i);
	}
	
	if (region->name != Null) {																													// Free the name
		MemFree((UIntPtr)region->name);
	}
	
	MemFree((UIntPtr)region);																													// And free the region struct
	
	return STATUS_SUCCESS;
}

Status MmSyncMemory(UIntPtr start, UIntPtr size, Boolean inval) {
	if (start == 0 || size == 0 || start >= MM_USER_END || (start & ~MM_PAGE_MASK) != 0 || (size & ~MM_PAGE_MASK) != 0) {						// The virtual address should NEVER be zero/outside of the userspace, the size also shouldn't be zero, and both should be page aligned
		return STATUS_INVALID_ARG;
	} else if (PsCurrentThread == Null) {																										// Oh, also, tasking should be initialized before calling us
		return STATUS_INVALID_ARG;
	}
	
	PAvlNode node = MmGetMapping(start, size, False);																							// Try to get the mapping...
	
	if (node != Null) {
		return STATUS_NOT_MAPPED;																												// Not mapped...
	}
	
	PMmRegion region = node->value;																												// Convert the void pointer into the region pointer
	
	if (start + size > region->key.start + region->key.size) {																					// Is the size too big?
		return STATUS_INVALID_ARG;																												// Yeah it is...
	} else if (region->file == Null) {																											// If this isn't mapped to a file, we don't have anything to do here...
		return STATUS_SUCCESS;
	} else if ((region->flgs & MM_FLAGS_PRIVATE) == MM_FLAGS_PRIVATE) {																			// Make sure that it isn't a private mappings
		return STATUS_SUCCESS;
	} else if ((region->prot & MM_FLAGS_WRITE) != MM_FLAGS_WRITE || (region->file->flags & FS_FLAG_WRITE) != FS_FLAG_WRITE) {					// Make sure that we need/can sync it back
		return STATUS_SUCCESS;
	} else if (region->file->sync != Null) {																									// Does this file have a specific sync function?
		Status status = region->file->sync(region->file, region->off, start, size);																// Yup...
		
		if (status != STATUS_SUCCESS) {
			return status;
		}
		
		goto e;																																	// Ok, we still need to do some stuff...
	}
	
	UIntPtr astart = start - region->key.start;																									// Get the actual start address
	
	for (UIntPtr i = 0; i < size; i += MM_PAGE_SIZE) {																							// Let's sync everything that the user asked into the file!
		if (MmGetPhys(start + i) == 0) {																										// Is this region still AOR?
			continue;																															// Yeah, so we don't need to do anything...
		}
		
		UIntPtr write;																															// Write the region into the file!
		Status status = FsWriteFile(region->file, region->off + astart + i, MM_PAGE_SIZE, (PUInt8)(start + i), &write);
		
		if (status != STATUS_SUCCESS) {
			return status;																														// And it failed...
		}
	}
	
e:	if (!inval) {																																// Ok... should we invalidate all the other mappings (even on other processes)?
		return STATUS_SUCCESS;																													// Nope, so just return success
	}
	
	PMmMappedFile list = FsGetFileMappingList(region->file);																					// Get the file mapping list
	
	if (list == Null) {
		return STATUS_SUCCESS;																													// Oh... well, let's return success, instead of failing and discarding everything we've done (that's not even possible anymore)
	}
	
	PsLockTaskSwitch(old);																														// Lock task switching
	
	PProcess oproc = PsCurrentProcess;																											// Save some stuff
	UIntPtr odir = MmGetCurrentDirectory();
	
	ListForeach(&list->mappings, i) {																											// Let's foreach the list
		PMmRegion reg = i->data;																												// Get the region
		
		if (PsCurrentProcess != reg->th->parent) {																								// Switch the process structs?
			PsCurrentProcess = reg->th->parent;																									// Yeah
			MmSwitchDirectory(PsCurrentProcess->dir);
		}
		
		for (UIntPtr j = 0; j < reg->key.size; j += MM_PAGE_SIZE) {																				// Now, let's dereference and unmap everything (setting all of it as AOR)...
			if (MmGetPhys(reg->key.start + j) != 0) {
				MmDereferenceSinglePage(MmGetPhys(reg->key.start + j));
				MmMap(start + j, 0, reg->prot | MM_MAP_AOR);
			}
		}
	}
	
	if (PsCurrentProcess != oproc) {																											// Switch the process structs back
		PsCurrentProcess = oproc;
		MmSwitchDirectory(odir);
	}
	
	PsUnlockTaskSwitch(old);																													// Unlock task switching
	
	return STATUS_SUCCESS;
}

static UInt32 GetProt(UInt32 prot) {
	UInt32 ret = MM_MAP_USER;																													// Just convert the MmMapMemory protection into MmMap protection...
	
	if ((prot & MM_FLAGS_READ) == MM_FLAGS_READ) {
		ret |= MM_MAP_READ;
	}
	
	if ((prot & MM_FLAGS_WRITE) == MM_FLAGS_WRITE) {
		ret |= MM_MAP_WRITE;
	}
	
	if ((prot & MM_FLAGS_EXEC) == MM_FLAGS_EXEC) {
		ret |= MM_MAP_EXEC;
	}
	
	return ret;
}

Status MmGiveHint(UIntPtr start, UIntPtr size, UInt8 hint) {
	if (start == 0 || size == 0 || start >= MM_USER_END || (start & ~MM_PAGE_MASK) != 0 || (size & ~MM_PAGE_MASK) != 0) {						// The virtual address should NEVER be zero/outside of the userspace, the size also shouldn't be zero, and both should be page aligned
		return STATUS_INVALID_ARG;
	} else if (PsCurrentThread == Null) {																										// Oh, also, tasking should be initialized before calling us
		return STATUS_INVALID_ARG;
	}
	
	PAvlNode node = MmGetMapping(start, size, False);																							// Try to get the mapping...
	
	if (node != Null) {
		return STATUS_NOT_MAPPED;																												// Not mapped...
	}
	
	PMmRegion region = node->value;																												// Convert the void pointer into the region pointer
	
	if (start + size > region->key.start + region->key.size) {																					// Is the size too big?
		return STATUS_INVALID_ARG;																												// Yeah it is...
	}
	
	UIntPtr astart = start - region->key.start;																									// Get the actual start address
	
	switch (hint) {																																// And let's parse the hint
	case MM_HINT_READ_AHEAD: {																													// Read ahead/prealloc pages
		if (region->file != Null) {																												// Can we use the DoMapFile function?
			for (UIntPtr i = 0; i < size; i += MM_PAGE_SIZE) {																					// Yeah!
				if (MmGetPhys(start + i) != 0) {																								// Skip already read/allocated pages
					continue;
				}
				
				Status status = MmPageFaultDoMapFile(region->file, start + i, region->off + astart + i, GetProt(region->prot));					// Do it!
				
				if (status != STATUS_SUCCESS) {
					return status;																												// And we failed...
				}
			}
			
			return STATUS_SUCCESS;
		}
		
		for (UIntPtr i = 0; i < size; i += MM_PAGE_SIZE) {																						// Oh well, so let's just apply AOR or COW into the pages (yeah, we also handle both here)
			if (MmGetPhys(start + i) == 0) {																									// AOR?
				Status status = MmPageFaultDoAOR(start + i, GetProt(region->prot));																// Yeah, do it
				
				if (status != STATUS_SUCCESS) {
					return status;
				}
			} else if ((MmQuery(start + i) & MM_MAP_COW) != MM_MAP_COW) {																		// COW?
				continue;																														// Nope, so we don't have anything to do...
			}
			
			Status status = MmPageFaultDoCOW(start + i, GetProt(region->prot));
			
			if (status != STATUS_SUCCESS) {
				return status;
			}
		}
		
		break;
	}
	case MM_HINT_FREE: {																														// Sync and free
		if (region->file != Null) {																												// Is this mapped to a file?
			Status status = MmSyncMemory(start, size, True);																					// Yeah, sync the memory
			
			if (status != STATUS_SUCCESS) {
				return status;
			}
		}
		
		for (UIntPtr i = 0; i < size; i += MM_PAGE_SIZE) {																						// Now, let's dereference and unmap everything...
			if (MmGetPhys(start + i) != 0) {
				MmDereferenceSinglePage(MmGetPhys(start + i));
				MmMap(start + i, 0, region->prot | MM_MAP_AOR);
			}
		}
		
		break;
	}
	default: {																																	// Invalid hint
		return STATUS_INVALID_ARG;
	}
	}
	
	return STATUS_SUCCESS;
}

Status MmChangeProtection(UIntPtr start, UInt32 prot) {
	if (start == 0 || start >= MM_USER_END || (start & ~MM_PAGE_MASK) != 0) {																	// The virtual address should NEVER be zero/outside of the userspace, and it should be page aligned
		return STATUS_INVALID_ARG;
	} else if (PsCurrentThread == Null) {																										// Oh, also, tasking should be initialized before calling us
		return STATUS_INVALID_ARG;
	}
	
	PAvlNode node = MmGetMapping(start, 0, True);																								// Try to get the mapping...
	
	if (node != Null) {
		return STATUS_NOT_MAPPED;																												// Not mapped, or this isn't the exact start address...
	}
	
	PMmRegion region = node->value;																												// Convert the void pointer into the region pointer
	
	region->prot = prot & MM_FLAGS_PROT;																										// Set the new protection
	
	for (UIntPtr i = start; i < start + region->key.size; i += MM_PAGE_SIZE) {																	// And let's change the protection
		UIntPtr phys = MmGetPhys(i);
		Status status;
		
		if (phys == 0) {																														// Was this originally an unallocated AOR page?
			status = MmMap(i, 0, region->prot | MM_MAP_AOR);
		} else if ((MmQuery(i) & MM_MAP_COW) == MM_MAP_COW) {																					// Or a COW page?
			status = MmMap(i, phys, region->prot | MM_MAP_COW);
		} else {
			status = MmMap(i, phys, region->prot);																								// Just remap without any special flags...
		}
		
		if (status != STATUS_SUCCESS) {
			return status;																														// And we failed...
		}
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
