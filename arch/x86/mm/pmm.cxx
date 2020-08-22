/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 03 of 2020, at 17:26 BRT
 * Last edited on July 05 of 2020, at 23:21 BRT */

#include <chicago/arch/arch.hxx>
#include <chicago/arch/boot.hxx>
#include <chicago/mm.hxx>
#include <chicago/textout.hxx>

Void PmmInit(Void) {
	/* Call ::Initialize, it's responsible for initializing all the structs/pointers, and set the InInit variable,
	 * that allow us to call ::InitializeRegion. */
	
	PhysMem::Initialize(ArchBootInfo->RegionsStartVirt, ArchBootInfo->KernelStart, ArchBootInfo->KernelEnd,
						ArchBootInfo->MaxPhysicalAddress, ArchBootInfo->PhysicalMemorySize);
	
	/* Let's InitializeRegion all of the free physical memory regions (that contains usable memory), making sure to
	 * NOT InitializeRegion the kernel pages, and in the end, call ::FinishInitialization. */
	
	for (UIntPtr i = 0; i < ArchBootInfo->MemoryMap.EntryCount; i++) {
		BootInfoMemMap *mmap = &ArchBootInfo->MemoryMap.Entries[i];
		
		if (i > 0 && mmap->Base == 0) {
			break;
		} else if (mmap->Free) {
			for (UIntPtr i = 0; i < mmap->Size; i += MM_PAGE_SIZE) {
				UIntPtr addr = mmap->Base + i;
				
				if (!(addr == 0 || (addr >= PhysMem::GetKernelStart() && addr < PhysMem::GetKernelEnd()))) {
					PhysMem::InitializeRegion(addr);
				}
			}
		}
	}
	
	/* The virtual memory manager (that is, the heap) needs to know where the kernel ends, let's also save where
	 * it starts. */
	
	ArchImp.SetKernelBoundaries((UIntPtr)ArchBootInfo->KernelStartVirt, (UIntPtr)ArchBootInfo->KernelEndVirt);
	PhysMem::FinishInitialization();
}