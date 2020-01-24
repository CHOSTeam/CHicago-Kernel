// File author is Ítalo Lima Marconato Matias
//
// Created on June 28 of 2018, at 19:19 BRT
// Last edited on January 23 of 2020, at 21:30 BRT

#include <chicago/arch/vmm.h>

#include <chicago/mm.h>
#include <chicago/string.h>

UIntPtr MmGetPhys(UIntPtr virt) {
	if ((MmGetP4(virt) & PAGE_PRESENT) != PAGE_PRESENT) {																		// This P4 entry exists?
		return 0;																												// Nope
	} else if ((MmGetP3(virt) & PAGE_PRESENT) != PAGE_PRESENT) {																// This P3 entry exists?
		return 0;																												// Nope
	} else if ((MmGetP3(virt) & PAGE_HUGE) == PAGE_HUGE) {																		// 1GiB P3 instead of 2MiB P2 or 4KiB P1?
		return (MmGetP3(virt) & PAGE_MASK) + (virt & 0x3FFFFFFF);																// Yes, return
	} else if ((MmGetP2(virt) & PAGE_PRESENT) != PAGE_PRESENT) {																// This P2 entry exists?
		return 0;																												// Nope
	} else if ((MmGetP2(virt) & PAGE_HUGE) == PAGE_HUGE) {																		// 2MiB P2 instead of 4KiB P1?
		return (MmGetP2(virt) & PAGE_MASK) + (virt & 0x1FFFFF);																	// Yes, return
	} else {
		return (MmGetP1(virt) & PAGE_MASK) + (virt & 0xFFF);																	// Return!
	}
}

UInt32 MmQuery(UIntPtr virt) {
	UIntPtr page = 0;
	
	if ((MmGetP4(virt) & PAGE_PRESENT) != PAGE_PRESENT) {																		// This P4 entry exists?
		return 0;																												// Nope
	} else if ((MmGetP3(virt) & PAGE_PRESENT) != PAGE_PRESENT) {																// This P3 entry exists?
		return 0;																												// Nope
	} else if ((MmGetP3(virt) & PAGE_HUGE) == PAGE_HUGE) {																		// 1GiB P3 instead of 2MiB P2 or 4KiB P1?
		page = MmGetP3(virt);																									// Yes
	} else if ((MmGetP2(virt) & PAGE_PRESENT) != PAGE_PRESENT) {																// This P2 entry exists?
		return 0;																												// Nope
	} else if ((MmGetP2(virt) & PAGE_HUGE) == PAGE_HUGE) {																		// 2MiB P2 instead of 4KiB P1?
		page = MmGetP2(virt);																									// Yes
	} else if ((MmGetP1(virt) & PAGE_PRESENT) != PAGE_PRESENT) {																// This P1 entry exists?
		return 0;																												// Nope
	} else {
		page = MmGetP1(virt);																									// Yes
	}
	
	UInt32 ret = MM_MAP_READ;																									// And convert the page flags to MmMap flags
	
	if ((page & PAGE_WRITE) == PAGE_WRITE) {
		ret |= MM_MAP_WRITE;
	}
	
	if ((page & PAGE_USER) == PAGE_USER) {
		ret |= (MM_MAP_USER | MM_MAP_KERNEL);
	} else {
		ret |= MM_MAP_KERNEL;
	}
	
	if ((page & PAGE_AOR) == PAGE_AOR) {
		ret |= MM_MAP_AOR;
	}
	
	if ((page & PAGE_NOEXEC) != PAGE_NOEXEC) {
		ret |= MM_MAP_EXEC;
	}
	
	return ret;
}

static Boolean MmIsAvaliable(UIntPtr page) {
	return ((page & PAGE_PRESENT) != PAGE_PRESENT) && ((page & PAGE_AOR) != PAGE_AOR);
}

