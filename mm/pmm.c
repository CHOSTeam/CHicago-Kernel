// File author is Ítalo Lima Marconato Matias
//
// Created on June 28 of 2018, at 18:42 BRT
// Last edited on January 23 of 2020, at 22:02 BRT

#include <chicago/mm.h>

PMmPageRegion MmPageRegions = Null;
PUIntPtr MmPageReferences = Null;
UIntPtr MmPageRegionCount = 0;
UIntPtr MmUsedBytes = 0;
UIntPtr MmMaxBytes = 0;

static UIntPtr MmCountFreePages(UIntPtr bm, UIntPtr start, UIntPtr max) {
	UIntPtr ret = 0;																																	// Just count how many consecutive bits are unset
	for (; start + ret <= max && !(bm & (1 << (start + ret))); ret++) ;
	return ret;
}

static IntPtr MmFindFreePages(UIntPtr bm, UIntPtr count) {
	for (UIntPtr i = 0; i < sizeof(UIntPtr) * 8; ) {
		if (bm == UINTPTR_MAX) {																														// All the bits are set?
			return -1;																																	// Yeah...
		} else if (32 - i < count) {																													// If all the other bytes were free, would we be able to alloc the specified count?
			return -1;																																	// Nope...
		} else if (bm == 0) {																															// No bits are set?
			if (sizeof(UIntPtr) * 8 < count) {																											// Nope! But do we have enough bits/pages?
				return -1;																																// No, we don't...
			}
			
			return i;																																	// Yes, we do!
		}
		
#if __INTPTR_WIDTH__ == 64																																// Get the index of the first zero bit in the bitmap
		UIntPtr bit = __builtin_ctzll(~bm);
#else
		UIntPtr bit = __builtin_ctz(~bm);
#endif
		UIntPtr aval = MmCountFreePages(bm, bit, 32 - i);																								// Count how many consecutive free pages we have?
		
		if (aval >= count) {																															// We have enough pages?
			return i + bit;																																// Yes, return!
		}
		
		bm >>= bit + aval;																																// Advance the bitmap forward
		i += bit + aval;																																// And the index as well
	}
	
	return -1;																																			// I don't think that we are supposed to reach here...
}

static Status MmAllocPage(UIntPtr count, PUIntPtr ret) {
	if (count == 0 || ret == Null) {																													// Check if the args are valid
		return STATUS_INVALID_ARG;
	} else if (MmPageRegions == Null || MmUsedBytes + count * MM_PAGE_SIZE >= MmMaxBytes) {																// Check if the physical region allocator is already initialized, after that, check if we have enough free bytes so satisfy the request
		return STATUS_OUT_OF_MEMORY;
	}
	
	for (UIntPtr i = 0; i < MmPageRegionCount; i++) {																									// Let's iterate through the regions
		if (MmPageRegions[i].free < count) {																											// Do we have enough free pages here?
			continue;																																	// Nope... continue searching in the next region
		}
		
		for (UIntPtr j = 0; j < (1024 * MM_PAGE_SIZE) / (8 * MM_PAGE_SIZE * sizeof(UIntPtr)); j++) {													// Now, let's search on each subregion...
			IntPtr bit = MmFindFreePages(MmPageRegions[i].pages[j], count);
			
			if (bit != -1) {																															// Did we find enough bytes in this subregion?
				MmPageRegions[i].pages[j] |= UINTPTR_MAX >> -(bit + count);																				// YEAH! Set all the required bits in the subregion
				MmPageRegions[i].free -= count;
				MmUsedBytes += count * MM_PAGE_SIZE;
				*ret = (i * MM_PAGE_SIZE * 1024) + (j * MM_PAGE_SIZE * sizeof(UIntPtr) * 8) + (bit * MM_PAGE_SIZE);
				return STATUS_SUCCESS;
			}
		}
	}
	
	return STATUS_OUT_OF_MEMORY;																														// We couldn't a region with enough pages...
}

