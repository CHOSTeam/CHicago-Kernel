/* File author is Ítalo Lima Marconato Matias
 *
 * Created on July 01 of 2020, at 16:07 BRT
 * Last edited on February 16 of 2021, at 10:23 BRT */

#pragma once

#include <boot.hxx>
#include <status.hxx>

#ifndef PAGE_SIZE
#define PAGE_SHIFT 12
#define PAGE_SIZE 0x1000
#ifdef _LP64
#define HUGE_PAGE_SHIFT 21
#define HUGE_PAGE_SIZE 0x200000
#else
#define HUGE_PAGE_SHIFT 22
#define HUGE_PAGE_SIZE 0x400000
#endif
#endif

#define PAGE_MASK (PAGE_SIZE - 1)
#define HUGE_PAGE_MASK (HUGE_PAGE_SIZE - 1)

#define PHYS_REGION_SHIFT 22
#ifdef _LP64
#define PHYS_REGION_PAGE_SHIFT 18
#else
#define PHYS_REGION_PAGE_SHIFT 17
#endif

#define PHYS_REGION_BITMAP_LEN ((1 << PHYS_REGION_SHIFT) / (1 << PHYS_REGION_PAGE_SHIFT))
#define PHYS_REGION_BITMAP_PSIZE (sizeof(UIntPtr) * 8)
#define PHYS_REGION_BITMAP_BSIZE (PAGE_SIZE * PHYS_REGION_BITMAP_PSIZE)
#define PHYS_REGION_BITMAP_MASK (PHYS_REGION_BITMAP_BSIZE - 1)

#define PHYS_REGION_PSIZE (PHYS_REGION_BITMAP_LEN * PHYS_REGION_BITMAP_PSIZE)
#define PHYS_REGION_BSIZE (PAGE_SIZE * PHYS_REGION_PSIZE)
#define PHYS_REGION_MASK (PHYS_REGION_BSIZE - 1)

#define MAP_USER 0x01
#define MAP_KERNEL 0x02
#define MAP_READ 0x04
#define MAP_WRITE 0x08
#define MAP_EXEC 0x10
#define MAP_HUGE 0x20
#define MAP_AOR 0x40
#define MAP_COW 0x80
#define MAP_RX (MAP_READ | MAP_EXEC)
#define MAP_RW (MAP_READ | MAP_WRITE)

namespace CHicago {

class PhysMem {
public:
    struct packed Region {
        UIntPtr Free, Used, Pages[PHYS_REGION_BITMAP_LEN];
    };

    static Void Initialize(BootInfo&);

	/* Each one of the functions (allocate/free/reference/dereference) needs three different versions of itself, one
	 * for doing said operation on a single page, one for multiple, contiguous, pages, and one for multiple, but
	 * non-contiguous, pages. */

    static Status AllocSingle(UIntPtr&, UIntPtr = PAGE_SIZE);
    static Status AllocContig(UIntPtr, UIntPtr&, UIntPtr = PAGE_SIZE);
    static Status AllocNonContig(UIntPtr, UIntPtr*, UIntPtr = PAGE_SIZE);

    static Status FreeSingle(UIntPtr);
    static Status FreeContig(UIntPtr, UIntPtr);
    static Status FreeNonContig(UIntPtr*, UIntPtr);

    static Status ReferenceSingle(UIntPtr, UIntPtr&, UIntPtr = PAGE_SIZE);
    static Status ReferenceContig(UIntPtr, UIntPtr, UIntPtr&, UIntPtr = PAGE_SIZE);
    static Status ReferenceNonContig(UIntPtr*, UIntPtr, UIntPtr*, UIntPtr = PAGE_SIZE);

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
    static Status AllocInt(UIntPtr, UIntPtr&, UIntPtr);
    static Status FreeInt(UIntPtr, UIntPtr);

    static UIntPtr KernelStart, KernelEnd, RegionCount, MinAddress, MaxAddress, MaxBytes, UsedBytes;
    static Region *Regions;
    static UInt8 *References;
    static Boolean Initialized;
};

class VirtMem {
public:
    static Void Initialize(BootInfo&);

    static Status Query(UIntPtr, UIntPtr&, UInt32&);
    static Status Map(UIntPtr, UIntPtr, UIntPtr, UInt32);
    static Status Unmap(UIntPtr, UIntPtr, Boolean = False);
};

class Heap {
public:
    /* The VirtMem::Initialize (that may be arch-specific, instead of our generic one) will call the init function, and
     * pre-map the entries that will be used (on the top level, so that we don't accidentally waste too much
     * memory). */

    static Void Initialize(UIntPtr, UIntPtr);

    static Status Increment(UIntPtr);
    static Status Decrement(UIntPtr);

    static Void *GetStart(Void) { return reinterpret_cast<Void*>(Start); }
    static Void *GetEnd(Void) { return reinterpret_cast<Void*>(End); }
    static Void *GetCurrent(Void) { return reinterpret_cast<Void*>(Current); }
    static UIntPtr GetSize(Void) { return End - Start; }
    static UIntPtr GetUsage(Void) { return CurrentAligned - Start; }
    static UIntPtr GetFree(Void) { return End - CurrentAligned; }
private:
    static Boolean Initialized;
    static UIntPtr Start, End, Current, CurrentAligned;
};

}
