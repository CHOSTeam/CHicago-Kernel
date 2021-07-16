/* File author is Ítalo Lima Marconato Matias
 *
 * Created on March 04 of 2021, at 17:19 BRT
 * Last edited on July 16 of 2021, at 10:31 BRT */

#pragma once

#include <base/status.hxx>
#include <base/traits.hxx>

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

#define VIRT_GROUP_PAGE_COUNT (PAGE_SIZE / sizeof(VirtMem::Page))
#define VIRT_GROUP_RANGE (PAGE_SIZE * VIRT_GROUP_PAGE_COUNT)

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
    struct packed Page {
        UIntPtr Count;
        UInt8 References;
        Page *NextSingle, *NextGroup, *LastSingle;
    };

    static Void Initialize(const BootInfo&);
#endif

    /* Each one of the functions (allocate/free/reference/dereference) needs three different versions of itself, one
     * for doing said operation on a single page, one for multiple, contiguous, pages, and one for multiple, but
     * non-contiguous, pages. */

    static Status Allocate(UIntPtr, UInt64&, UInt64 = PAGE_SIZE);
    static Status Allocate(UIntPtr, UInt64*, UInt64 = PAGE_SIZE);

    static Status Free(UInt64, UIntPtr = 1);
    static Status Free(UInt64*, UIntPtr = 1);

    static Status Reference(UInt64, UIntPtr, UInt64&, UInt64 = PAGE_SIZE);
    static Status Reference(UInt64*, UIntPtr, UInt64*, UInt64 = PAGE_SIZE);

    static Status Dereference(UInt64, UIntPtr = 1);
    static Status Dereference(UInt64*, UIntPtr = 1);

    /* Now we also need some helper functions for getting reference count of one page, to get the kernel (physical)
     * start/end, to get the amount of memory the system has, how much has been used, and how much is free. */

    static UIntPtr GetReferences(UInt64);
#ifdef KERNEL
    static Void FreeWaitingPages(Void);

    static inline UIntPtr GetKernelStart() { return KernelStart; }
    static inline UIntPtr GetKernelEnd() { return KernelEnd; }
    static inline UInt64 GetMinAddress() { return MinAddress; }
    static inline UInt64 GetMaxAddress() { return MaxAddress; }
    static inline UInt64 GetSize() { return MaxBytes; }
    static inline UInt64 GetUsage() { return UsedBytes; }
    static inline UInt64 GetFree() { return MaxBytes - UsedBytes; }
private:
    static UInt64 Reverse(Page *Node);

    static UInt64 MinAddress, MaxAddress, MaxBytes, UsedBytes;
    static UIntPtr PageCount, KernelStart, KernelEnd;
    static Page *Pages, *FreeList, *WaitingList;
    static Boolean Initialized;
#else
    static UInt64 GetSize();
    static UInt64 GetUsage();
    static UInt64 GetFree();
#endif
};

class VirtMem {
public:
#ifdef KERNEL
    struct packed Page {
        UIntPtr Count;
        Page *NextSingle, *NextGroup, *LastSingle;
    };

    static Void Initialize(const BootInfo&);
    static Void FreeWaitingPages(Void);

#endif
    static Status Query(UIntPtr, UInt64&, UInt32&);
    static Status Map(UIntPtr, UInt64, UIntPtr, UInt32);
    static Status Unmap(UIntPtr, UIntPtr, Boolean = False);

    static Status Allocate(UIntPtr, UIntPtr&, UIntPtr = PAGE_SIZE);
    static Status Free(UIntPtr, UIntPtr);

    static Status MapIo(UInt64, UIntPtr&, UIntPtr&);
#ifdef KERNEL
private:
    static Status ExpandPool(Void);
    static UIntPtr Reverse(Page*);

    static UIntPtr Start, End, Current, FreeCount;
    static Page *FreeList, *WaitingList;
#endif
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
#ifdef KERNEL
    struct Block {
        UIntPtr Magic, Size;
#ifndef _LP64
        UIntPtr Padding[2];
#endif
        union {
            struct { Block *Next, *Prev; } Free;
            UInt8 Data[0];
        };
    };

    static Void ReturnMemory();
#endif

    static Void *Allocate(UIntPtr);
    static Void *Allocate(UIntPtr, UIntPtr);
    static Void Free(Void*);

#ifdef KERNEL
private:
    static Block *Split(Block*, UIntPtr, Boolean = True);
    static Block *CreateBlock(UIntPtr);
    static Block *FindFree(UIntPtr);
    static Boolean AddFree(Block*);
    static Void RemoveFree(Block*);

    static Block *Head, *Tail;
#endif
};

