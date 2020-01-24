// File author is Ítalo Lima Marconato Matias
//
// Created on September 15 of 2018, at 12:46 BRT
// Last edited on January 24 of 2020, at 09:15 BRT

#include <chicago/mm.h>
#include <chicago/process.h>
#include <chicago/virt.h>

UInt32 VirtConvertFlags(UInt32 flags) {
	if (flags == 0) {																	// Invalid?
		return MM_MAP_USER;																// Yes, just return MM_MAP_USER...
	}
	
	UInt32 ret = MM_MAP_USER;															// Let's convert the flags!
	
	if ((flags & VIRT_PROT_READ) == VIRT_PROT_READ) {
		ret |= MM_MAP_READ;
	}
	
	if ((flags & VIRT_PROT_WRITE) == VIRT_PROT_WRITE) {
		ret |= MM_MAP_WRITE;
	}
	
	if ((flags & VIRT_PROT_EXEC) == VIRT_PROT_EXEC) {
		ret |= MM_MAP_EXEC;
	}
	
	return ret;
}

UInt32 VirtConvertFlags2(UInt32 flags) {
	if (flags == 0) {																	// Invalid?
		return 0;																		// Yes, return 0
	}
	
	UInt32 ret = 0;																		// Let's convert the flags!
	
	if ((flags & MM_MAP_READ) == MM_MAP_READ) {
		ret |= VIRT_PROT_READ;
	}
	
	if ((flags & MM_MAP_WRITE) == MM_MAP_WRITE) {
		ret |= VIRT_PROT_WRITE;
	}
	
	if ((flags & MM_MAP_EXEC) == MM_MAP_EXEC) {
		ret |= VIRT_PROT_EXEC;
	}
	
	return ret;
}

Status VirtAllocAddress(UIntPtr addr, UIntPtr size, UInt32 flags, PUIntPtr ret) {
	Boolean check = True;
	Boolean aor = (flags & VIRT_FLAGS_ALLOCNOW) != VIRT_FLAGS_ALLOCNOW;
	
	if ((size % MM_PAGE_SIZE) != 0) {													// Page align the size
		size += MM_PAGE_SIZE - (size % MM_PAGE_SIZE);
	}
	
	if (addr == 0) {																	// We need to alloc a new address?
		if ((flags & VIRT_FLAGS_HIGHEST) == VIRT_FLAGS_HIGHEST) {						// Yes, alloc the highest address?
			addr = MmFindHighestFreeVirt(MM_USER_START, MM_USER_END, size);				// Yes, so use the MmFindHighestFreeVirt
		} else {
			addr = MmFindFreeVirt(MM_USER_START, MM_USER_END, size);					// No, so use the MmFindFreeVirt
		}
		
		if (addr == 0) {																// Failed?
			return STATUS_OUT_OF_MEMORY;												// Yes...
		} else {
			check = False;
		}
	} else if (addr >= MM_USER_END) {													// Valid address?
		return STATUS_INVALID_ARG;														// No...
	} else if ((addr % MM_PAGE_SIZE) != 0) {											// Page aligned?
		addr -= addr % MM_PAGE_SIZE;													// No, so let's page align it!
	}
	
	if (check) {
		for (UIntPtr i = addr; i < addr + size; i += MM_PAGE_SIZE) {
			if (MmQuery(i)) {															// In use?
				return STATUS_ALREADY_MAPPED;											// Yes >:(
			}
		}
	}
	
	UIntPtr old = addr;
	UInt32 flgs = VirtConvertFlags(flags);												// Convert the flags
	
	for (UIntPtr i = addr; i < addr + size; i += MM_PAGE_SIZE) {						// And let's do it!
		UIntPtr phys = 0;																// Alloc some phys space (if we need to)
		
		if (!aor && MmReferenceSinglePage(0, &phys) != STATUS_SUCCESS) {				// Failed?
			for (UIntPtr j = old; j < i; j += MM_PAGE_SIZE) {							// Yes, so undo everything :(
				if ((MmQuery(j) & MM_MAP_AOR) != MM_MAP_AOR) {
					MmDereferenceSinglePage(MmGetPhys(j));
				}
				
				MmUnmap(j);
			}
			
			return STATUS_OUT_OF_MEMORY;
		}
		
		Status status = MmMap(i, phys, flgs | (aor ? MM_MAP_AOR : 0));					// Try to map!
		
		if (status != STATUS_SUCCESS) {
			MmDereferenceSinglePage(phys);												// Failed, undo everything...
			
			for (UIntPtr j = old; j < i; j += MM_PAGE_SIZE) {
				if ((MmQuery(j) & MM_MAP_AOR) != MM_MAP_AOR) {
					MmDereferenceSinglePage(MmGetPhys(j));
				}
				
				MmUnmap(j);
			}
			
			return status;
		}
	}
	
	if (PsCurrentThread != Null && PsCurrentProcess != Null) {
		PsCurrentProcess->mem_usage += size;
	}
	
	*ret = addr;																		// :)
	
	return STATUS_SUCCESS;
}