static Status MmFreePage(UIntPtr addr, UIntPtr count) {
	if ((addr % MM_PAGE_SIZE) != 0) {																													// Page align the address
		addr -= addr % MM_PAGE_SIZE;
	}
	
	if (addr == 0 || count == 0 || MmPageRegions == Null || addr + count * MM_PAGE_SIZE >= MmMaxBytes || MmUsedBytes < count) {							// Sanity check
		return STATUS_INVALID_ARG;
	}
	
	UIntPtr i = addr >> 22;																																// Get the region index
	
	if (MmPageRegions[i].free < count) {																												// More one sanity check
		return STATUS_INVALID_ARG;
	}
	
	UIntPtr j = (addr - (i << 22)) >> 17;																												// Get the subregion index
	UIntPtr k = (addr - (i << 22) - (j << 17)) >> MM_PAGE_SIZE_SHIFT;																					// And get the bit (index) inside of the subregion
	
	MmPageRegions[i].pages[j] &= ~(UINTPTR_MAX >> ~(k + count));																						// Unset all the bits that we need to
	MmPageRegions[i].free -= count;																														// Update the free counter on the region
	MmUsedBytes -= count * MM_PAGE_SIZE;																												// Finally, update the used bytes counter
	
	return STATUS_SUCCESS;
}

Status MmAllocSinglePage(PUIntPtr ret) {
	return MmAllocPage(1, ret);																															// Just a single page...
}

Status MmAllocContigPages(UIntPtr count, PUIntPtr ret) {
	return MmAllocPage(count, ret);																														// Just redirect to the alloc page function, it try to alloc contig pages
}

Status MmAllocNonContigPage(UIntPtr count, PUIntPtr ret) {
	if (count == 0 || ret == Null) {																													// First, sanity check
		return STATUS_INVALID_ARG;
	}
	
	for (UIntPtr i = 0; i < count; i++) {																												// Well, let's alloc page by page, as they don't need to be contig... this is probably a bit faster than the contig alloc... or a lot...
		UIntPtr addr;
		Status status = MmAllocSinglePage(&addr);
		
		if (status != STATUS_SUCCESS) {
			MmFreeNonContigPages(ret, i);																												// Failed, free all the allocated pages
			return status;																																// And return the status code
		}
		
		ret[i] = addr;																																	// And save the allocated page
	}
	
	return STATUS_SUCCESS;
}

Status MmFreeSinglePage(UIntPtr addr) {
	return MmFreePage(addr, 1);																															// Just a single page...
}

Status MmFreeContigPages(UIntPtr addr, UIntPtr count) {
	return MmFreePage(addr, count);																														// Just redirect to the free page function, it will free contig pages
}

Status MmFreeNonContigPages(PUIntPtr pages, UIntPtr count) {
	if (pages == Null || count == 0) {																													// Sanity checks...
		return STATUS_INVALID_ARG;
	}
	
	for (UIntPtr i = 0; i < count; i++) {																												// And let's free page per page...
		Status status = MmFreeSinglePage(pages[i]);
		
		if (status != STATUS_SUCCESS) {
			return status;																																// Failed, return the status code...
		}
	}
	
	return STATUS_SUCCESS;
}

Status MmReferenceSinglePage(UIntPtr addr, PUIntPtr ret) {
	if ((addr % MM_PAGE_SIZE) != 0) {																													// Page aligned?
		addr -= addr % MM_PAGE_SIZE;																													// No, so let's align it!
	}
	
	if (addr == 0) {																																	// Should we alloc the page?
		Status status = MmAllocSinglePage(&addr);																										// Yeah, try to do it...
		
		if (status != STATUS_SUCCESS) {
			return status;																																// Failed, return the status code...
		}
		
		return MmReferenceSinglePage(addr, ret);																										// And return with a recursive call!
	} else if (MmPageReferences == Null || MmUsedBytes == 0 || ret == Null || addr >= MmMaxBytes) {														// Some sanity checks
		return STATUS_INVALID_ARG;
	}
	
	MmPageReferences[addr >> MM_PAGE_SIZE_SHIFT]++;
	*ret = addr;
	
	return STATUS_SUCCESS;
}

Status MmReferenceContigPages(UIntPtr addr, UIntPtr count, PUIntPtr ret) {
	if ((addr % MM_PAGE_SIZE) != 0) {																													// Page aligned?
		addr -= addr % MM_PAGE_SIZE;																													// No, so let's align it!
	}
	
	if (addr == 0) {																																	// Should we alloc the page?
		Status status = MmAllocContigPages(count, &addr);																								// Yeah, try to do it...
		
		if (status != STATUS_SUCCESS) {
			return status;																																// Failed, return the status code...
		}
		
		return MmReferenceContigPages(addr, count, ret);																								// And return with a recursive call!
	} else if (MmPageReferences == Null || MmUsedBytes < count * MM_PAGE_SIZE || ret == Null || (addr + count * MM_PAGE_SIZE) >= MmMaxBytes) {			// Some sanity checks
		return STATUS_INVALID_ARG;
	} else if (count == 0) {
		return STATUS_INVALID_ARG;
	}
	
	for (UIntPtr i = 0; i < count; i++) {																												// Now, reference all the pages, one by one
		UIntPtr discard;
		Status status = MmReferenceSinglePage(addr + i * MM_PAGE_SIZE, &discard);
		
		if (status != STATUS_SUCCESS) {
			MmDereferenceContigPages(addr, i);																											// Failed, dereference all the pages that the operation was completed successfully
			return status;
		}
	}
	
	return STATUS_SUCCESS;
}

