// File author is Ítalo Lima Marconato Matias
//
// Created on May 31 of 2018, at 18:45 BRT
// Last edited on January 24 of 2020, at 11:31 BRT

#define __CHICAGO_PMM__

#include <chicago/arch/bootmgr.h>
#include <chicago/arch/pmm.h>

#include <chicago/arch.h>
#include <chicago/string.h>

UIntPtr MmBootAllocPointer = 0;
UIntPtr KernelRealEnd = 0;

UIntPtr MmBootAlloc(UIntPtr size, Boolean align) {
	if (MmBootAllocPointer == 0) {																						// Disabled?
		return 0;																										// Yup
	} else if (align && ((MmBootAllocPointer % MM_PAGE_SIZE) != 0)) {													// Align to page size?
		MmBootAllocPointer += MM_PAGE_SIZE - (MmBootAllocPointer % MM_PAGE_SIZE);										// Yes
	}
	
	MmBootAllocPointer += size;																							// Increase the ptr
	StrSetMemory((PUInt8)MmBootAllocPointer - size, 0, size);
	
	return MmBootAllocPointer - size;																					// Return the old one
}

static UIntPtr PMMCountMemory(Void) {
	PBootmgrMemoryMap mmap = BootmgrMemMap;																				// Here we're going to use the memory map for getting the memory size
	UIntPtr mmapi = 0;
	UIntPtr max = 0;
	
	while (mmapi < BootmgrMemMapCount) {
		if (mmap->type > 4) {																							// Valid?
			mmap->type = 2;																								// Nope, so let's set as reserved
		}
		
		if ((mmapi > 0) && (mmap->base == 0)) {																			// End (before expected)?
			break;
		} else {
			if (mmap->base + mmap->length > max) {
				max = mmap->base + mmap->length;
			}
		}
		
		mmap++;
		mmapi++;
	}
	
	return max;
}

static Void PMMFreePage(UIntPtr addr) {
	UIntPtr i = addr >> 22;																								// Calculate all the indexes
	UIntPtr j = (addr - (i << 22)) >> 17;
	UIntPtr k = (addr - (i << 22) - (j << 17)) >> MM_PAGE_SIZE_SHIFT;
	MmPageRegions[i].pages[j] &= ~(UINTPTR_MAX >> ~(k + 1));															// Free the required bits in the bitmap
	MmPageRegions[i].free++;																							// Increase the free counter
	MmUsedBytes -= MM_PAGE_SIZE;																						// And decrease the used bytes counter
}

Void PMMInit(Void) {
	MmBootAllocPointer = KernelSymbolTable + KernelSymbolTableSize;														// Set the start of the temp allocator
	MmMaxBytes = PMMCountMemory();																						// Get memory size based on the memory map entries
	MmUsedBytes = MmMaxBytes;																							// We're going to free the avaliable regions later...
	MmPageRegionCount = MmMaxBytes / MM_PAGE_SIZE / 1024;																// Setup the MmPageRegionCount variable
	MmPageRegions = (PMmPageRegion)MmBootAlloc(MmPageRegionCount * sizeof(MmPageRegion), False);						// Alloc the page frame allocator regions
	MmPageReferences = (PUInt8)MmBootAlloc(MmMaxBytes / MM_PAGE_SIZE * sizeof(UInt8), False);							// Also alloc the page frame reference map
	
	for (UIntPtr i = 0; i < MmPageRegionCount; i++) {																	// Init all the bitmaps on the regions
		StrSetMemory(MmPageRegions[i].pages, 0xFF, sizeof(MmPageRegions[i].pages));
	}
	
	KernelRealEnd = MmBootAllocPointer;																					// Setup the KernelRealEnd variable
	MmBootAllocPointer = 0;																								// Break/disable the MmBootAlloc, now we should use MemAllocate/AAllocate/Reallocate/ZAllocate/Free/AFree
	
	if ((KernelRealEnd % MM_PAGE_SIZE) != 0) {																			// Align the KernelRealEnd variable to page size
		KernelRealEnd += MM_PAGE_SIZE - (KernelRealEnd % MM_PAGE_SIZE);
	}
	
	PBootmgrMemoryMap mmap = BootmgrMemMap;
	UIntPtr mmapi = 0;
#if __INTPTR_WIDTH__ == 64
	UIntPtr kstart = ((UIntPtr)(&KernelStart)) - 0xFFFF800000000000;
	UIntPtr kend = KernelRealEnd - 0xFFFF800000000000;
#else
	UIntPtr kstart = ((UIntPtr)(&KernelStart)) - 0xC0000000;
	UIntPtr kend = KernelRealEnd - 0xC0000000;
#endif
	
	while (mmapi < BootmgrMemMapCount) {
		if (mmap->type > 4) {																							// Valid?
			mmap->type = 2;																								// Nope, so let's set as reserved
		} else if ((mmapi > 0) && (mmap->base == 0)) {																	// End (before expected)?
			break;
		}
		
		if (mmap->type == 1) {																							// Avaliable for use?
			for (UIntPtr i = 0; i < mmap->length; i += MM_PAGE_SIZE) {													// YES!
				UIntPtr addr = mmap->base + i;
				
				if (!(addr == 0 || ((addr >= kstart) && (addr < kend)))) {												// Is it inside of the kernel region?
					PMMFreePage(addr);																					// NOPE!
				}
			}
		}
		
		mmap++;
		mmapi++;
	}
}
