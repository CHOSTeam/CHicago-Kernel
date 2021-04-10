/* File author is Ítalo Lima Marconato Matias
 *
 * Created on March 04 of 2021, at 17:19 BRT
 * Last edited on March 10 of 2021 at 16:55 BRT */

#pragma once

#include <base/status.hxx>

#ifdef KERNEL
#include <sys/arch.hxx>
#endif

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

#ifdef _LP64
#define ALLOC_BLOCK_MAGIC 0xBEEFD337CE8DB73F
#else
#define ALLOC_BLOCK_MAGIC 0xCE8DB73F
#endif

namespace CHicago {

class PhysMem {
public:
#ifdef KERNEL
    struct packed Region {
        UIntPtr Free, Used, Pages[PHYS_REGION_BITMAP_LEN];
    };

    static Void Initialize(BootInfo&);
#endif

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
#ifdef KERNEL
    static inline UIntPtr GetKernelStart() { return KernelStart; }
    static inline UIntPtr GetKernelEnd() { return KernelEnd; }
    static inline UIntPtr GetMinAddress() { return MinAddress; }
    static inline UIntPtr GetMaxAddress() { return MaxAddress; }
    static inline UIntPtr GetSize() { return MaxBytes; }
    static inline UIntPtr GetUsage() { return UsedBytes; }
    static inline UIntPtr GetFree() { return MaxBytes - UsedBytes; }
private:
    static Status FindFreePages(UIntPtr, UIntPtr, UIntPtr&, UIntPtr&);
    static Status AllocInt(UIntPtr, UIntPtr&, UIntPtr);
    static Status FreeInt(UIntPtr, UIntPtr);

    static UIntPtr KernelStart, KernelEnd, RegionCount, MinAddress, MaxAddress, MaxBytes, UsedBytes;
    static Region *Regions;
    static UInt8 *References;
    static Boolean Initialized;
#else
    static UIntPtr GetSize();
    static UIntPtr GetUsage();
    static UIntPtr GetFree();
#endif
};

class VirtMem {
public:
#ifdef KERNEL
    static Void Initialize(BootInfo&);
#endif

    static Status Query(UIntPtr, UIntPtr&, UInt32&);
    static Status Map(UIntPtr, UIntPtr, UIntPtr, UInt32);
    static Status Unmap(UIntPtr, UIntPtr, Boolean = False);
};

struct AllocBlock {
    UIntPtr Magic;
    UIntPtr Start, Size;
    UInt64 Pad;
#ifndef _LP64
    UInt32 Pad2;
#endif
    AllocBlock *Next, *Prev;
};

class Heap {
public:
    /* The VirtMem::Initialize (that may be arch-specific, instead of our generic one) will call the init function, and
     * pre-map the entries that will be used (on the top level, so that we don't accidentally waste too much
     * memory). */

#ifdef KERNEL
    static Void Initialize(UIntPtr, UIntPtr);
    static Void ReturnPhysical();
#endif

    static Status Increment(UIntPtr);
    static Void Decrement(UIntPtr);

    static Void *Allocate(UIntPtr);
    static Void *Allocate(UIntPtr, UIntPtr);
    static Void Deallocate(Void*);

#ifdef KERNEL
    static inline Void *GetStart() { return reinterpret_cast<Void*>(Start); }
    static inline Void *GetEnd() { return reinterpret_cast<Void*>(End); }
    static inline Void *GetCurrent() { return reinterpret_cast<Void*>(Current); }
    static inline UIntPtr GetSize() { return End - Start; }
    static inline UIntPtr GetUsage() { return CurrentAligned - Start; }
    static inline UIntPtr GetFree() { return End - CurrentAligned; }
private:
    static Void AddFree(AllocBlock*);
    static Void RemoveFree(AllocBlock*);
    static Void SplitBlock(AllocBlock*, UIntPtr);
    static AllocBlock *FuseBlock(AllocBlock*);
    static AllocBlock *FindBlock(UIntPtr);
    static AllocBlock *CreateBlock(UIntPtr);

    static Boolean Initialized;
    static AllocBlock *Base, *Tail;
    static UIntPtr Start, End, Current, CurrentAligned;
#else
    static Void *GetStart();
    static Void *GetEnd();
    static Void *GetCurrent();
    static UIntPtr GetSize();
    static UIntPtr GetUsage();
    static UIntPtr GetFree();
#endif
};

}