UIntPtr MmFindFreeVirt(UIntPtr start, UIntPtr end, UIntPtr count) {
	if (start % MM_PAGE_SIZE != 0) {																							// Page align the start
		start -= count % MM_PAGE_SIZE;
	}
	
	if (end % MM_PAGE_SIZE != 0) {																								// Page align the end
		end += MM_PAGE_SIZE - (end % MM_PAGE_SIZE);
	}
	
	if (count % MM_PAGE_SIZE != 0) {																							// Page align the count
		count += MM_PAGE_SIZE - (count % MM_PAGE_SIZE);
	}
	
	UIntPtr c = 0;
	UIntPtr p = start;
	
	for (UIntPtr i = start; i < end; i += 0x8000000000) {																		// Let's try to find the first free virtual address!
		if (MmIsAvaliable(MmGetP4(i))) {																						// This P4 is allocated?
			c += 0x8000000000;																									// No!
			
			if (i < 0x8000000000) {																								// 0x00000000?
				c -= MM_PAGE_SIZE;																								// Yes, but it's reserved...
				p += MM_PAGE_SIZE;
			}
			
			if (c >= count) {																									// We need more memory?
				return p;																										// No, so return!
			}
			
			continue;
		}
		
		for (UIntPtr j = 0; j < 0x8000000000; j += 0x40000000) {																// Let's check the P3s
			if (MmIsAvaliable(MmGetP3(i + j))) {																				// This P3 is allocated?
				c += 0x40000000;																								// No!
				
				if ((i < 0x8000000000) && (j < 0x40000000)) {																	// 0x00000000?
					c -= MM_PAGE_SIZE;																							// Yes, but it's reserved...
					p += MM_PAGE_SIZE;
				}
				
				if (c >= count) {																								// We need more memory?
					return p;																									// No, so return!
				}
				
				continue;
			} else if ((MmGetP3(i + j) & PAGE_HUGE) == PAGE_HUGE) {																// 1GiB P3 instead of 2MiB P2 (or 4KiB P1)?
				c = 0;																											// Yes :(
				p = i + j + 0x40000000;
				continue;
			}
			
			for (UIntPtr k = 0; k < 0x40000000; k += 0x200000) {																// Let's check the P2s
				if (MmIsAvaliable(MmGetP2(i + j + k))) {																		// This P2 is allocated?
					c += 0x200000;																								// No!
					
					if ((i < 0x8000000000) && (j < 0x40000000) && (k < 0x200000)) {												// 0x00000000?
						c -= MM_PAGE_SIZE;																						// Yes, but it's reserved...
						p += MM_PAGE_SIZE;
					}
					
					if (c >= count) {																							// We need more memory?
						return p;																								// No, so return!
					}

					continue;
				} else if ((MmGetP2(i + j + k) & PAGE_HUGE) == PAGE_HUGE) {														// 2MiB P2 instead of 4KiB P1?
					c = 0;																										// Yes :(
					p = i + j + k + 0x200000;
					continue;
				}
				
				for (UIntPtr l = 0; l < 0x200000; l += 0x1000) {																// Let's check the P1s
					if (((i == 0) && (j == 0) && (k == 0) && (l == 0)) || !MmIsAvaliable(MmGetP1(i + j + k + l))) {				// This P1 is allocated?
						c = 0;																									// Yes :(
						p = i + j + k + l + 0x1000;
						continue;
					}
					
					c += 0x1000;																								// It's free, so we can use it!
					
					if (c >= count) {																							// We need more memory?
						return p;																								// No, so return!
					}
				}
			}
		}
	}
	
	return 0;																													// We failed
}

