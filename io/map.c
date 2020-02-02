// File author is Ítalo Lima Marconato Matias
//
// Created on January 26 of 2020, at 11:22 BRT
// Last edited on January 26 of 2020, at 13:31 BRT

#include <chicago/mm.h>

Status FsMapFile(PFsNode file, UIntPtr start, UIntPtr off) {
	if (file == Null || start == 0 || (start & ~MM_PAGE_MASK) != 0) {												// The virtual address should NEVER be zero/outside of the userspace, and should be page aligned
		return STATUS_INVALID_ARG;
	} else if (PsCurrentThread == Null) {																			// Oh, also, tasking should be initialized before calling us
		return STATUS_INVALID_ARG;
	} else if ((file->flags & FS_FLAG_FILE) != FS_FLAG_FILE) {														// It need to be a file
		return STATUS_NOT_FILE;
	}
	
	PAvlNode node = MmGetMapping(start, 0, True);																	// Try to get the mapping...
	
	if (node == Null) {
		return STATUS_NOT_MAPPED;																					// Not mapped, or this isn't the exact start address...
	}
	
	PMmRegion region = node->value;																					// Convert the void pointer into the region pointer
	
	if (region->file != Null) {																						// Make sure that this region isn't already mapped to another file
		return STATUS_ALREADY_MAPPED;
	}
	
	region->file = file;																							// Set the file
	region->off = off;																								// Set the file offset
	
	MmPrepareMapFile(region->key.start, region->key.start + region->key.size);										// Prepare the region
	
	return STATUS_SUCCESS;																							// And this is the end!
}

Status FsUnmapFile(UIntPtr start) {
	if (start == 0 || start >= MM_USER_END || (start & ~MM_PAGE_MASK) != 0) {										// The virtual address should NEVER be zero/outside of the userspace, and it should be page aligned
		return STATUS_INVALID_ARG;
	} else if (PsCurrentThread == Null) {																			// Oh, also, tasking should be initialized before calling us
		return STATUS_INVALID_ARG;
	}
	
	PAvlNode node = MmGetMapping(start, 0, True);																	// Try to get the mapping...
	
	if (node != Null) {
		return STATUS_NOT_MAPPED;																					// Not mapped, or this isn't the exact start address...
	}
	
	PMmRegion region = node->value;																					// Convert the void pointer into the region pointer
	
	if (region->file == Null) {
		return STATUS_NOT_MAPPED;																					// Not mapped to a file
	}
	
	Status status = MmSyncMemory(start, start + region->key.size);													// Sync
	
	if (status != STATUS_SUCCESS) {
		return status;
	}
	
	MmPrepareUnmapFile(start, start + region->key.size);															// Set everything to AOR
	
	region->file = Null;
	region->off = 0;
	
	return STATUS_SUCCESS;
}