Status MmReferenceNonContigPages(PUIntPtr pages, UIntPtr count, PUIntPtr ret) {
	if (MmPageReferences == Null || MmUsedBytes < count * MM_PAGE_SIZE || count == 0 || pages == Null || ret == Null) {									// Some sanity checks
		return STATUS_INVALID_ARG;
	}
	
	for (UIntPtr i = 0; i < count; i++) {																												// Now, reference all the pages, one by one
		Status status = MmReferenceSinglePage(pages[i], &ret[i]);
		
		if (status != STATUS_SUCCESS) {
			MmDereferenceNonContigPages(pages, i);																										// Failed, dereference all the pages that the operation was completed successfully
			return status;
		}
	}
	
	return STATUS_SUCCESS;
}

Status MmDereferenceSinglePage(UIntPtr addr) {
	if ((addr % MM_PAGE_SIZE) != 0) {																													// Page aligned?
		addr -= addr % MM_PAGE_SIZE;																													// No, so let's align it!
	}
	
	if (MmPageReferences == Null || MmUsedBytes == 0 || addr == 0 || addr >= MmMaxBytes) {																// Some sanity checks
		return STATUS_INVALID_ARG;
	}
	
	MmPageReferences[addr >> MM_PAGE_SIZE_SHIFT]--;																										// Decrease the reference counter
	
	if (MmPageReferences[addr >> MM_PAGE_SIZE_SHIFT] == 0) {																							// Was this the last reference?
		MmFreeSinglePage(addr);																															// Yeah, so we can free it
	}
	
	return STATUS_SUCCESS;
}

Status MmDereferenceContigPages(UIntPtr addr, UIntPtr count) {
	if ((addr % MM_PAGE_SIZE) != 0) {																													// Page aligned?
		addr -= addr % MM_PAGE_SIZE;																													// No, so let's align it!
	}
	
	if (MmPageReferences == Null || MmUsedBytes < count * MM_PAGE_SIZE || addr == 0 || (addr + count * MM_PAGE_SIZE) >= MmMaxBytes) {					// Some sanity checks
		return STATUS_INVALID_ARG;
	}
	
	for (UIntPtr i = 0; i < count; i++) {																												// Dereference page per page
		Status status = MmDereferenceSinglePage(addr + i * MM_PAGE_SIZE);
		
		if (status != STATUS_SUCCESS) {
			return status;																																// Failed, return the status code
		}
	}
	
	return STATUS_SUCCESS;
}

Status MmDereferenceNonContigPages(PUIntPtr pages, UIntPtr count) {
	if (MmPageReferences == Null || MmUsedBytes < count * MM_PAGE_SIZE || count == 0 || pages == Null) {												// Some sanity checks
		return STATUS_INVALID_ARG;
	}
	
	for (UIntPtr i = 0; i < count; i++) {																												// Dereference page per page
		Status status = MmDereferenceSinglePage(pages[i]);
		
		if (status != STATUS_SUCCESS) {
			return status;																																// Failed, return the status code
		}
	}
	
	return STATUS_SUCCESS;
}

UIntPtr MmGetReferences(UIntPtr addr) {
	if ((addr == 0) || (addr >= MmMaxBytes)) {																											// Sanity check...
		return 0;
	} else if ((addr % MM_PAGE_SIZE) != 0) {																											// Page aligned?
		addr -= addr % MM_PAGE_SIZE;																													// No, so let's align it!
	}
	
	return MmPageReferences[addr >> MM_PAGE_SIZE_SHIFT];
}

UIntPtr MmGetSize(Void) {
	return MmMaxBytes;
}

UIntPtr MmGetUsage(Void) {
	return MmUsedBytes;
}

UIntPtr MmGetFree(Void) {
	return MmMaxBytes - MmUsedBytes;
}
