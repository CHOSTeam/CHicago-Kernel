// File author is Ítalo Lima Marconato Matias
//
// Created on January 25 of 2020, at 13:36 BRT
// Last edited on February 02 of 2020, at 10:43 BRT

#include <chicago/debug.h>
#include <chicago/mm.h>
#include <chicago/panic.h>
#include <chicago/string.h>

static PChar MmPageFaultStrings[] = {
	"read/write to non-present area",
	"read/write to kernel-only area",
	"write to read-only area"
};

static UInt32 MmPageFaultPanicCodes[] = {
	PANIC_MM_READWRITE_TO_NONPRESENT_AREA,
	PANIC_MM_READWRITE_TO_KERNEL_AREA,
	PANIC_MM_WRITE_TO_READONLY_AREA
};

Status MmPageFaultDoAOR(UIntPtr addr, UInt32 prot) {
	if (addr == 0 || (addr & ~MM_PAGE_MASK) != 0) {																													// Sanity check
		return STATUS_INVALID_ARG;
	} else if (MmGetPhys(addr) != 0) {																																// Make sure that we need to AOR
		if (MmQuery(addr) == 0) {																																	// But we need to reset the present bit?
			Status status = MmMap(addr, MmGetPhys(addr), prot);																										// Yeah...
			
			if (status != STATUS_SUCCESS) {
				return status;
			}
		}
		
		return STATUS_SUCCESS;
	}
	
	UIntPtr phys;																																					// Yes, Alloc the physical address..
	Status status = MmReferenceSinglePage(0, &phys);
	
	if (status != STATUS_SUCCESS) {
		return status;																																				// Oh, it failed...
	} else if ((status = MmMap(addr, phys, prot)) != STATUS_SUCCESS) {																								// Try to map it!
		MmDereferenceSinglePage(phys);																																// Failed...
		return status;
	}
	
	return STATUS_SUCCESS;
}

Status MmPageFaultDoCOW(UIntPtr addr, UInt32 prot) {
	if (addr == 0 || (addr & ~MM_PAGE_MASK) != 0) {																													// Sanity check
		return STATUS_INVALID_ARG;
	}
	
	UIntPtr oldp = MmGetPhys(addr);																																	// Get the old physical address
	
	if (MmGetReferences(oldp) == 1) {																																// Is this the last reference?
		return MmMap(addr, oldp, prot);																																// Yeah, just remap it without the COW flag...
	}
	
	UIntPtr newp;																																					// Oh well, we need to alloc more one page, and copy the contents of the old page into it...
	Status status = MmReferenceSinglePage(0, &newp);
	
	if (status != STATUS_SUCCESS) {
		return status;																																				// Failed...
	}
	
	PUInt8 temp = (PUInt8)MmMapTemp(oldp, MM_MAP_KDEF);																												// Temp map the old physical address
	
	if (temp == Null) {
		MmDereferenceSinglePage(oldp);
		return STATUS_OUT_OF_MEMORY;
	} else if ((status = MmMap(addr & MM_PAGE_MASK, newp, prot)) != STATUS_SUCCESS) {																				// Remap the virt address with the new physical address
		MmUnmap((UIntPtr)temp);
		MmDereferenceSinglePage(newp);
		return status;
	}
	
	StrCopyMemory((PUInt8)(addr & MM_PAGE_MASK), temp, MM_PAGE_SIZE);																								// Copy the contents of the old page into the new one
	MmUnmap((UIntPtr)temp);																																			// Unmap and dereference the old page
	MmDereferenceSinglePage(oldp);
	
	return STATUS_SUCCESS;
}

