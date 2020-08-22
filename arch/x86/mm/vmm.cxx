/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 03 of 2020, at 22:55 BRT
 * Last edited on August 11 of 2020, at 15:35 BRT */

#include <chicago/arch/arch.hxx>
#include <chicago/arch/mm.hxx>
#include <chicago/string.hxx>
#include <chicago/textout.hxx>

/* Basic inline helpers for the other functions. */

static inline Void UpdateTLB(Void *VirtAddr) {
	asm volatile("invlpg (%0)" :: "r"((UIntPtr)VirtAddr & MM_PAGE_MASK) : "memory");
}

static inline UIntPtr *GetTableEntry(UIntPtr TableAddress, UIntPtr Index, UIntPtr IndexShift) {
	return (UIntPtr*)(TableAddress + (Index << IndexShift));
}

static inline Int8 CheckTableEntry(UIntPtr TableAddress, UIntPtr Index, UIntPtr IndexShift,
								   UIntPtr *&Entry, Boolean CleanEntry = False) {
	/* If we need to clean the entry, then, uh, clean the entry. */
	
	if (CleanEntry) {
		UpdateTLB(GetTableEntry(TableAddress, 0, 0));
		StrSetMemory(GetTableEntry(TableAddress, 0, 0), 0, MM_PAGE_SIZE);
		return -1;
	}
	
	/* Now, get the entry (GetTableEntry returns a pointer, so we can invlpg the page tables. */
	
	Entry = GetTableEntry(TableAddress, Index, IndexShift);
	
	/* Now, we need to check two things: first, if this isn't a present page, return -1 (error), if we're mapping a new virtual
	 * address, the map function should manually create that entry, if it's any other function, it should return that the memory
	 * address isn't mapped. If the entry is a huge page, it is already mapped, and the map function should fail, or whatever
	 * function called us should know that the page is already mapped. */
	
	if (!(*Entry & PAGE_PRESENT)) {
		return -1;
	} else if (*Entry & PAGE_HUGE) {
		return -2;
	}
	
	return 0;
}

static inline UIntPtr GetOff(UIntPtr VirtAddr, UIntPtr Size) {
	/* Is there even any better way to do this? lol. */
	
	return VirtAddr & (Size == 0 ? 0xFFF :
#ifdef ARCH_64
					   (Size == 2 ? 0x3FFFFFFF : 0x1FFFFF));
#else
					   0x3FFFFF);
#endif
}

static Status CheckPageTables(UIntPtr VirtAddr, UIntPtr *&OutEnt, UIntPtr &CurLvl,
							  Boolean CleanCur = False) {
	/* Just like on the PMM, we need to check if the address is not a null pointer */
	
	if (VirtAddr < MM_PAGE_SIZE) {
		return Status::InvalidArg;
	}
	
	/* We need to check each level of the page tables, there are 4 different levels on x86-64, and 2 on x86-32. This checking may break
	 * out early, if we find a huge page in any of the levels. Also, save the page indexes, as it makes the code a bit more readable. */
	
	Int8 ret = -1;
	
	VirtAddr &= MM_PAGE_MASK;
	
#ifdef ARCH_64
	UIntPtr pml4e = (VirtAddr >> 39) & 0x1FF, pdpe = (VirtAddr >> 30) & 0x1FF,
			pde = (VirtAddr >> 21) & 0x1FF, pte = (VirtAddr >> 12) & 0x1FF;
	
	/* For x86-64, we're also going to need some special values to add to the base table address (starting with the PDP entries). */
	
	UIntPtr pml4a = (VirtAddr >> 27) & 0x1FF000, pdpa = (VirtAddr >> 18) & 0x3FFFF000,
			pda = (VirtAddr >> 9) & 0x7FFFFFF000;
	
	switch (CurLvl) {
	case 1: {
		if ((ret = CheckTableEntry(0xFFFFFFFFFFFFF000, pml4e, 3, OutEnt, CleanCur)) == -1) {
			return Status::NotMapped;
		}
		
		CleanCur = False;
		CurLvl++;
	}
	case 2: {
		if ((ret = CheckTableEntry(0xFFFFFFFFFFE00000 + pml4a, pdpe, 3, OutEnt, CleanCur)) < 0) {
			return Status::NotMapped;
		}
		
		CleanCur = False;
		CurLvl++;
	}
	case 3: {
		if ((ret = CheckTableEntry(0xFFFFFFFFC0000000 + pdpa, pde, 3, OutEnt, CleanCur)) < 0) {
			return Status::NotMapped;
		}
		
		CleanCur = False;
		CurLvl++;
	}
	case 4: {
		ret = CheckTableEntry(0xFFFFFF8000000000 + pda, pte, 3, OutEnt, CleanCur);
		break;
	}
	}
#else
	UIntPtr pde = (VirtAddr >> 22) & 0xFFF, pte = (VirtAddr >> 12) & 0x3FF;
	
	switch (CurLvl) {
	case 1: {
		if ((ret = CheckTableEntry(0xFFFFF000, pde, 2, OutEnt, CleanCur)) < 0) {
			return Status::NotMapped;
		}
		
		CleanCur = False;
		CurLvl++;
	}
	case 2: {
		ret = CheckTableEntry(0xFFC00000 + (pde << 12), pte, 2, OutEnt, CleanCur);
		break;
	}
	}
#endif
	
	return (ret < -1 || ret >= 0) ? Status::Success : Status::NotMapped;
}