#ifdef KERNEL
template<class T, class U> static inline UIntPtr FreeWaitingPages(T *&FreeList, T *&WaitingList, U &Reverse) {
    UIntPtr count = 0;

    while (WaitingList != Null) {
        /* Easiest case to handle is when this is a whole new entry (doesn't continue any other), as we just need
         * to add it to the right place. Other than that, we might need to increase the size of one region, and
         * merge multiple regions after increasing the size. */

        T *cur = FreeList, *prev = Null, *next = WaitingList->NextSingle;

        for (; cur != Null && Reverse(cur) < Reverse(WaitingList) &&
               Reverse(WaitingList) != Reverse(cur) + (cur->Count << PAGE_SHIFT) &&
               Reverse(WaitingList) + PAGE_SIZE != Reverse(cur); prev = cur, cur = cur->NextGroup) ;

        count++;
        WaitingList->LastSingle = WaitingList;

        if (cur == Null && prev == Null) {
            WaitingList->NextSingle = Null;
            FreeList = WaitingList;
        } else if (cur == Null) {
            WaitingList->NextSingle = Null;
            prev->NextGroup = WaitingList;
        } else if (Reverse(WaitingList) == Reverse(cur) + (cur->Count << PAGE_SHIFT)) {
            WaitingList->NextSingle = cur->LastSingle->NextSingle;
            WaitingList->LastSingle = Null;
            cur->LastSingle->NextSingle = WaitingList;
            cur->LastSingle = WaitingList;
            cur->Count++;

            while (cur->NextGroup != Null && Reverse(cur) + (cur->Count << PAGE_SHIFT) == Reverse(cur->NextGroup)) {
                T *next = cur->NextGroup;
                cur->Count += next->Count;
                cur->LastSingle->NextSingle = next;
                cur->LastSingle = next->LastSingle;
                cur->NextGroup = next->NextGroup;
                next->Count = 1;
                next->NextGroup = next->LastSingle = Null;
            }
        } else if (Reverse(WaitingList) + (WaitingList->Count << PAGE_SHIFT) == Reverse(cur)) {
            (prev != Null ? prev->NextGroup : FreeList) = WaitingList;
            WaitingList->Count += cur->Count;
            WaitingList->NextSingle = cur;
            WaitingList->NextGroup = cur->NextGroup;
            WaitingList->LastSingle = cur->LastSingle;
            cur->Count = 1;
            cur->NextGroup = cur->LastSingle = Null;
        } else {
            (prev != Null ? prev->NextGroup : FreeList) = WaitingList;
            WaitingList->NextSingle = WaitingList->NextGroup = cur;
        }

        WaitingList = next;
    }

    return count;
}

template<class T, class U, class V> static inline
Status AllocatePages(T *&FreeList, U &Reverse, UIntPtr Count, V &Out, V Align) {
    T *prev = Null;

    for (T *cur = FreeList; cur != Null; prev = cur, cur = cur->NextGroup) {
        if (cur->Count < Count) continue;
        else if (Reverse(cur) & (Align - 1)) {
            UIntPtr size = cur->Count - 1;
            T *prev2 = cur, *cur2 = cur->NextSingle;
            UInt64 end = Reverse(cur) + (cur->Count << PAGE_SHIFT);

            for (; cur2 != Null && Reverse(cur2) < end && size >= Count; prev2 = cur2, cur2 = cur2->NextSingle, size--)
                if (!(Reverse(cur2) & (Align - 1))) break;
            if (cur2 == Null || Reverse(cur2) >= end || size < Count) continue;

            /* Two things we might have to do here: Or we need to just decrease the cur->Count, or we need to split the
             * block in two (which later might be rejoined in the FreeWaitingList func). */

            Out = Reverse(cur2);

            for (UIntPtr i = 0; i < Count; i++) {
                T *next = cur2->NextSingle;
                cur2->Count = 0;
                if constexpr (IsSameV<T, PhysMem::Page>) cur2->References = 1;
                cur2->NextSingle = Null;
                cur2 = next;
            }

            if (Reverse(cur2) >= end) cur->Count -= Count;
            else {
                cur2->Count = size - Count;
                cur2->NextGroup = cur->NextGroup;
                cur->Count -= size;
                cur->NextGroup = cur2;
            }

            return prev2->NextSingle = cur2, Status::Success;
        }

        /* Taking the pages from the start is already enough (alignment is OK), we just need to move the group start
         * (or remove the group). */

        Out = Reverse(cur);

        UIntPtr nsize = cur->Count - Count;
        T *group = cur->NextGroup, *last = cur->LastSingle;

        while (Count--) {
            T *next = cur->NextSingle;
            cur->Count = 0;
            if constexpr (IsSameV<T, PhysMem::Page>) cur->References = 1;
            cur->NextSingle = cur->NextGroup = cur->LastSingle = Null;
            cur = next;
        }

        if (prev == Null) FreeList = cur;
        else prev->NextSingle = prev->NextGroup = cur;

        if (nsize) cur->NextGroup = group, cur->LastSingle = last, cur->Count = nsize;
        else if (prev != Null) prev->NextGroup = group;

        return Status::Success;
    }

    return Status::OutOfMemory;
}
#endif

}
