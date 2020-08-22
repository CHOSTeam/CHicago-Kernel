/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 01 of 2020, at 19:47 BRT
 * Last edited on August 22 of 2020, at 20:01 BRT */

#include <chicago/mm.hxx>
#include <chicago/string.hxx>
#include <chicago/textout.hxx>

/* This is probably the only code (non-header) file with some type of macro definition on it lol. */

#ifdef ARCH_64
#define GET_FIRST_UNSET_BIT(bm) __builtin_ctzll(~(bm))
#else
#define GET_FIRST_UNSET_BIT(bm) __builtin_ctz(~(bm))
#endif

/* All of the static private variables. */

UIntPtr PhysMem::KernelStart = 0, PhysMem::KernelEnd = 0, PhysMem::RegionCount = 0,
		PhysMem::MaxAddress = 0, PhysMem::MaxBytes = 0, PhysMem::UsedBytes = 0;
PhysMem::Region *PhysMem::Regions = Null;
UInt8 *PhysMem::References = Null;
Boolean PhysMem::InInit = False;

Void PhysMem::Initialize(UInt8 *Start, UIntPtr KernelStart, UIntPtr KernelEnd, UIntPtr MaxAddress, UIntPtr MaxBytes) {
	/* This function should only be called once by the arch-specific kernel entry, it is responsible for initializing
	 * the physical memory manager (allocate/deallocate physical memory, and manage how many references the pages have
	 * around all the tasks/CPUs/whatever). */
	
	if (InInit) {
		return;
	}
	
	/* First, setup some of the basic fields, including the InInit field, that indicates that the we were called,
	 * but the FinishInitialization functions isn't haven't been called (that is, we can call InitializeRegion). */
	
	RegionCount = MaxAddress / MM_PAGE_SIZE / 1024;
	PhysMem::KernelStart = KernelStart;
	PhysMem::KernelEnd = KernelEnd;
	PhysMem::MaxAddress = MaxAddress;
	PhysMem::MaxBytes = MaxBytes;
	UsedBytes = MaxBytes;
	InInit = True;
	
	/* Now, we need to initialize the region list, and the reference list, we need to do this before setting the
	 * kernel end, as we're going to put those lists after the current kernel end.
	 * There is one very important thing, we expect the arch-entry to already have mapped those lists onto virtual
	 * memory, on x86(-32/-64), the bootloader does it for us, on other architectures, it's probably going to be a
	 * bit different, but it needs to be already mapped. */
	
	Regions = (PhysMem::Region*)Start;
	References = &Start[RegionCount * sizeof(PhysMem::Region)];
	
	/* Now, let's clear the region list and the reference list (set all the entries on the region list to be already
	 * used, and all the entries on the references list to have 0 references). */
	
	for (UIntPtr i = 0; i < RegionCount; i++) {
		Regions[i].Free = 0;
		Regions[i].Used = (1 << MM_REGION_SIZE_SHIFT) / (1 << MM_REGION_PAGE_SIZE_SHIFT);
		StrSetMemory(Regions[i].Pages, 0xFF, sizeof(Regions[i].Pages));
	}
	
	StrSetMemory(References, 0, MaxAddress / MM_PAGE_SIZE);
	
	/* Everything that is left now is initializing all the regions, but were not going to do that, the caller should
	 * do it, as it should have some type of memory map, for determining the free regions. */
}

Void PhysMem::InitializeRegion(UIntPtr Address) {
	/* This function can only be called when we're initializing the physical memory manager, calling at any other
	 * time should just return without doing anything. */
	
	if (!InInit) {
		return;
	} else if (UsedBytes < MM_PAGE_SIZE) {
		return;
	}
	
	/* First, save some indexes that make our job a lot easier. */
	
	UIntPtr i = Address >> MM_REGION_SIZE_SHIFT,
			j = (Address - (i << MM_REGION_SIZE_SHIFT)) >> MM_REGION_PAGE_SIZE_SHIFT,
			k = (Address - (i << MM_REGION_SIZE_SHIFT) - (j << MM_REGION_PAGE_SIZE_SHIFT)) >> MM_PAGE_SIZE_SHIFT;
	
	/* Now, free the page and decrease the used bytes count. */
	
	Regions[i].Pages[j] &= ~((UIntPtr)1 << k);
	Regions[i].Free++;
	Regions[i].Used--;
	UsedBytes -= MM_PAGE_SIZE;
}

