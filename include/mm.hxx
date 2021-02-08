/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 01 of 2020, at 16:07 BRT
 * Last edited on February 08 of 2021, at 13:47 BRT */

#pragma once

#include <boot.hxx>
#include <status.hxx>

#ifndef MM_PAGE_SIZE
#define MM_PAGE_SIZE 0x1000
#ifdef _LP64
#define MM_HUGE_PAGE_SIZE 0x200000
#else
#define MM_HUGE_PAGE_SIZE 0x400000
#endif
#endif

#define MM_PAGE_MASK (MM_PAGE_SIZE - 1)
#define MM_HUGE_PAGE_MASK (MM_HUGE_PAGE_SIZE - 1)

#ifndef MM_PAGE_SHIFT
#define MM_PAGE_SHIFT 12
#define MM_REGION_SHIFT 22
#ifdef _LP64
#define MM_REGION_PAGE_SHIFT 18
#else
#define MM_REGION_PAGE_SHIFT 17
#endif
#endif

#define MM_REGION_BITMAP_LEN ((1 << MM_REGION_SHIFT) / (1 << MM_REGION_PAGE_SHIFT))
#define MM_REGION_BITMAP_PSIZE (sizeof(UIntPtr) * 8)
#define MM_REGION_BITMAP_BSIZE (MM_PAGE_SIZE * MM_REGION_BITMAP_PSIZE)
#define MM_REGION_BITMAP_MASK (MM_REGION_BITMAP_BSIZE - 1)

#define MM_REGION_PSIZE (MM_REGION_BITMAP_LEN * MM_REGION_BITMAP_PSIZE)
#define MM_REGION_BSIZE (MM_PAGE_SIZE * MM_REGION_PSIZE)
#define MM_REGION_MASK (MM_REGION_BSIZE - 1)

class PhysMem {
public:
	struct Region {
		UIntPtr Free;
		UIntPtr Used;
		UIntPtr Pages[MM_REGION_BITMAP_LEN];
	};
	
	static Void Initialize(BootInfo&);
	
	/* Each one of the functions (allocate/free/reference/dereference) needs three different versions of itself, one
	 * for doing said operation on a single page, one for multiple, contiguous, pages, and one for multiple, but
	 * non-contiguous, pages. */
	
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
	
	/* Now we also need some helper functions for getting reference count of one page, to get the kernel (physical)
	 * start/end, to get the amount of memory the system has, how much has been used, and how much is free. */
	
	static UInt8 GetReferences(UIntPtr);
	static UIntPtr GetKernelStart(Void) { return KernelStart; }
	static UIntPtr GetKernelEnd(Void) { return KernelEnd; }
	static UIntPtr GetMinAddress(Void) { return MinAddress; }
	static UIntPtr GetMaxAddress(Void) { return MaxAddress; }
	static UIntPtr GetSize(Void) { return MaxBytes; }
	static UIntPtr GetUsage(Void) { return UsedBytes; }
	static UIntPtr GetFree(Void) { return MaxBytes - UsedBytes; }
private:
	static UIntPtr CountFreePages(UIntPtr, UIntPtr, UIntPtr);
	static Status FindFreePages(UIntPtr, UIntPtr, UIntPtr&, UIntPtr&);
	static Status AllocInt(UIntPtr, UIntPtr&);
	static Status FreeInt(UIntPtr, UIntPtr);
	
	static UIntPtr KernelStart, KernelEnd, RegionCount, MinAddress, MaxAddress, MaxBytes, UsedBytes;
	static Region *Regions;
	static UInt8 *References;
	static Boolean Initialized;
};