UIntPtr MmFindHighestFreeVirt(UIntPtr start, UIntPtr end, UIntPtr count) {
	if (start % MM_PAGE_SIZE != 0) {																							// Page align the start
		start -= count % MM_PAGE_SIZE;
	}
	
	if (end % MM_PAGE_SIZE != 0) {																								// Page align the end
		end += MM_PAGE_SIZE - (end % MM_PAGE_SIZE);
	}
	
	if (count % MM_PAGE_SIZE != 0) {																							// Page align the count
		count += MM_PAGE_SIZE - (count % MM_PAGE_SIZE);
	}
	
	UIntPtr c = 0;
	UIntPtr p = end;
	
	for (UIntPtr i = end - 0x8000000000; i > start; i -= 0x8000000000) {														// Let's try to find the first free virtual address!
		if (MmIsAvaliable(MmGetP4(i))) {																						// This P4 is allocated?
			c += 0x8000000000;																									// No!
			
			if (i == 0) {																										// 0x00000000?
				c -= MM_PAGE_SIZE;																								// Yes, but it's reserved...
			}
			
			if (c >= count) {																									// We need more memory?
				return p - count;																								// No, so return!
			}
			
			continue;
		}
		
		for (UIntPtr j = 0x7FC0000000; j > 0; j -= 0x40000000) {																// Alright, so let's check the P3s!
			if (MmIsAvaliable(MmGetP3(i + j))) {																				// This P3 is allocated?
				c += 0x40000000;																								// No!
				
				if ((i == 0) && (j == 0)) {																						// 0x00000000?
					c -= MM_PAGE_SIZE;																							// Yes, but it's reserved...
				}
				
				if (c >= count) {																								// We need more memory?
					return p - count;																							// No, so return!
				}
				
				continue;
			} else if ((MmGetP3(i + j) & PAGE_HUGE) == PAGE_HUGE) {																// 1GiB P3 instead of 2MiB P2 (or 4KiB P1)?
				c = 0;																											// Yes :(
				p = i + j - 0x40000000;
			}
			
			for (UIntPtr k = 0x3FE00000; k > 0; k -= 0x200000) {																// Let's check the P2s
				if (MmIsAvaliable(MmGetP2(i + j + k))) {																		// This P2 is allocated?
					c += 0x200000;																								// No!
					
					if ((i == 0) && (j == 0) && (k == 0)) {																		// 0x00000000?
						c -= MM_PAGE_SIZE;																						// Yes, but it's reserved...
					}
					
					if (c >= count) {																							// We need more memory?
						return p - count;																						// No, so return!
					}
					
					continue;
				} else if ((MmGetP2(i + j + k) & PAGE_HUGE) == PAGE_HUGE) {														// 2MiB P2 instead of 4KiB P1?
					c = 0;																										// Yes :(
					p = i + j + k - 0x200000;
				}
				
				for (UIntPtr l = 0x200000; l > 0; l -= 0x1000) {																// Let's check the P1s
					if ((i == start) && (j == 0) && (k == 0) && (l == 0)) {														// curr == start?
						return 0;																								// Yes, we failed :(
					} else if (MmIsAvaliable(MmGetP1(i + j + k + l - 1))) {														// This P1 is allocated?
						c += 0x1000;																							// No!
						
						if (c >= count) {																						// We need more memory?
							return p - count;																					// No, so return!
						}
						
						continue;
					}
					
					c = 0;																										// Yes :(
					p = i + j + k + l - 0x1000;
				}
			}
		}
	}
	
	return 0;																													// We failed
}

UIntPtr MmMapTemp(UIntPtr phys, UInt32 flags) {
	for (UIntPtr i = 0xFFFFFF0000000000; i < 0xFFFFFF8000000000; i += MM_PAGE_SIZE) {											// Let's try to find an free temp address
		if (MmQuery(i) == 0) {																									// Free?
			if (MmMap(i, phys, flags) == STATUS_SUCCESS) {																		// Yes, so try to map it
				return i;																										// Mapped! Now we only need to return it
			}
		}
	}
	
	return 0;																													// Failed...
}