Status VirtFreeAddress(UIntPtr addr, UIntPtr size) {
	if ((addr == 0) || (addr >= MM_USER_END)) {											// Valid address?
		return STATUS_INVALID_ARG;														// Nope
	} else if ((addr % MM_PAGE_SIZE) != 0) {											// Page aligned?
		addr -= addr % MM_PAGE_SIZE;													// No, page align it!
	}
	
	if ((size % MM_PAGE_SIZE) != 0) {													// Page align the size
		size += MM_PAGE_SIZE - (size % MM_PAGE_SIZE);
	}
	
	for (UIntPtr i = addr; i < addr + size; i += MM_PAGE_SIZE) {						// Let's try to do it!
		if (!MmQuery(i)) {																// Mapped?
			return STATUS_NOT_MAPPED;													// No...
		}
		
		if ((MmQuery(i) & MM_MAP_AOR) != MM_MAP_AOR) {									// Free/dereference the physical page
			MmDereferenceSinglePage(MmGetPhys(i));
		}
		
		MmUnmap(i);																		// And unmap
	}
	
	if (PsCurrentThread != Null && PsCurrentProcess != Null) {
		PsCurrentProcess->mem_usage -= size;
	}
	
	return STATUS_SUCCESS;
}

UInt32 VirtQueryProtection(UIntPtr addr) {
	if ((addr == 0) || (addr >= MM_USER_END)) {											// Valid address?
		return 0;																		// Nope
	} else {
		return VirtConvertFlags2(MmQuery(addr));										// Just try to use the VirtConvertFlags2 (MM_MAP to VIRT_PROT)
	}
}

Status VirtChangeProtection(UIntPtr addr, UIntPtr size, UInt32 flags) {
	if ((addr == 0) || (addr >= MM_USER_END)) {											// Valid address?
		return STATUS_INVALID_ARG;														// Nope...
	} else if ((addr % MM_PAGE_SIZE) != 0) {											// Page aligned?
		addr -= addr % MM_PAGE_SIZE;													// Nope, let's page align this addr!
	}
	
	if ((size % MM_PAGE_SIZE) != 0) {													// Page align the size
		size += MM_PAGE_SIZE - (size % MM_PAGE_SIZE);
	}
	
	UInt32 flgs = VirtConvertFlags(flags);												// Convert our flags
	
	for (UIntPtr i = addr; i < addr + size; i += MM_PAGE_SIZE) {						// Let's try to do it!
		if (!MmQuery(i)) {																// Mapped?
			return STATUS_NOT_MAPPED;													// No...
		}
		
		UInt32 extra = ((MmQuery(i) & MM_MAP_AOR) == MM_MAP_AOR) ? MM_MAP_AOR : 0;		// Get if we need to set the AOR flag
		Status status = MmMap(i, MmGetPhys(i), flgs | extra);							// Try to remap with the new protection
		
		if (status != STATUS_SUCCESS) {
			return status;																// Failed :(
		}
	}
	
	return STATUS_SUCCESS;
}

UIntPtr VirtGetUsage(Void) {
	if (PsCurrentThread != Null && PsCurrentProcess != Null) {
		return PsCurrentProcess->mem_usage;
	} else {
		return 0;
	}
}