Status MmPageFaultDoMapFile(PFsNode file, UIntPtr addr, UIntPtr off, UInt32 prot) {
	if (file == Null || addr == 0 || (addr & ~MM_PAGE_MASK) != 0) {																									// Sanity check
		return STATUS_INVALID_ARG;
	}
	
	MmPrepareUnmapFile(addr, addr + MM_PAGE_SIZE);																													// Yeah, we need to call prepare unmap file here...
	
	if (file->map != Null) {																																		// Should we even do anything here, or this file have a specific map function?
		return file->map(file, addr, off, prot);																													// Specific map function...
	}
	
	Boolean free = False;
	
	if (MmGetPhys(addr) == 0) {																																		// Should we alloc first?
		Status status = MmPageFaultDoAOR(addr, prot);																												// Yeah, do it
		
		if (status != STATUS_SUCCESS) {
			return status;
		}
		
		free = True;
	}
	
	if (off >= file->length) {																																		// Ok, do we need to read the file?
		return STATUS_SUCCESS;																																		// Nope, just return
	}
	
	UIntPtr read;
	Status status = FsReadFile(file, off, MM_PAGE_SIZE, (PUInt8)addr, &read);																						// Now, read the file!
	
	if (status != STATUS_SUCCESS) {
		if (free) {																																					// And we failed...
			MmDereferenceSinglePage(MmGetPhys(addr));
			MmUnmap(addr);
		}
		
		return status;
	}
	
	return STATUS_SUCCESS;
}

static Status MmPageFaultCrash(PVoid arch, UIntPtr addr, UInt8 reason) {
	DbgWriteFormated("PANIC! Page fault (%s) at 0x%x\r\n", MmPageFaultStrings[reason], addr);																		// Print the error in the debug port
	ArchPanic(MmPageFaultPanicCodes[reason], arch);																													// And call the arch panic function...
	return STATUS_MAP_ERROR;																																		// So, uh, dummy error code?
}

Status MmPageFaultHandler(PMmRegion region, PVoid arch, UIntPtr addr, UInt32 flags, UInt8 reason) {
	if (reason == MM_PF_READWRITE_ON_KERNEL_ONLY) {																													// Are we here because the user tried to write to a kernel only page?
		return MmPageFaultCrash(arch, addr, reason);																												// ... Panic
	} else if ((flags & MM_MAP_AOR) != MM_MAP_AOR && (region == Null || region->file == Null) && reason == MM_PF_READWRITE_ON_NON_PRESENT) {						// Oh, wait, is this a page fault from non-present read/write, and the page isn't AOR nor is file mapped?
		return MmPageFaultCrash(arch, addr, reason);																												// ...
	} else if ((flags & MM_MAP_COW) != MM_MAP_COW && reason == MM_PF_WRITE_ON_READONLY) {																			// Write on read-only, and the page isn't set to COW?
		return MmPageFaultCrash(arch, addr, reason);
	}
	
	Status status;
	
	if (region != Null && region->file != Null && reason == MM_PF_READWRITE_ON_NON_PRESENT) {																		// Should we map (part of) a file into this page?
		status = MmPageFaultDoMapFile(region->file, addr, (addr & MM_PAGE_MASK) - region->key.start + region->off, flags & MM_MAP_PROT);							// Yeah, handle it in another function...
		
		if (status != STATUS_SUCCESS) {
			return MmPageFaultCrash(arch, addr, reason);																											// Failed...
		}
		
		return STATUS_SUCCESS;
	}
	
	switch (reason) {																																				// Finally, let's go!
	case MM_PF_READWRITE_ON_NON_PRESENT: {																															// Alloc on Runtime (AOR)
		status = MmPageFaultDoAOR(addr & MM_PAGE_MASK, flags & MM_MAP_PROT);
		break;
	}
	case MM_PF_WRITE_ON_READONLY: {																																	// Copy on Write (COW)
		status = MmPageFaultDoCOW(addr & MM_PAGE_MASK, (flags & MM_MAP_PROT) | MM_MAP_WRITE);
		break;
	}
	default: {
		return MmPageFaultCrash(arch, addr, MM_PF_READWRITE_ON_NON_PRESENT);																						// Wtf?	
	}
	}
	
	if (status != STATUS_SUCCESS) {
		return MmPageFaultCrash(arch, addr, reason);																												// Failed...
	}
	
	return STATUS_SUCCESS;
}
