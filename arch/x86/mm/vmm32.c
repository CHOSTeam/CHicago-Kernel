// File author is Ítalo Lima Marconato Matias
//
// Created on June 28 of 2018, at 19:19 BRT
// Last edited on January 25 of 2020, at 12:08 BRT

#include <chicago/arch/vmm.h>

#include <chicago/mm.h>
#include <chicago/string.h>

UIntPtr MmGetPhys(UIntPtr virt) {
	if ((MmGetPDE(virt) & PAGE_PRESENT) != PAGE_PRESENT) {																			// The page table exists?
		return 0;																													// No, so just return 0
	} else if ((MmGetPDE(virt) & PAGE_HUGE) == PAGE_HUGE) {																			// 4MiB page?
		return (MmGetPDE(virt) & MM_PAGE_MASK) + (virt & 0x3FFFFF);																	// Yes, return!
	} else {
		return (MmGetPTE(virt) & MM_PAGE_MASK) + (virt & 0xFFF);																	// Return!
	}
}

UInt32 MmQuery(UIntPtr virt) {
	UIntPtr page = 0;
	
	if ((MmGetPDE(virt) & PAGE_PRESENT) != PAGE_PRESENT) {																			// The page table exists?
		return 0;																													// No, so just return 0
	} else if ((MmGetPDE(virt) & PAGE_HUGE) == PAGE_HUGE) {																			// 4MiB page?
		page = MmGetPDE(virt);																										// Yes...
	} else if ((MmGetPTE(virt) & PAGE_PRESENT) != PAGE_PRESENT) {																	// Same as above
		return 0;
	} else {
		page = MmGetPTE(virt);																										// Normal PTE!
	}
	
	UInt32 ret = MM_MAP_READ | MM_MAP_EXEC;																							// And convert the page flags to MmMap flags
	
	if ((page & PAGE_WRITE) == PAGE_WRITE) {
		ret |= MM_MAP_WRITE;
	}
	
	if ((page & PAGE_USER) == PAGE_USER) {
		ret |= (MM_MAP_USER | MM_MAP_KERNEL);
	} else {
		ret |= MM_MAP_KERNEL;
	}
	
	if ((page & PAGE_COW) == PAGE_COW) {
		ret |= MM_MAP_COW;
	}
	
	return ret;
}

static Boolean MmIsAllocated(UIntPtr page) {
	return (page & PAGE_PRESENT) == PAGE_PRESENT || (page & PAGE_AOR) == PAGE_AOR;
}

UIntPtr MmFindFreeVirt(UIntPtr start, UIntPtr end, UIntPtr count) {
	if (start % MM_PAGE_SIZE != 0) {																								// Page align the start
		start -= count % MM_PAGE_SIZE;
	}
	
	if (end % MM_PAGE_SIZE != 0) {																									// Page align the end
		end += MM_PAGE_SIZE - (end % MM_PAGE_SIZE);
	}
	
	if (count % MM_PAGE_SIZE != 0) {																								// Page align the count
		count += MM_PAGE_SIZE - (count % MM_PAGE_SIZE);
	}
	
	UIntPtr c = 0;
	UIntPtr p = start;
	
	for (UIntPtr i = start; i < end; i += MM_PAGE_SIZE * 1024) {																	// Let's try to find the first free virtual address!
		if (MmIsAllocated(MmGetPDE(i))) {																							// This PDE is allocated?
			if ((MmGetPDE(i) & PAGE_HUGE) == PAGE_HUGE) {																			// Yes! 4MiB page?
				c = 0;																												// Yes :(
				p = i + 0x400000;
			} else {
				for (UIntPtr j = 0; j < MM_PAGE_SIZE * 1024; j += MM_PAGE_SIZE) {													// Nope! Let's check the PTEs
					if (((i == 0) && (j == 0)) || MmIsAllocated(MmGetPTE(i + j))) {													// Allocated?
						c = 0;																										// Yes :(
						p = i + j + MM_PAGE_SIZE;
					} else {
						c += MM_PAGE_SIZE;																							// No! (+4KB)

						if (c >= count) {																							// We need more memory?
							return p;																								// No, so return!
						}
					}
				}
			}
		} else {
			c += MM_PAGE_SIZE * 1024;																								// No! (+4MB)
			
			if (i < MM_PAGE_SIZE * 1024) {																							// 0x00000000?
				c -= MM_PAGE_SIZE;																									// Yes, but it's reserved...
				p += MM_PAGE_SIZE;
			}
			
			if (c >= count) {																										// We need more memory?
				return p;																											// No, so return!
			}
		}
	}
	
	return 0;																														// We failed :(
}

