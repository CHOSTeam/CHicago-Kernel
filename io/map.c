// File author is Ítalo Lima Marconato Matias
//
// Created on January 26 of 2020, at 11:22 BRT
// Last edited on February 02 of 2020, at 15:53 BRT

#include <chicago/alloc.h>
#include <chicago/mm.h>
#include <chicago/string.h>

List FsMappedFiles = { Null, Null, 0, False };

static PMmMappedFile FsGetFileMappingList(PFsNode file) {
	ListForeach(&FsMappedFiles, i) {																				// Let's foreach our mapping list
		if (((PMmMappedFile)i->data)->file == file) {
			return (PMmMappedFile)i->data;																			// We found it!
		}
	}
	
	return Null;																									// Not found...
}

static Boolean FsAddFileMapping(PFsNode file, PMmRegion region) {
	PMmMappedFile list = FsGetFileMappingList(file);																// Try to get the file mapping list for the file
	Boolean free = False;
	
	if (list == Null) {
		list = (PMmMappedFile)MemAllocate(sizeof(MmMappedFile));													// Oh, we need to create it...
		
		if (list == Null) {
			return False;																							// Failed...
		}
		
		if (!ListAdd(&FsMappedFiles, list)) {																		// Try to add it
			MemFree((UIntPtr)list);																					// And we failed...
			return False;
		}
		
		StrSetMemory(&list->mappings, 0, sizeof(List));																// Clean the mapping list of the file
		free = True;
	}
	
	if (!ListAdd(&list->mappings, region)) {																		// Try to add us
		UIntPtr idx;																								// Failed, if required, try to remove the file mappings list
		
		if (free && ListSearch(&FsMappedFiles, list, &idx)) {
			ListRemove(&FsMappedFiles, idx);																		// Ok, remove it!
		}
		
		return False;
	}
	
	return True;
}

static Void FsRemoveFileMapping(PFsNode file, PMmRegion region) {
	PMmMappedFile list = FsGetFileMappingList(file);																// Try to get the file mapping list for the file
	UIntPtr idx;
	
	if (list == Null) {
		return;																										// ...
	} else if (ListSearch(&list->mappings, region, &idx)) {															// Try to find the memory region
		ListRemove(&list->mappings, idx);																			// And remove it
	}
	
	if (list->mappings.length == 0 && ListSearch(&FsMappedFiles, list, &idx)) {										// Do we have any more regions on this list? No? So we can remove it
		ListRemove(&FsMappedFiles, idx);
	}
}

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
	} else if ((region->flgs & MM_FLAGS_PRIVATE) != MM_FLAGS_PRIVATE && !FsAddFileMapping(file, region)) {			// Try to add the mapping to the list
		return STATUS_OUT_OF_MEMORY;
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
	
	Status status = MmSyncMemory(start, start + region->key.size, True);											// Sync
	
	if (status != STATUS_SUCCESS) {
		return status;
	}
	
	FsRemoveFileMapping(region->file, region);																		// Remove the mapping from the list
	MmPrepareUnmapFile(start, start + region->key.size);															// Set everything to AOR
	
	region->file = Null;
	region->off = 0;
	
	return STATUS_SUCCESS;
}