Status MmMap(UIntPtr virt, UIntPtr phys, UInt32 flags) {
	if ((virt % MM_PAGE_SIZE) != 0) {																							// Align to page size
		virt -= virt % MM_PAGE_SIZE;
	}
	
	if ((phys % MM_PAGE_SIZE) != 0) {																							// Align to page size
		phys -= phys % MM_PAGE_SIZE;
	}
	
	UInt32 flags2 = ((flags & MM_MAP_AOR) == MM_MAP_AOR) ? PAGE_AOR : PAGE_PRESENT;												// Convert the MmMap flags to page flags
	
	if ((flags & MM_MAP_WRITE) == MM_MAP_WRITE) {																				// Writeable?
		flags2 |= PAGE_WRITE;																									// Yes
	}
	
	if (((flags & MM_MAP_USER) == MM_MAP_USER) && ((flags & MM_MAP_KERNEL) == MM_MAP_KERNEL)) {									// User mode?
		flags2 |= PAGE_USER;																									// Yes
	} else if ((flags & MM_MAP_USER) == MM_MAP_USER) {
		flags2 |= PAGE_USER;																									// ^
	}
	
	if ((flags & MM_MAP_EXEC) != MM_MAP_EXEC) {																					// Enable code execution?
		flags2 |= PAGE_NOEXEC;																									// No
	}
	
	if ((MmGetP4(virt) & PAGE_PRESENT) != PAGE_PRESENT) {																		// This P4 entry exists?
		UIntPtr block;																											// No, so let's alloc it
		
		if (MmReferenceSinglePage(0, &block)) {																					// Failed?
			return STATUS_OUT_OF_MEMORY;																						// Then just return
		}
		
		if (virt >= 0xFFFF800000000000) {																						// Kernel-only page directory?
			MmSetP4(virt, block, 0x03);																							// Yes, so put the p4 entry as present, writeable
		} else {
			MmSetP4(virt, block, 0x07);																							// No, so put the p4 entry as present, writeable and set the user bit
		}
		
		MmInvlpg(MmGetP3Loc(virt));																								// Update the TLB
		StrSetMemory((PUInt8)(MmGetP3Loc(virt)), 0, 0x1000);																	// And clean the p3 entries
	}
	
	if ((MmGetP3(virt) & PAGE_HUGE) == PAGE_HUGE) {																				// This P3 entry is a huge one? (1GiB page)
		return STATUS_MAP_ERROR;																								// Yes, but sorry, we don't support mapping it YET
	} else if ((MmGetP3(virt) & PAGE_PRESENT) != PAGE_PRESENT) {																// This P3 entry exists?
		UIntPtr block;																											// No, so let's alloc it
		
		if (MmReferenceSinglePage(0, &block)) {																					// Failed?
			return STATUS_OUT_OF_MEMORY;																						// Then just return
		}
		
		if (virt >= 0xFFFF800000000000) {																						// Kernel-only page directory?
			MmSetP3(virt, block, 0x03);																							// Yes, so put the p3 entry as present, writeable
		} else {
			MmSetP3(virt, block, 0x07);																							// No, so put the p3 entry as present, writeable and set the user bit
		}
		
		MmInvlpg(MmGetP2Loc(virt));																								// Update the TLB
		StrSetMemory((PUInt8)(MmGetP2Loc(virt)), 0, 0x1000);																	// And clean the p2 entries
	}
	
	if ((MmGetP2(virt) & PAGE_HUGE) == PAGE_HUGE) {																				// This P2 entry is a huge one? (2MiB page)
		return STATUS_MAP_ERROR;																								// Yes, but sorry, we don't support mapping it YET
	} else if ((MmGetP2(virt) & PAGE_PRESENT) != PAGE_PRESENT) {																// This P2 entry exists?
		UIntPtr block;																											// No, so let's alloc it
		
		if (MmReferenceSinglePage(0, &block) != STATUS_SUCCESS) {																// Failed?
			return STATUS_OUT_OF_MEMORY;																						// Then just return
		}
		
		if (virt >= 0xFFFF800000000000) {																						// Kernel-only page directory?
			MmSetP2(virt, block, 0x03);																							// Yes, so put the p3 entry as present, writeable
		} else {
			MmSetP2(virt, block, 0x07);																							// No, so put the p3 entry as present, writeable and set the user bit
		}
		
		MmInvlpg(MmGetP1Loc(virt));																								// Update the TLB
		StrSetMemory((PUInt8)(MmGetP1Loc(virt)), 0, 0x1000);																	// And clean the p2 entries
	}
	
	MmSetP1(virt, phys, flags2);																								// Map the phys addr to the virt addr
	MmInvlpg(virt);																												// Update the TLB
	
	return STATUS_SUCCESS;
}