Void PhysMem::FinishInitialization(Void) {
	/* Just as in the other function, we should only be called when we're initializing the PMM (actually, when we
	 * just finished initializing it). */
	
	if (InInit) {
		InInit = False;
	}
}

/* Most of the alloc/free functions only redirect to the AllocInt function, the exception for this is the NonContig
 * functions, as the allocated memory pages DON'T need to be contiguous, so we can alloc one-by-one (which have a
 * lower chance of failing, as in the contig functions, the pages have to be on the same region). */

Status PhysMem::AllocSingle(UIntPtr &Out) {
	return AllocInt(1, Out);
}

Status PhysMem::AllocContig(UIntPtr Count, UIntPtr &Out) {
	return AllocInt(Count, Out);
}

Status PhysMem::AllocNonContig(UIntPtr Count, UIntPtr *Out) {
	/* We need to manually check the two paramenter here, as we're going to alloc page-by-page, and we're going to
	 * return multiple addresses, instead of returning only a single one that points to the start of a bunch of
	 * consecutive pages. */
	
	if (Count == 0 || Out == Null) {
		return Status::InvalidArg;
	}
	
	UIntPtr addr;
	Status status;
	
	for (UIntPtr i = 0; i < Count; i++) {	
		if ((status = AllocSingle(addr)) != Status::Success) {
			FreeNonContig(Out, i);
			return status;
		}
		
		Out[i] = addr;
	}
	
	return Status::Success;
}

Status PhysMem::FreeSingle(UIntPtr Page) {
	return FreeInt(Page, 1);
}

Status PhysMem::FreeContig(UIntPtr Start, UIntPtr Count) {
	return FreeInt(Start, Count);
}

Status PhysMem::FreeNonContig(UIntPtr *Pages, UIntPtr Count) {
	/* This is the same as AllocNonContig, but instead of calling AllocSingle, we call FreeSingle, and we
	 * don't need to save anything after doing that. */
	
	if (Count == 0 || Pages == Null) {
		return Status::InvalidArg;
	}
	
	Status status;
	
	for (UIntPtr i = 0; i < Count; i++) {
		if ((status = FreeSingle(Pages[i])) != Status::Success) {
			return status;
		}
	}
	
	return Status::Success;
}

Status PhysMem::ReferenceSingle(UIntPtr Page, UIntPtr &Out) {
	/* For referencing single pages, we can just check if we need to alloc a new page (if we do we self
	 * call us with said new allocated page), and increase the reference counter for the page. */
	
	if (Page < MM_PAGE_SIZE) {
		Status status = AllocSingle(Page);
		
		if (status != Status::Success) {
			return status;
		}
		
		return ReferenceSingle(Page, Out);
	} else if (References == Null || UsedBytes < MM_PAGE_SIZE || (Page & MM_PAGE_MASK) >= MaxAddress) {
		return Status::InvalidArg;
	}
	
	References[(Page & MM_PAGE_MASK) >> MM_PAGE_SIZE_SHIFT]++;
	Out = Page & MM_PAGE_MASK;
	
	return Status::Success;
}

Status PhysMem::ReferenceContig(UIntPtr Start, UIntPtr Count, UIntPtr &Out) {
	/* Referencing multiple contig pages is also pretty simple, same checks as the other function, but
	 * we need to reference all the pages in a loop. */
	
	Status status;
	
	if (Start < MM_PAGE_SIZE) {
		if ((status = AllocContig(Count, Start)) != Status::Success) {
			return status;
		}
		
		return ReferenceContig(Start, Count, Out);
	} else if (References == Null || UsedBytes < Count * MM_PAGE_SIZE || Count == 0 ||
			   (Start & MM_PAGE_MASK) + Count * MM_PAGE_SIZE >= MaxAddress) {
		return Status::InvalidArg;
	}
	
	/* For sake of convinience, let's align the start address to the page boundary. */
	
	Start &= MM_PAGE_MASK;
	
	for (UIntPtr i = 0, discard = 0; i < Count; i++) {
		if ((status = ReferenceSingle(Start + i * MM_PAGE_SIZE, discard)) != Status::Success) {
			DereferenceContig(Start, Count);
			return status;
		}
	}
	
	Out = Start;
	
	return Status::Success;
}