Status ArchImpl::GetPhys(Void *VirtAddr, UIntPtr &Out) {
	/* We can use the CheckPageTables function, as it will return the last entry (or an error status, if the memory is not mapped. */
	
	Status status;
	UIntPtr *ent = Null, lvl = 1;
	
	if ((status = CheckPageTables((UIntPtr)VirtAddr, ent, lvl)) != Status::Success) {
		return status;
	}
	
	Out = (*ent & MM_PAGE_MASK) | GetOff((UIntPtr)VirtAddr, lvl * (*ent & PAGE_HUGE));
	
	return Status::Success;
}

Status ArchImpl::Query(Void *VirtAddr, UInt32 &Out) {
	/* Here we can also use the CheckPageTables function! As it returns the whole entry, instead of returning only the physical
	 * address! */
	
	Status status;
	UIntPtr *ent = Null, lvl = 1;
	
	if ((status = CheckPageTables((UIntPtr)VirtAddr, ent, lvl)) != Status::Success) {
		return status;
	}
	
	/* Let's set the out variable to the minimum permissions/flags (read/exec on x86-32 and read-only on x86-64). */
	
#ifdef ARCH_64
	Out = MM_MAP_READ;
#else
	Out = MM_MAP_READ | MM_MAP_EXEC;
#endif
	
	/* And let's parse the page flags, there are some flags that only exists on x86-64, and some flags that implicitly means that
	 * another flag should also be set, so let's handle all of that. */
	
	if (*ent & PAGE_WRITE) {
		Out |= MM_MAP_WRITE;
	}
	
	if (*ent & PAGE_USER) {
		Out |= MM_MAP_USER | MM_MAP_KERNEL;
	} else {
		Out |= MM_MAP_KERNEL;
	}
	
	if (*ent & PAGE_HUGE) {
		Out |= MM_MAP_HUGE;
	}
	
	if (*ent & PAGE_COW) {
		Out |= MM_MAP_COW;
	}
	
#ifdef ARCH_64
	if (!(*ent & PAGE_NOEXEC)) {
		Out |= MM_MAP_EXEC;
	}
#endif
	
	return Status::Success;
}