Status MmUnmap(UIntPtr virt) {
	if ((MmGetP4(virt) & PAGE_PRESENT) != PAGE_PRESENT) {																		// This P4 entry exists?
		return STATUS_NOT_MAPPED;																								// Nope
	} else if ((MmGetP3(virt) & PAGE_PRESENT) != PAGE_PRESENT) {																// This P3 entry exists?
		return STATUS_NOT_MAPPED;																								// Nope
	} else if ((MmGetP3(virt) & PAGE_HUGE) == PAGE_HUGE) {																		// This P3 entry is a huge one? (1GiB page)
		return STATUS_UNMAP_ERROR;																								// Yes, but sorry, we don't support mapping it YET
	} else if ((MmGetP2(virt) & PAGE_PRESENT) != PAGE_PRESENT) {																// This P2 entry exists?
		return STATUS_NOT_MAPPED;																								// Nope
	} else if ((MmGetP2(virt) & PAGE_HUGE) == PAGE_HUGE) {																		// This P2 entry is a huge one? (2MiB page)
		return STATUS_UNMAP_ERROR;																								// Yes, but sorry, we don't support mapping it YET
	} else if ((MmGetP1(virt) & PAGE_PRESENT) != PAGE_PRESENT) {																// This P1 entry exists?
		return STATUS_NOT_MAPPED;																								// Nope
	} else {
		MmSetP1(virt, 0, 0);																									// Yes, so unmap the virt addr
		MmInvlpg(virt);																											// Update the TLB
		return STATUS_SUCCESS;
	}
}

Status MmPrepareMapFile(UIntPtr start, UIntPtr end) {
	if ((start % MM_PAGE_SIZE) != 0) {																							// Align everything to page size
		start -= start % MM_PAGE_SIZE;
	}
	
	for (UIntPtr i = start; i < end; i += MM_PAGE_SIZE) {																		// First, let's check everything...
		if ((MmGetP4(i) & PAGE_PRESENT) != PAGE_PRESENT) {																		// This P4 entry exists?
			return STATUS_NOT_MAPPED;																							// Nope
		} else if ((MmGetP3(i) & PAGE_PRESENT) != PAGE_PRESENT) {																// This P3 entry exists?
			return STATUS_NOT_MAPPED;																							// Nope
		} else if ((MmGetP3(i) & PAGE_HUGE) == PAGE_HUGE) {																		// This P3 entry is a huge one? (1GiB page)
			return STATUS_MAPFILE_FAILED;																						// Yes, but sorry, we don't support mapping it YET
		} else if ((MmGetP2(i) & PAGE_PRESENT) != PAGE_PRESENT) {																// This P2 entry exists?
			return STATUS_NOT_MAPPED;																							// Nope
		} else if ((MmGetP2(i) & PAGE_HUGE) == PAGE_HUGE) {																		// This P2 entry is a huge one? (2MiB page)
			return STATUS_MAPFILE_FAILED;																						// Yes, but sorry, we don't support mapping it YET
		}
		
		UIntPtr page = MmGetP1(i);																								// Get the PTE entry
		
		if ((page & PAGE_PRESENT) != PAGE_PRESENT && (page & PAGE_AOR) != PAGE_AOR) {											// Not mapped (or already mapped to another mapped file)...
			return STATUS_NOT_MAPPED;
		} else if ((page & PAGE_OTHER) == PAGE_OTHER) {																			// Already mapped to another file?
			return STATUS_ALREADY_MAPPED;																						// Yup...
		}
	}
	
	for (; start < end; start += MM_PAGE_SIZE) {																				// Now, let's just set the PAGE_OTHER flag in everything, and also unset the present flag in everything
		UIntPtr page = MmGetP1(start);
		MmSetP1(start, page & PAGE_MASK, ((page & 0xFFF) | PAGE_OTHER) & ~PAGE_PRESENT);
		MmInvlpg(start);
	}
	
	return STATUS_SUCCESS;
}