Status PhysMem::ReferenceNonContig(UIntPtr *Pages, UIntPtr Count, UIntPtr *Out) {
	/* For non-contig pages, we just call ReferenceSingle on each of the pages. */
	
	if (References == Null || UsedBytes < Count * MM_PAGE_SIZE || Count == 0 ||
		Pages == Null || Out == Null) {
		return Status::InvalidArg;
	}
	
	Status status;
	
	for (UIntPtr i = 0; i < Count; i++) {
		if ((status = ReferenceSingle(Pages[i], Out[i])) != Status::Success) {
			DereferenceNonContig(Pages, i);
			return status;
		}
	}
	
	return Status::Success;
}

Status PhysMem::DereferenceSingle(UIntPtr Page) {
	/* Dereferencing is pretty easy, make sure that we referenced the address before, if we have, decrease the
	 * ref count, and if we were the last ref to the address, dealloc it.
	 * Let's align the address to the page boundary to make our job easier. */
	
	Page &= MM_PAGE_MASK;
	
	if (References == Null || UsedBytes < MM_PAGE_SIZE || Page == 0 || Page >= MaxAddress ||
		References[Page >> MM_PAGE_SIZE_SHIFT] == 0) {
		return Status::InvalidArg;
	}
	
	if (!(References[Page >> MM_PAGE_SIZE_SHIFT]--)) {
		return FreeSingle(Page);
	}
	
	return Status::Success;
}

/* Dereferencing contig pages and non-contig pages is pretty much the same process, but on contig pages, we can
 * just add 'i * PAGE_SIZE' to the start address to get the page, on non-contig, we just need to access Pages[i]
 * to get the page. */

Status PhysMem::DereferenceContig(UIntPtr Start, UIntPtr Count) {
	Start &= MM_PAGE_MASK;
	
	if (References == Null || UsedBytes < Count * MM_PAGE_SIZE || Start == 0 ||
		Count == 0 || Start + Count * MM_PAGE_SIZE >= MaxAddress) {
		return Status::InvalidArg;
	}
	
	Status status;
	
	for (UIntPtr i = 0; i < Count; i++) {
		if ((status = DereferenceSingle(Start + i * MM_PAGE_SIZE)) != Status::Success) {
			return status;
		}
	}
	
	return Status::Success;
}

Status PhysMem::DereferenceNonContig(UIntPtr *Pages, UIntPtr Count) {
	if (References == Null || UsedBytes < Count * MM_PAGE_SIZE || Count == 0 || Pages == Null) {
		return Status::InvalidArg;
	}
	
	Status status;
	
	for (UIntPtr i = 0; i < Count; i++) {
		if ((status = DereferenceSingle(Pages[i])) != Status::Success) {
			return status;
		}
	}
	
	return Status::Success;
}

UInt8 PhysMem::GetReferences(UIntPtr Page) {
	if (References == Null || Page < MM_PAGE_SIZE || Page >= MaxAddress) {
		return 0;
	}
	
	return References[(Page & MM_PAGE_MASK) >> MM_PAGE_SIZE_SHIFT];
}

UIntPtr PhysMem::CountFreePages(UIntPtr BitMap, UIntPtr Start, UIntPtr End) {
	/* As each bit of the bitmap is one physical memory page, to count the amount of free pages we just need to count
	 * how many (consecutive) bits are NOT set. */
	
	UIntPtr ret = 0;
	
	for (; Start + ret <= End && !(BitMap & (1 << (Start + ret))); ret++) ;
	
	return ret;
}

