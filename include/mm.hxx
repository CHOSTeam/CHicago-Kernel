/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 01 of 2020, at 16:07 BRT
 * Last edited on November 01 of 2020, at 12:35 BRT */

#ifndef __MM_HXX__
#define __MM_HXX__

#include <lock.hxx>

#ifndef MM_PAGE_SIZE
#define MM_PAGE_SIZE 0x1000
#ifdef ARCH_64
#define MM_HUGE_PAGE_SIZE 0x200000
#else
#define MM_HUGE_PAGE_SIZE 0x400000
#endif
#endif

#ifndef MM_PAGE_MASK
#define MM_PAGE_MASK (-MM_PAGE_SIZE)
#define MM_HUGE_PAGE_MASK (-MM_PAGE_SIZE)
#endif

#ifndef MM_PAGE_SIZE_SHIFT
#define MM_PAGE_SIZE_SHIFT 12
#define MM_REGION_SIZE_SHIFT 22
#ifdef ARCH_64
#define MM_REGION_PAGE_SIZE_SHIFT 18
#else
#define MM_REGION_PAGE_SIZE_SHIFT 17
#endif
#endif

/* Maybe we could move those defines into an enum class? Idk, implementing all the operators to
 * still use it normally seems like a bit of trouble lol. */

#define MM_MAP_USER 0x01
#define MM_MAP_KERNEL 0x02
#define MM_MAP_READ 0x04
#define MM_MAP_WRITE 0x08
#define MM_MAP_EXEC 0x10
#define MM_MAP_PROT (MM_MAP_USER | MM_MAP_KERNEL | MM_MAP_READ | MM_MAP_WRITE | MM_MAP_EXEC)
#define MM_MAP_AOR 0x20
#define MM_MAP_COW 0x40
#define MM_MAP_HUGE 0x80
#define MM_MAP_KDEF (MM_MAP_KERNEL | MM_MAP_READ | MM_MAP_WRITE)
#define MM_MAP_UDEF (MM_MAP_USER | MM_MAP_READ | MM_MAP_WRITE)

#define MM_PF_READWRITE_ON_NON_PRESENT 0x00
#define MM_PF_READWRITE_ON_KERNEL_ONLY 0x01
#define MM_PF_WRITE_ON_READONLY 0x02

/* Initially, I though about only using the magic number when compiled with DBG enabled, but
 * it's probably a good idea to always check if the block is actually valid. */

#define ALLOC_BLOCK_MAGIC 0xDEADB33F

struct AllocBlock {
	UInt32 Magic;
	Boolean Free;
	UIntPtr Size;
	UIntPtr Start;
	AllocBlock *Next;
	AllocBlock *Prev;
};

class PhysMem {
public:
	struct Region {
		UIntPtr Free;
		UIntPtr Used;
		UIntPtr Pages[(1 << MM_REGION_SIZE_SHIFT) /
					  (1 << MM_REGION_PAGE_SIZE_SHIFT)];
	};
	
	/* We expect the kernel (arch-specific) entry point to initialize us by calling Initialize and
	 * InitializeRegion, the entry point should determine the start of the regions pointer and of the
	 * references pointer. */
	
	static Void Initialize(UInt8*, UIntPtr, UIntPtr, UInt64, UInt64);
	static Void InitializeRegion(UIntPtr);
	static Void FinishInitialization(Void);
	
	/* Each one of the functions (allocate/free/reference/dereference) needs three different versions
	 * of itself, one for doing said operation on a single page, one for multiple, contiguous, pages, and
	 * one for multiple, but non-contiguous, pages. */
	
	static Status AllocSingle(UIntPtr&);
	static Status AllocContig(UIntPtr, UIntPtr&);
	static Status AllocNonContig(UIntPtr, UIntPtr*);
	
	static Status FreeSingle(UIntPtr);
	static Status FreeContig(UIntPtr, UIntPtr);
	static Status FreeNonContig(UIntPtr*, UIntPtr);
	
	static Status ReferenceSingle(UIntPtr, UIntPtr&);
	static Status ReferenceContig(UIntPtr, UIntPtr, UIntPtr&);
	static Status ReferenceNonContig(UIntPtr*, UIntPtr, UIntPtr*);
	
	static Status DereferenceSingle(UIntPtr);
	static Status DereferenceContig(UIntPtr, UIntPtr);
	static Status DereferenceNonContig(UIntPtr*, UIntPtr);
	
	/* Now we also need some helper functions for getting reference count of one page, to get the kernel
	 * (physical) start/end, to get the amount of memory the system has, how much has been used, and how
	 * much is free. */
	
	static UInt8 GetReferences(UIntPtr);
	static UIntPtr GetKernelStart(Void) { return KernelStart; }
	static UIntPtr GetKernelEnd(Void) { return KernelEnd; }
	static UInt64 GetMaxAddress(Void) { return MaxAddress; }
	static UInt64 GetSize(Void) { return MaxBytes; }
	static UInt64 GetUsage(Void) { return UsedBytes; }
	static UInt64 GetFree(Void) { return MaxBytes - UsedBytes; }
private:
	static UIntPtr CountFreePages(UIntPtr, UIntPtr, UIntPtr);
	static Status FindFreePages(UIntPtr, UIntPtr, UIntPtr&);
	static Status AllocInt(UIntPtr, UIntPtr&);
	static Status FreeInt(UIntPtr, UIntPtr);
	
	static UIntPtr KernelStart, KernelEnd, RegionCount;
	static UInt64 MaxAddress, MaxBytes, UsedBytes;
	static Region *Regions;
	static UInt8 *References;
	static Boolean InInit;
	static SpinLock Lock;
};

/* What would our kernel be without some type of memory allocator? Our memory allocator is pretty simple,
 * and probably pretty unoptimized and slow, but hey, at least it works. */

class Heap {
public:
	/* The arch-specific entry point should call the init function manually, and probably pre-map the pages
	 * that are going to be used (or something like that, on x86, for example, we just alloc the phys addr
	 * of the PD/PML4 for the kernel heap). */
	
	static Void Initialize(UIntPtr, UIntPtr);
	
	static Status Increment(UIntPtr);
	static Status Decrement(UIntPtr);
	
	/* I think that the functions that this is a good place to put the functions that manage the actual
	 * memory allocations on the heap, instead of using a separate class/file. */
	
	static Void *Allocate(UIntPtr);
	static Void *Allocate(UIntPtr, UIntPtr);
	static Void Deallocate(Void*);
	
	static Void *GetStart(Void) { return (Void*)Start; }
	static Void *GetEnd(Void) { return (Void*)End; }
	static Void *GetCurrent(Void) { return (Void*)Current; }
	static UInt64 GetSize(Void) { return End - Start; }
	static UInt64 GetUsage(Void) { return CurrentAligned - Start; }
	static UInt64 GetFree(Void) { return End - CurrentAligned; }
private:
	static Void SplitBlock(AllocBlock*, UIntPtr);
	static AllocBlock *FuseBlock(AllocBlock*);
	static AllocBlock *FindBlock(AllocBlock*&, UIntPtr);
	static AllocBlock *CreateBlock(AllocBlock*, UIntPtr);
	
	static UIntPtr Start, End, Current, CurrentAligned;
	static SpinLock HeapLock, AllocLock;
	static AllocBlock *AllocBase;
};

#endif