UIntPtr MmFindHighestFreeVirt(UIntPtr start, UIntPtr end, UIntPtr count) {
	if (start % MM_PAGE_SIZE != 0) {																								// Page align the start
		start -= count % MM_PAGE_SIZE;
	}
	
	if (end % MM_PAGE_SIZE != 0) {																									// Page align the end
		end += MM_PAGE_SIZE - (end % MM_PAGE_SIZE);
	}
	
	if (count % MM_PAGE_SIZE != 0) {																								// Page align the count
		count += MM_PAGE_SIZE - (count % MM_PAGE_SIZE);
	}
	
	UIntPtr c = 0;
	UIntPtr p = end;
	
	for (UIntPtr i = end - (MM_PAGE_SIZE * 1024); i > start; i -= MM_PAGE_SIZE * 1024) {											// Let's try to find the highest free virtual address!
		if (MmIsAllocated(MmGetPDE(i))) {																							// This PDE is allocated?
			if ((MmGetPDE(i) & PAGE_HUGE) == PAGE_HUGE) {																			// Yes! 4MiB page?
				c = 0;																												// Yes :(
				p = i - 0x400000;
			} else {
				for (UIntPtr j = MM_PAGE_SIZE * 1024; j > 0; j -= MM_PAGE_SIZE) {													// Nope! Let's check the PTEs
					UIntPtr rj = j - 1;

					if ((i == start) && (j == 0)) {																					// curr == start?
						return 0;																									// Yes, we failed :(
					} else if (MmIsAllocated(MmGetPTE(i + rj))) {																	// Allocated?
						c = 0;																										// Yes :(
						p = i + j - MM_PAGE_SIZE;
					} else {
						c += MM_PAGE_SIZE;																							// No! (+4KB)

						if (c >= count) {																							// We need more memory?
							return p - count;																						// No, so return!
						}
					}
				}
			}
		} else {
			c += MM_PAGE_SIZE * 1024;																								// No! (+4MB)
			
			if (i == 0) {																											// 0x00000000?
				c -= MM_PAGE_SIZE;																									// Yes...
			}
			
			if (c >= count) {																										// We need more memory?
				return p - count;																									// No, so return!
			}
		}
	}
	
	return 0;																														// We failed :(
}

UIntPtr MmMapTemp(UIntPtr phys, UInt32 flags) {
	for (UIntPtr i = 0xFF800000; i < 0xFFC00000; i += MM_PAGE_SIZE) {																// Let's try to find an free temp address
		if (MmQuery(i) == 0) {																										// Free?
			if (MmMap(i, phys, flags) == STATUS_SUCCESS) {																			// Yes, so try to map it
				return i;																											// Mapped! Now we only need to return it
			}
		}
	}
	
	return 0;																														// Failed...
}

Status MmMap(UIntPtr virt, UIntPtr phys, UInt32 flags) {
	if ((virt % MM_PAGE_SIZE) != 0) {																								// Align to page size
		virt -= virt % MM_PAGE_SIZE;
	}
	
	if ((phys % MM_PAGE_SIZE) != 0) {																								// Align to page size
		phys -= phys % MM_PAGE_SIZE;
	}
	
	UInt32 flags2 = ((flags & MM_MAP_AOR) == MM_MAP_AOR) ? PAGE_AOR : PAGE_PRESENT;													// Convert the MmMap flags to page flags
	
	if ((flags & MM_MAP_USER) == MM_MAP_USER) {
		flags2 |= PAGE_USER;
	}
	
	if ((flags & MM_MAP_WRITE) == MM_MAP_WRITE) {
		flags2 |= PAGE_WRITE;
	}
	
	if ((MmGetPDE(virt) & PAGE_PRESENT) != PAGE_PRESENT) {																			// This page table exists?
		UIntPtr block;																												// No, so let's alloc it
		
		if (MmReferenceSinglePage(0, &block) != STATUS_SUCCESS) {																	// Failed?
			return STATUS_OUT_OF_MEMORY;																							// Then just return
		}
		
		if (virt >= 0xC0000000) {																									// Kernel-only page directory?
			MmSetPDE(virt, block, 0x03);																							// Yes, so put the pde as present, writeable
		} else {
			MmSetPDE(virt, block, 0x07);																							// No, so put the pde as present, writeable and set the user bit
		}
		
		MmInvlpg(MmGetPTELoc(virt));																								// Refresh the TLB
		StrSetMemory((PUInt8)(MmGetPTELoc(virt)), 0, 0x1000);																		// And clean the PTE entries
	} else if ((MmGetPDE(virt) & PAGE_HUGE) == PAGE_HUGE) {																			// 4MiB page?
		return STATUS_MAP_ERROR;																									// Yes, but sorry, we don't support mapping it YET
	}
	
	MmSetPTE(virt, phys, flags2);																									// Map the phys addr to the virt addr
	MmInvlpg(virt);																													// Refresh the TLB
	
	return STATUS_SUCCESS;
}