Status PhysMem::FindFreePages(UIntPtr BitMap, UIntPtr Count, UIntPtr &Out) {
	/* Each unset bit is one free page, for finding consective free pages, we can iterate through the bits of the
	 * bitmap and search for free bits. */
	
	UIntPtr bc = sizeof(UIntPtr) * 8;
	
	for (UIntPtr i = 0; i < bc;) {
		if (BitMap == UINTPTR_MAX || bc - i < Count) {
			/* If all the remaining bits are set, or if there aren't enough bits left, there is really nothing we
			 * can do, we're out of memory (in this bitmap). */
			
			return Status::OutOfMemory;
		} else if (BitMap == 0) {
			/* If all the remaining bytes are free, we can just return the current position, as we already checked
			 * if we have enough bits! */
			
			Out = i;
			
			return Status::Success;
		}
		
		/* Well, we need to check how many unset bits we have, first, get the location of the first unset bit here,
		 * after that, count how many avaliable bits we have. */
		
		UIntPtr bit = GET_FIRST_UNSET_BIT(BitMap);
		UIntPtr aval = CountFreePages(BitMap, bit, bc - i);
		
		if (aval >= Count) {
			Out = i + bit;
			return Status::Success;
		}
		
		BitMap >>= bit + aval;
		i += bit + aval;
	}
	
	return Status::OutOfMemory;
}

Status PhysMem::AllocInt(UIntPtr Count, UIntPtr &Out) {
	/* We need to check if all of the arguments are valid, and if the physical memory manager have already been
	 * initialized. */
	
	if (Count == 0) {
		return Status::InvalidArg;
	} else if (Regions == Null || UsedBytes + Count * MM_PAGE_SIZE >= MaxBytes) {
		return Status::OutOfMemory;
	}
	
	/* Now, we can just go through each region, and each bitmap in each region, and try to find the consecutive
	 * free pages that we were asked for. */
	
	for (UIntPtr i = 0; i < RegionCount; i++) {
		if (Regions[i].Free < Count) {
			/* This variable exists so we don't need to check a region that WILL NOT have enough bits in any of 
			 * the bitmaps. */
			
			continue;
		}
		
		for (UIntPtr j = 0; j < (1 << MM_REGION_SIZE_SHIFT) /
								(1 << MM_REGION_PAGE_SIZE_SHIFT); j++) {
			UIntPtr bit;
			
			if (FindFreePages(Regions[i].Pages[j], Count, bit) == Status::Success) {
				/* Oh, we actually found enough free pages! we need to set all the bits that we're going to use,
				 * increase the used bytes variable, and calculate the return value. */
				
				for (UIntPtr k = bit; k < bit + Count; k++) {
					Regions[i].Pages[j] |= (UIntPtr)1 << k;
				}
				
				Regions[i].Free -= Count;
				Regions[i].Used += Count;
				UsedBytes += Count * MM_PAGE_SIZE;
				Out = (i * (1 << MM_REGION_SIZE_SHIFT)) +
					  (j * (1 << MM_REGION_PAGE_SIZE_SHIFT)) +
					  (bit * MM_PAGE_SIZE);
				
				return Status::Success;
			}
		}
	}
	
	return Status::OutOfMemory;
}

Status PhysMem::FreeInt(UIntPtr Start, UIntPtr Count) {
	/* Do the same checks that we did in the AllocInt function, and align the start address to a page boundary. */
	
	if (Start < MM_PAGE_SIZE || Count == 0 ||
		(Start & MM_PAGE_MASK) + Count * MM_PAGE_SIZE >= MaxAddress || UsedBytes < Count * MM_PAGE_SIZE) {
		return Status::InvalidArg;
	}
	
	Start &= MM_PAGE_MASK;
	
	/* Just as we did on the InitializeRegion function, save some indexes that will make our job a bit easier.
	 * After that, we can check if the region even has enough bytes used, and then, finally, unset the bits on
	 * the region's bitmap and update the amount of free/used pages. */
	
	UIntPtr i = Start >> MM_REGION_SIZE_SHIFT,
			j = (Start - (i << MM_REGION_SIZE_SHIFT)) >> MM_REGION_PAGE_SIZE_SHIFT,
			k = (Start - (i << MM_REGION_SIZE_SHIFT) - (j << MM_REGION_PAGE_SIZE_SHIFT)) >> MM_PAGE_SIZE_SHIFT;
	
	if (Regions[i].Used < Count) {
		return Status::InvalidArg;
	}
	
	for (UIntPtr b = k; b < k + Count; b++) {
		Regions[i].Pages[j] &= ~((UIntPtr)1 << b);
	}
	
	Regions[i].Free += Count;
	Regions[i].Used -= Count;
	UsedBytes -= Count * MM_PAGE_SIZE;
	
	return Status::Success;
}
