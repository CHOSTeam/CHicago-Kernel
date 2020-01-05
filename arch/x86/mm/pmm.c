// File author is Ítalo Lima Marconato Matias
//
// Created on May 31 of 2018, at 18:45 BRT
// Last edited on December 30 of 2019, at 12:12 BRT

#define __CHICAGO_PMM__

#include <chicago/arch/bootmgr.h>
#include <chicago/arch/pmm.h>

#include <chicago/arch.h>
#include <chicago/mm.h>

UIntPtr MmBootAllocPointer = 0;
UIntPtr KernelRealEnd = 0;

UIntPtr MmBootAlloc(UIntPtr size, Boolean align) {
	if (MmBootAllocPointer == 0) {																						// Disabled?
		return 0;																										// Yup
	} else if (align && ((MmBootAllocPointer % MM_PAGE_SIZE) != 0)) {													// Align to page size?
		MmBootAllocPointer += MM_PAGE_SIZE - (MmBootAllocPointer % MM_PAGE_SIZE);										// Yes
	}
	
	MmBootAllocPointer += size;																							// Increase the ptr
	
	return MmBootAllocPointer - size;																					// Return the old one
}

UIntPtr PMMCountMemory(Void) {
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

Void PMMInit(Void) {
	MmBootAllocPointer = KernelSymbolTable + KernelSymbolTableSize;														// Set the start of the temp allocator
	MmMaxBytes = PMMCountMemory();																						// Get memory size based on the memory map entries
	MmUsedBytes = MmMaxBytes;																							// We're going to free the avaliable pages later
	MmPageStack = (PUIntPtr)MmBootAlloc(MmMaxBytes / MM_PAGE_SIZE * sizeof(UIntPtr), False);							// Alloc the page frame allocator stack using the initial boot allocator
	MmPageReferences = (PUIntPtr)MmBootAlloc(MmMaxBytes / MM_PAGE_SIZE * sizeof(UIntPtr), False);						// Also alloc the page frame reference map
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
				
				if ((addr != 0) && (!((addr >= kstart) && (addr < kend)))) {											// Just check if the addr isn't 0 nor it's the kernel physical address
					MmFreePage(addr);																					// Everything is ok, SO FREE IT!
				}
			}
		}
		
		mmap++;
		mmapi++;
	}
}