Status MmPrepareUnmapFile(UIntPtr start, UIntPtr end) {
	if ((start % MM_PAGE_SIZE) != 0) {																							// Align everything to page size
		start -= start % MM_PAGE_SIZE;
	}
	
	for (UIntPtr i = start; i < end; i += MM_PAGE_SIZE) {																		// First, let's check everything...
		if ((MmGetP4(i) & PAGE_PRESENT) != PAGE_PRESENT) {																		// This P4 entry exists?
			return STATUS_NOT_MAPPED;																							// Nope
		} else if ((MmGetP3(i) & PAGE_PRESENT) != PAGE_PRESENT) {																// This P3 entry exists?
			return STATUS_NOT_MAPPED;																							// Nope
		} else if ((MmGetP3(i) & PAGE_HUGE) == PAGE_HUGE) {																		// This P3 entry is a huge one? (1GiB page)
			return STATUS_MAPFILE_FAILED;																						// Yes, but sorry, we don't support mapping it YET
		} else if ((MmGetP2(i) & PAGE_PRESENT) != PAGE_PRESENT) {																// This P2 entry exists?
			return STATUS_NOT_MAPPED;																							// Nope
		} else if ((MmGetP2(i) & PAGE_HUGE) == PAGE_HUGE) {																		// This P2 entry is a huge one? (2MiB page)
			return STATUS_MAPFILE_FAILED;																						// Yes, but sorry, we don't support mapping it YET
		}
		
		UIntPtr page = MmGetP1(i);																								// Get the PTE entry
		
		if ((page & PAGE_PRESENT) != PAGE_PRESENT && (page & PAGE_OTHER) != PAGE_OTHER) {										// Not mapped...
			return STATUS_NOT_MAPPED;
		} else if ((page & PAGE_OTHER) != PAGE_OTHER) {																			// Not mapped to any file?
			return STATUS_UNMAPFILE_FAILED;																						// Yup...
		}
	}
	
	for (; start < end; start += MM_PAGE_SIZE) {																				// Now, let's just unset the PAGE_OTHER flag in everything, and also set the present flag (if required)
		UIntPtr page = MmGetP1(start);
		MmSetP1(start, page & PAGE_MASK, ((page & 0xFFF) & ~PAGE_OTHER) | (((page & PAGE_AOR) != PAGE_AOR) ? PAGE_PRESENT : 0));
		MmInvlpg(start);
	}
	
	return STATUS_SUCCESS;
}

extern UIntPtr MmKernelDirectoryInt;

UIntPtr MmCreateDirectory(Void) {
	PUInt64 cp4 = (PUInt64)0xFFFFFFFFFFFFF000;
	UIntPtr ret;																												// Allocate one physical page
	PUInt64 p4 = Null;
	
	if (MmReferenceSinglePage(0, &ret) != STATUS_SUCCESS) {																		// Failed?
		return 0;																												// Yes...
	}
	
	if ((p4 = (PUInt64)MmMapTemp(ret, MM_MAP_KDEF)) == 0) {																		// Try to map it to an temp addr
		MmDereferenceSinglePage(ret);																							// Failed...
		return 0;
	}
	
	for (UInt64 i = 0; i < 512; i++) {																							// Let's fill the P4!
		if (((cp4[i] & PAGE_PRESENT) != PAGE_PRESENT) || (!((i == 256) || (i == 257) || (i == 511)))) {							// We're going to copy this p4 entry?
			p4[i] = 0;																											// Nope
		} else if (i == 511) {																									// Recursive mapping entry?
			p4[i] = (ret & PAGE_MASK) | 3;																						// Yes
		} else {
			p4[i] = cp4[i];																										// Just copy it
		}
	}
	
	MmUnmap((UIntPtr)p4);																										// Unmap the temp addr
	
	return ret;
}