Status MmUnmap(UIntPtr virt) {
	if ((MmGetPDE(virt) & PAGE_PRESENT) != PAGE_PRESENT) {																			// This page table exists?
		return STATUS_NOT_MAPPED;																									// No...
	} else if ((MmGetPDE(virt) & PAGE_HUGE) == PAGE_HUGE) {																			// 4MiB page?
		return STATUS_UNMAP_ERROR;																									// Yes, but sorry, we don't support mapping it YET
	} else if ((MmGetPTE(virt) & PAGE_PRESENT) != PAGE_PRESENT) {																	// Same as above
		return STATUS_NOT_MAPPED;																									// No...
	} else {
		MmSetPTE(virt, 0, 0);																										// Yes, so unmap the virt addr
		MmInvlpg(virt);																												// Refresh the TLB
		return STATUS_SUCCESS;
	}
}

UIntPtr MmCreateDirectory(Void) {
	UIntPtr ret;																													// Allocate one physical page
	PUInt32 dir = Null;
	
	if (MmReferenceSinglePage(0, &ret) != STATUS_SUCCESS) {																			// Failed?
		return 0;																													// Yes...
	}
	
	if ((dir = (PUInt32)MmMapTemp(ret, MM_MAP_KDEF)) == 0) {																		// Try to map it to an temp addr
		MmDereferenceSinglePage(ret);																								// Failed...
		return 0;
	}
	
	for (UInt32 i = 0; i < 1024; i++) {																								// Let's fill the table!
		if (((MmGetPDE(i << 22) & PAGE_PRESENT) != PAGE_PRESENT) || (i < 768) || (i == 1022)) {										// Non-present, user or temp?
			dir[i] = 0;																												// Yes, so just zero it
		} else if (i == 1023) {																										// Recursive mapping entry?
			dir[i] = (ret & MM_PAGE_MASK) | 3;																						// Yes
		} else {																													// Normal kernel entry?
			dir[i] = MmGetPDE(i << 22);																								// Yes
		}
	}
	
	MmUnmap((UIntPtr)dir);																											// Unmap the temp addr
	
	return ret;
}

Void MmFreeDirectory(UIntPtr dir) {
	if ((dir == 0) || (dir == MmGetCurrentDirectory())) {																			// Sanity checks
		return;
	}
	
	PUInt32 tmp = Null;
	
	if ((tmp = (PUInt32)MmMapTemp(dir, MM_MAP_KDEF)) == 0) {																		// Let's try to map us to an temp addr
		MmDereferenceSinglePage(dir);																								// Failed, so just free the dir physical address
		return;
	}
	
	for (UInt32 i = 0; i < 768; i++) {																								// Let's free the user tables
		if ((tmp[i] & PAGE_PRESENT) == PAGE_PRESENT) {																				// Present?
			if ((tmp[i] & PAGE_HUGE) == PAGE_HUGE) {																				// Yes, 4MiB page?
				MmDereferenceSinglePage(tmp[i] & MM_PAGE_MASK);																		// Yes, free it!
				continue;
			}
			
			UIntPtr tabpa = tmp[i] & MM_PAGE_MASK;																					// Yes
			PUInt32 tabta = Null;
			
			if ((tabta = (PUInt32)MmMapTemp(tabpa, MM_MAP_KDEF)) == 0) {
				MmDereferenceSinglePage(tabpa);
				continue;
			}
			
			for (UInt32 j = 0; j < 1024; j++) {
				if ((tabta[j] & PAGE_PRESENT) == PAGE_PRESENT) {																	// Present?
					MmDereferenceSinglePage(tabta[j] & MM_PAGE_MASK);																// Yes, just use the dereference function
				}				
			}
			
			MmUnmap((UIntPtr)tabta);																								// Unmap the temp addr
			MmDereferenceSinglePage(tabpa);																							// And use the dereference function
		}
	}
}

UIntPtr MmGetCurrentDirectory(Void) {
	UIntPtr ret;
	Asm Volatile("mov %%cr3, %0" : "=r"(ret));																						// Current page directory (physical address) is in CR3
	return ret;
}

Void MmSwitchDirectory(UIntPtr dir) {
	if (dir == 0) {																													// Null pointer?
		return;																														// Yes, so just return
	}
	
	Asm Volatile("mov %0, %%cr3" :: "r"(dir));																						// Switch the CR3!
}