Status ArchImpl::MapTemp(UIntPtr PhysAddr, UInt32 Flags, Void *&Out) {
	/* We should probably try to implement a differnt way to do this later, but, for now, let's just brute force a free address. */
	
	UInt32 discard;
	UIntPtr pz = (Flags & MM_MAP_HUGE) ? MM_HUGE_PAGE_SIZE : MM_PAGE_SIZE;
	
#ifdef ARCH_64
	for (UIntPtr i = 0xFFFFFF0000000000; i < 0xFFFFFF8000000000; i += pz) {
#else
	for (UIntPtr i = 0xFF800000; i < 0xFFC00000; i += pz) {
#endif
		if (Query((Void*)i, discard) == Status::NotMapped) {
			Out = (Void*)i;
			return Map(Out, PhysAddr, Flags);
		}
	}
	
	return Status::OutOfMemory;
}

Status ArchImpl::Map(Void *VirtAddr, UIntPtr PhysAddr, UInt32 Flags) {
	/* Before ANYTHING, we need to align the addresses, if the MM_MAP_HUGE flag is set, we need to align it to the huge page size,
	 * if it is not set, just align it to the normal page size. After that, we need to convert the map flags into page flags. */
	
	if (Flags & MM_MAP_HUGE) {
		VirtAddr = (Void*)((UIntPtr)VirtAddr & MM_HUGE_PAGE_MASK);
		PhysAddr &= MM_HUGE_PAGE_MASK;
	} else {
		VirtAddr = (Void*)((UIntPtr)VirtAddr & MM_PAGE_MASK);
		PhysAddr &= PhysAddr;
	}
	
	UInt32 flgs = (Flags & MM_MAP_AOR) ? PAGE_AOR : PAGE_PRESENT;
	
	if (Flags & MM_MAP_USER) {
		flgs |= PAGE_USER;
	}
	
	if (Flags & MM_MAP_COW) {
		flgs |= PAGE_COW;
	} else if (Flags & MM_MAP_WRITE) {
		flgs |= PAGE_WRITE;
	}
	
	if (Flags & MM_MAP_HUGE) {
		flgs |= PAGE_HUGE;
	}
	
#ifdef ARCH_64
	if (!(Flags & MM_MAP_EXEC)) {
		flgs |= PAGE_NOEXEC;
	}
#endif
	
	/* Now, it's time to recursevely alloc all the page levels until we reach the one where we need to put the phys address we're
	 * going to map. */
	
	Status status;
	
#ifdef ARCH_64
	UIntPtr *ent = Null, lvl = 1, dlvl = (Flags & MM_MAP_HUGE) ? 3 : 4;
#else
	UIntPtr *ent = Null, lvl = 1, dlvl = (Flags & MM_MAP_HUGE) ? 1 : 2;
#endif
	
	while ((status = CheckPageTables((UIntPtr)VirtAddr, ent, lvl)) != Status::Success) {
		/* If we reached this code block, the current level doesn't exists, and we need to manually alloc it. We only need to alloc
		 * the physical address, as it will automatically get a virtual address (thanks to the recursive mapping that we did).
		 * But wait, do we really need to alloc it? If we are already at the wanted level, we don't need to manually alloc a new
		 * physical address! */
		
		if (lvl == dlvl) {
			break;
		}
		
		UIntPtr phys;
		
		if ((status = PhysMem::ReferenceSingle(0, phys)) != Status::Success) {
			return status;
		}
		
#ifdef ARCH_64
		if ((UIntPtr)VirtAddr >= 0xFFFF800000000000) {
#else
		if ((UIntPtr)VirtAddr >= 0xC0000000) {
#endif
			*ent = phys | 0x03;
		} else {
			*ent = phys | 0x07;
		}
		
		lvl++;
		CheckPageTables((UIntPtr)VirtAddr, ent, lvl, True);
	}
	
	/* If the address is already mapped, return that the user should probably unmap it before calling us again. */
	
	if (status != Status::NotMapped) {
		return status == Status::Success ? Status::AlreadyMapped : status;
	}
	
	*ent = PhysAddr | flgs;
	UpdateTLB(VirtAddr);
	
	return Status::Success;
}

Status ArchImpl::Unmap(Void *VirtAddr) {
	/* Unmapping is pretty easy, check all the page table levels, if anything except for Status::Success is the return value, probably
	 * this address is not mapped, else, unset the present bit on the entry. */
	
	Status status;
	UIntPtr *ent = Null, lvl = 1;
	
	if ((status = CheckPageTables((UIntPtr)VirtAddr, ent, lvl)) != Status::Success) {
		return status;
	}
	
	*ent &= ~PAGE_PRESENT;
	UpdateTLB(VirtAddr);
	
	return Status::Success;
}

Status ArchImpl::CreateDirectory(UIntPtr &Out) {
	/* We need to get the outer most page table level (PML4 on x86-64 and PD on x86-32), after saving that, we need to allocate one
	 * physical page for the new directory, and temp map it into the memory. */
	
#ifdef ARCH_64
	UIntPtr *cdir = (UIntPtr*)0xFFFFFFFFFFFFF000;
#else
	UIntPtr *cdir = (UIntPtr*)0xFFFFF000;
#endif
	UIntPtr *dir = Null;
	Status status;
	
	if ((status = PhysMem::ReferenceSingle(0, Out)) != Status::Success) {
		return status;
	} else if ((status = MapTemp(Out, MM_MAP_KDEF, (Void*&)dir)) != Status::Success) {
		PhysMem::DereferenceSingle(Out);
		return status;
	}
	
	/* Now, let's copy all the entries from the current directory into the new one, we have to remember that we SHOULDN'T copy the user
	 * pages, that we should make the recursive page directory entry, and that we SHOULDN'T copy any of the pages that we use for temp
	 * mappings. */
	
#ifdef ARCH_64
	UIntPtr max = 512;
#else
	UIntPtr max = 1024;
#endif
	
	for (UInt32 i = 0; i < max; i++) {
		if (!(cdir[i] & PAGE_PRESENT) ||
#ifdef ARCH_64
			i < 256 || i == 510) {
#else
			i < 768 || i == 1022) {
#endif
			dir[i] = 0;
		} else if (i == max - 1) {
			dir[i] = (Out & MM_PAGE_MASK) | 3;
		} else {
			dir[i] = cdir[i];
		}
	}
	
	Unmap(dir);
	
	return Status::Success;
}

/* We could move this function into two files (one for x86-32 and one for x86-64), but, at least for now, let's implement all here. */

#ifdef ARCH_64
Status ArchImpl::FreeDirectory(UIntPtr Directory) {
	/* We need to check if the directory is valid, and if we're not trying to free the current directory, freeing the current directory
	 * is unsupported (well, we could go into the kernel directory, but for now let's just return InvalidArg). */
	
	if (Directory == 0 || Directory == GetCurrentDirectory()) {
		return Status::InvalidArg;
	}
	
	/* We need to map the directory into memory, fortunally, we have a function that allow us to find a temp address to map it, we just
	 * need to remember to unmap it later, else we may run out of temp space later on. */
	
	Status ret = Status::Success;
	Status status;
	UIntPtr *pml4 = Null;
	
	if ((status = MapTemp(Directory, MM_MAP_KDEF, (Void*&)pml4)) != Status::Success) {
		return status;
	}
	
	/* Now there are somethings to take in consideration: The kernel starts at the page directory index 256, so we should only free things
	 * below that index. Also, we need to free the temp mappings, and they are at the page directory index 510 (yes, x86-64 have half the
	 * amount of entries per directory level, but double the amount of levels). */
	
	for (UIntPtr i = 0; i < 512; i++) {
		if (i >= 256 && i != 510) {
			continue;
		} else if (!(pml4[i] & PAGE_PRESENT)) {
			continue;
		}
		
		/* Now that we're sure that this entry doesn't contain critical kernel data, and is actually present, let's go map its PDP. */
		
		UIntPtr *pdp = Null;
		
		if ((status = MapTemp(pml4[i] & MM_PAGE_MASK, MM_MAP_KDEF, (Void*&)pdp)) != Status::Success) {
			if (ret == Status::Success) {
				ret = status;
			}
			
			PhysMem::DereferenceSingle(pml4[i] & MM_PAGE_MASK);
			continue;
		}
		
		for (UIntPtr j = 0; j < 512; j++) {
			/* One extra check we need to do here (and in pretty much all the other level, except for the PT, and, of course, the PML4):
			 * We need to check if this is a huge page, if it is, we don't need to iterate through it. */
			
			if (!(pdp[j] & PAGE_PRESENT)) {
				continue;
			} else if (pdp[j] & PAGE_HUGE) {
				/* We don't really support 1GiB pages (huge pages at the PDP), but anyways, let's handle it. */
				
				PhysMem::DereferenceSingle(pdp[j] & 0xFFFFFFFFC0000000);
				continue;
			}
			
			UIntPtr *pd = Null;
			
			if ((status = MapTemp(pdp[j] & MM_PAGE_MASK, MM_MAP_KDEF, (Void*&)pd)) != Status::Success) {
				if (ret == Status::Success) {
					ret = status;
				}
				
				PhysMem::DereferenceSingle(pdp[j] & MM_PAGE_MASK);
				continue;
			}
			
			for (UIntPtr k = 0; k < 512; k++) {
				if (!(pd[j] & PAGE_PRESENT)) {
					continue;
				} else if (pd[j] & PAGE_HUGE) {
					/* And now we DO support huge PD pages (2MiB pages), so we actually have a macro to get the physical address. */
					
					PhysMem::DereferenceSingle(pd[k] & MM_HUGE_PAGE_MASK);
					continue;
				}
				
				UIntPtr *pt = Null;
				
				if ((status = MapTemp(pd[k] & MM_PAGE_MASK, MM_MAP_KDEF, (Void*&)pt)) != Status::Success) {
					if (ret == Status::Success) {
						ret = status;
					}
					
					PhysMem::DereferenceSingle(pd[k] & MM_PAGE_MASK);
					continue;
				}
				
				for (UIntPtr l = 0; l < 512; l++) {
					/* Finally, we're at the lowest level, no need to check for huge pages, or map and iterate. */
					
					if (pt[l] & PAGE_PRESENT) {
						PhysMem::DereferenceSingle(pt[l] & MM_PAGE_MASK);
					}
				}
				
				Unmap(pt);
				PhysMem::DereferenceSingle(pd[k] & MM_PAGE_MASK);
			}
			
			Unmap(pd);
			PhysMem::DereferenceSingle(pdp[j] & MM_PAGE_MASK);
		}
		
		Unmap(pdp);
		PhysMem::DereferenceSingle(pml4[i] & MM_PAGE_MASK);
	}
	
	Unmap(pml4);
	PhysMem::DereferenceSingle(Directory);
	
	return ret;
}
#else
Status ArchImpl::FreeDirectory(UIntPtr Directory) {
	/* We need to check if the directory is valid, and if we're not trying to free the current directory, freeing the current directory
	 * is unsupported (well, we could go into the kernel directory, but for now let's just return InvalidArg). */
	
	if (Directory == 0 || Directory == GetCurrentDirectory()) {
		return Status::InvalidArg;
	}
	
	/* We need to map the directory into memory, fortunally, we have a function that allow us to find a temp address to map it, we just
	 * need to remember to unmap it later, else we may run out of temp space later on. */
	
	Status ret = Status::Success;
	Status status;
	UIntPtr *pd = Null;
	
	if ((status = MapTemp(Directory, MM_MAP_KDEF, (Void*&)pd)) != Status::Success) {
		return status;
	}
	
	/* Now there are somethings to take in consideration: The kernel starts at the page directory index 768, so we should only free things
	 * below that index. Also, we need to free the temp mappings, and they are at the page directory index 1022. */
	
	for (UIntPtr i = 0; i < 1024; i++) {
		if (i >= 768 && i != 1022) {
			continue;
		} else if (!(pd[i] & PAGE_PRESENT)) {
			continue;
		} else if (pd[i] & PAGE_HUGE) {
			PhysMem::DereferenceSingle(pd[i] & MM_HUGE_PAGE_MASK);
			continue;
		}
		
		/* We need to make the page table into memory, but this time, if the mapping fail, we're not going to exit, let's just skip this
		 * entry (but still remember what the status was, so we can return it later. */
		
		UIntPtr *pt = Null;
		
		if ((status = MapTemp(pd[i] & MM_PAGE_MASK, MM_MAP_KDEF, (Void*&)pt)) != Status::Success) {
			if (ret == Status::Success) {
				ret = status;
			}
			
			PhysMem::DereferenceSingle(pd[i] & MM_PAGE_MASK);
			continue;
		}
		
		for (UIntPtr j = 0; j < 1024; j++) {
			if (pt[j] & PAGE_PRESENT) {
				PhysMem::DereferenceSingle(pt[i] & MM_PAGE_MASK);
			}
		}
		
		Unmap(pt);
		PhysMem::DereferenceSingle(pd[i] & MM_PAGE_MASK);
	}
	
	Unmap(pd);
	PhysMem::DereferenceSingle(Directory);
	
	return ret;
}
#endif

UIntPtr ArchImpl::GetCurrentDirectory(Void) {
	/* The physical address of the current page directory is (or at least should be) always be on the CR3 register. We can read it to get
	 * the current page directory... */
	
	UIntPtr ret;
	asm volatile("mov %%cr3, %0" : "=r"(ret));
	
	return ret;
}

Void ArchImpl::SwitchDirectory(UIntPtr Directory) {
	/* ... and write to it to set the current page directory. */
	
	if (Directory == 0) {
		return;
	}
	
	asm volatile("mov %0, %%cr3" :: "r"(Directory));
}

Void VmmInit(Void) {
	/* We need to prealloc the PD/PML4 entries for the kernel heap, let's use all the memory from the end of the kernel until the start
	 * of the temp entries for that, it should only use around 1MiB of physical memory for that, so we still should have plently of
	 * physical memory for us. */
	
	Status status;
#ifdef ARCH_64
	UIntPtr hend = 0xFFFFFF0000000000, esize = 0x8000000000, *ent = Null, lvl = 1,
#else
	UIntPtr hend = 0xFF800000, esize = 0x400000, *ent = Null, lvl = 1,
#endif
			hstart = (ArchImp.GetKernelEnd() + esize - 1) & -esize;
	
	for (UIntPtr i = hstart, p; i < hend; i += esize) {
		if ((status = CheckPageTables(i, ent, lvl)) != Status::NotMapped || lvl != 1) {
			continue;
		}
		
		if ((status = PhysMem::ReferenceSingle(0, p)) != Status::Success) {
			Debug->Write("PANIC! Failed to initialize the virtual memory manager\n");
			Arch->Halt();
		}
		
		*ent = (p & MM_PAGE_MASK) | 0x03;
		lvl++;
		
		UpdateTLB(ent);
		CheckPageTables(i, ent, lvl, True);
		
		lvl = 1;
	}
	
	Heap::Initialize(hstart, hend);
}
