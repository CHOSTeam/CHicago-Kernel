// File author is Ítalo Lima Marconato Matias
//
// Created on December 25 of 2019, at 10:55 BRT
// Last edited on January 21 of 2020, at 23:16 BRT

#include <chicago/alloc.h>
#include <chicago/debug.h>
#include <chicago/panic.h>
#include <chicago/process.h>
#include <chicago/rand.h>
#include <chicago/shm.h>

List ShmSectionList = { Null, Null, 0, False };

static PShmSection ShmGetSection(UIntPtr key) {
	ListForeach(&ShmSectionList, i) {																			// Let's iterate the section list
		if (((PShmSection)i->data)->key == key) {
			return (PShmSection)i->data;																		// We found it!
		}
	}
	
	return Null;
}

Status ShmCreateSection(UIntPtr size, PUIntPtr key, PUIntPtr ret) {
	if (PsCurrentProcess == Null || size == 0 || key == Null) {													// Sanity checks
		return STATUS_INVALID_ARG;
	}
	
	PShmMappedSection msect = (PShmMappedSection)MemAllocate(sizeof(ShmMappedSection));							// Create the struct that we're going to add to the process shm list
	
	if (msect == Null) {
		return STATUS_OUT_OF_MEMORY;																			// Failed...
	}
	
	msect->shm = (PShmSection)MemAllocate(sizeof(ShmSection));													// Create the shm section struct
	
	if (msect->shm == Null) {
		MemFree((UIntPtr)msect);																				// Failed...
		return STATUS_OUT_OF_MEMORY;
	}
	
	msect->shm->key = RandGenerate();																			// Set the key
	
	while (ShmGetSection(msect->shm->key) != Null) {															// Now, let's make sure that we don't have the same key as some other shm section..
		msect->shm->key = RandGenerate();
	}
	
	msect->shm->refs = 1;																						// For now it have one reference
	msect->shm->pregions.head = Null;																			// Clean the physical regions map
	msect->shm->pregions.tail = Null;
	msect->shm->pregions.length = 0;
	msect->shm->pregions.free = False;
	
	Status status = VirtAllocAddress(0, size, SHM_NEW_FLAGS, &msect->virt);										// Alloc the virtual address
	
	if (status != STATUS_SUCCESS) {
		MemFree((UIntPtr)msect->shm);																			// Failed...
		MemFree((UIntPtr)msect);
		return status;
	}
	
	for (UIntPtr i = msect->virt; i < msect->virt + size; i += MM_PAGE_SIZE) {									// Let's add everything to the physical regions map
		if (!ListAdd(&msect->shm->pregions, (PVoid)MmGetPhys(i))) {
			while (msect->shm->pregions.length != 0) {															// Failed, let's first remove everything from the physcial regions map
				ListRemove(&msect->shm->pregions, 0);
			}
			
			VirtFreeAddress(msect->virt, size);																	// And now free everything
			MemFree((UIntPtr)msect->shm);
			MemFree((UIntPtr)msect);
			
			return STATUS_OUT_OF_MEMORY;
		}
	}
	
	if (!ListAdd(&ShmSectionList, msect->shm)) {																// Add the section to the global section list
		while (msect->shm->pregions.length != 0) {																// Failed, let's first remove everything from the physical regions map
			ListRemove(&msect->shm->pregions, 0);
		}
		
		VirtFreeAddress(msect->virt, size);																		// And now free everything
		MemFree((UIntPtr)msect->shm);
		MemFree((UIntPtr)msect);
		
		return STATUS_OUT_OF_MEMORY;
	} else if (!ListAdd(&PsCurrentProcess->shm_mapped_sections, msect)) {										// Now add it to the process mapped shm sections list
		while (msect->shm->pregions.length != 0) {																// Failed, let's first remove everything from the physical regions map
			ListRemove(&msect->shm->pregions, 0);
		}
		
		ListRemove(&ShmSectionList, ShmSectionList.length - 1);													// Remove it from the global section list
		VirtFreeAddress(msect->virt, size);																		// And now free everything
		MemFree((UIntPtr)msect->shm);
		MemFree((UIntPtr)msect);
		
		return STATUS_OUT_OF_MEMORY;
	}
	
	*key = msect->shm->key;																						// Save the key
	*ret = msect->virt;																							// And the virt address
	
	return STATUS_SUCCESS;
}