Void MmFreeDirectory(UIntPtr dir) {
	if ((dir == 0) || (dir == MmGetCurrentDirectory())) {																		// Sanity checks
		return;
	}
	
	PUInt64 p4 = Null;
	
	if ((p4 = (PUInt64)MmMapTemp(dir, MM_MAP_KDEF)) == 0) {																		// Let's try to map us to an temp addr
		MmDereferenceSinglePage(dir);																							// Failed, so just free the dir physical address
		return;
	}
	
	for (UInt64 i = 0; i < 512; i++) {																							// Let's go!
		if ((i == 256) || (i == 257) || (i == 511)) {																			// Kernel entry or recursive mapping entry?
			continue;																											// Yes
		} else if ((p4[i] & PAGE_PRESENT) != PAGE_PRESENT) {																	// This P4 entry exists?
			continue;																											// Nope
		}
		
		PUInt64 p3 = Null;																										// Let's map it!
			
		if ((p3 = (PUInt64)MmMapTemp(p4[i] & PAGE_MASK, MM_MAP_KDEF)) == 0) {
			MmDereferenceSinglePage(p4[i] & PAGE_MASK);
			continue;
		}
		
		for (UInt64 j = 0; j < 512; j++) {																						// Let's free the P3 entries
			if ((p3[j] & PAGE_PRESENT) != PAGE_PRESENT) {																		// This P3 entry exists?
				continue;																										// Nope
			} else if ((p3[j] & PAGE_HUGE) == PAGE_HUGE) {																		// Huge P3 page (1 GiB)?
				MmDereferenceSinglePage(p3[j] & PAGE_MASK);																		// Yes, free it!
				continue;
			}
			
			PUInt64 p2 = Null;																									// Let's map it!
			
			if ((p2 = (PUInt64)MmMapTemp(p3[j] & PAGE_MASK, MM_MAP_KDEF)) == 0) {
				MmDereferenceSinglePage(p3[j] & PAGE_MASK);
				continue;
			}
			
			for (UInt64 k = 0; k < 512; k++) {																					// Let's free the P2 entries
				if ((p2[k] & PAGE_PRESENT) != PAGE_PRESENT) {																	// This P2 entry exists?
					continue;																									// Nope
				} else if ((p2[k] & PAGE_HUGE) == PAGE_HUGE) {																	// Huge P2 page (2 MiB)?
					MmDereferenceSinglePage(p2[k] & PAGE_MASK);																	// Yes, free it!
					continue;
				}
				
				PUInt64 p1 = Null;																								// Let's map it!

				if ((p1 = (PUInt64)MmMapTemp(p2[k] & PAGE_MASK, MM_MAP_KDEF)) == 0) {
					MmDereferenceSinglePage(p2[k] & PAGE_MASK);
					continue;
				}
				
				for (UInt64 l = 0; l < 512; l++) {																				// Let's free the P1 entries
					if ((p1[l] & PAGE_PRESENT) == PAGE_PRESENT) {																// Present?
						MmDereferenceSinglePage(p1[l] & PAGE_MASK);																// Yes, free it
					}
				}
				
				MmUnmap((UIntPtr)p1);																							// Unmap the P1
				MmDereferenceSinglePage(p2[k] & PAGE_MASK);																		// And free it
			}
			
			MmUnmap((UIntPtr)p2);																								// Unmap the P2
			MmDereferenceSinglePage(p3[j] & PAGE_MASK);																			// And free it
		}
		
		MmUnmap((UIntPtr)p3);																									// Unmap the P3
		MmDereferenceSinglePage(p4[i] & PAGE_MASK);																				// And free it
	}
	
	MmUnmap((UIntPtr)p4);																										// Unmap the P4
	MmDereferenceSinglePage(dir);																								// And free it
}

UIntPtr MmGetCurrentDirectory(Void) {
	UIntPtr ret;
	Asm Volatile("mov %%cr3, %0" : "=r"(ret));																					// Current page directory (physical address) is in CR3
	return ret;
}

Void MmSwitchDirectory(UIntPtr dir) {
	if (dir == 0) {																												// Null pointer?
		return;																													// Yes, so just return
	}
	
	Asm Volatile("mov %0, %%cr3" :: "r"(dir));																					// Switch the CR3!
}