static PShmMappedSection ShmGetMappedSection(UIntPtr key) {
	ListForeach(&PsCurrentProcess->shm_mapped_sections, i) {													// Let's iterate the mapped sections list
		if (((PShmMappedSection)i->data)->shm->key == key) {													// Found?
			return (PShmMappedSection)i->data;																	// Yes!
		}
	}
	
	return Null;
}

static UIntPtr ShmGetMappedSectionVirt(PShmSection shm) {
	ListForeach(&PsCurrentProcess->shm_mapped_sections, i) {													// Let's iterate the mapped sections list
		if (((PShmMappedSection)i->data)->shm == shm) {															// Found?
			return ((PShmMappedSection)i->data)->virt;															// Yes, so it is already mapped, return the virtual address
		}
	}
	
	return 0;
}

Status ShmMapSection(UIntPtr key, PUIntPtr ret) {
	if (PsCurrentProcess == Null) {																				// Sanity checks
		return STATUS_SHM_DOESNT_EXISTS;
	}
	
	PShmSection shm = ShmGetSection(key);																		// Try to get the section
	
	if (shm == Null) {
		return STATUS_SHM_DOESNT_EXISTS;																		// Failed...
	}
	
	UIntPtr virt = ShmGetMappedSectionVirt(shm);																// Let's see if this section is already mapped
	
	if (virt != 0) {
		*ret = virt;																							// Yes, it is!
		return STATUS_SUCCESS;
	}
	
	PShmMappedSection msect = (PShmMappedSection)MemAllocate(sizeof(ShmMappedSection));							// Create the struct that we're going to add to the process shm list
	
	if (msect == Null) {
		return STATUS_OUT_OF_MEMORY;
	}
	
	msect->shm = shm;																							// Set the shm
	
	Status status = VirtAllocAddress(0, shm->pregions.length * MM_PAGE_SIZE, SHM_EXIST_FLAGS, &msect->virt);	// Alloc the virtual address
	
	if (status != STATUS_SUCCESS) {
		MemFree((UIntPtr)msect);																				// Failed...
		return status;
	}
	
	for (UIntPtr i = 0; i < shm->pregions.length; i++) {														// Let's map everything!
		status = MmMap(msect->virt + i * MM_PAGE_SIZE, (UIntPtr)ListGet(&shm->pregions, i), SHM_MM_FLAGS);
		if (status != STATUS_SUCCESS) {
			VirtFreeAddress(msect->virt, shm->pregions.length * MM_PAGE_SIZE);									// ...
			MemFree((UIntPtr)msect);
			return status;
		}
	}
	
	if (!ListAdd(&PsCurrentProcess->shm_mapped_sections, msect)) {												// Now add it to the process mapped shm sections list
		VirtFreeAddress(msect->virt, shm->pregions.length * MM_PAGE_SIZE);										// Failed...
		MemFree((UIntPtr)msect);
		
		return STATUS_OUT_OF_MEMORY;
	}
	
	msect->shm->refs++;																							// Increase the reference counter
	*ret = msect->virt;																							// Save the virt address
	
	return STATUS_SUCCESS;
}

static Void ShmRemoveFromList(PList list, PVoid item) {
	UIntPtr i;																									// Just use ListSearch + ListRemove...
	
	if (ListSearch(list, item, &i)) {
		ListRemove(list, i);
	}
}

Status ShmUnmapSection(UIntPtr key) {
	if (PsCurrentProcess == Null) {																				// Sanity checks
		return STATUS_SHM_DOESNT_EXISTS;
	}
	
	PShmMappedSection msect = ShmGetMappedSection(key);															// Try to get the mapped shm
	
	if (msect == Null) {
		return STATUS_SHM_DOESNT_EXISTS;																		// It's not mapped...
	}
	
	ShmRemoveFromList(&PsCurrentProcess->shm_mapped_sections, msect);											// Ok! First, let's remove it from the mapped shm sections list
	VirtFreeAddress(msect->virt, msect->shm->pregions.length * MM_PAGE_SIZE);									// Now, free the virtual memory (and if this is the last reference, also free the physical memory)
	
	msect->shm->refs--;																							// Decrease the reference count
	
	if (msect->shm->refs == 0) {																				// This is the last reference?
		ShmRemoveFromList(&ShmSectionList, msect->shm);															// Yes, first, let's remove it from the section list
		
		while (msect->shm->pregions.length != 0) {																// Now, free the physical regions map
			ListRemove(&msect->shm->pregions, 0);
		}
		
		MemFree((UIntPtr)msect->shm);																			// And free the shm struct
	}
	
	MemFree((UIntPtr)msect);																					// Free the msect strct
	
	return STATUS_SUCCESS;
}
