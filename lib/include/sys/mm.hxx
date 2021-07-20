/* File author is Ítalo Lima Marconato Matias
 *
 * Created on March 04 of 2021, at 17:19 BRT
 * Last edited on July 19 of 2021, at 23:21 BRT */

#pragma once

#include <base/status.hxx>
#include <base/traits.hxx>

#ifdef KERNEL
#include <sys/arch.hxx>
#include <util/lock.hxx>
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
        volatile UInt8 References;
        Page *NextSingle, *NextGroup, *LastSingle;
    };
private:
    static UInt64 Reverse(Page*);
public:
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
    static inline UIntPtr GetKernelStart(Void) { return KernelStart; }
    static inline UIntPtr GetKernelEnd(Void) { return KernelEnd; }
    static inline UInt64 GetMinAddress(Void) { return MinAddress; }
    static inline UInt64 GetMaxAddress(Void) { return MaxAddress; }
    static inline UInt64 GetSize(Void) { return MaxBytes; }
    static inline UInt64 GetUsage(Void) { return UsedBytes; }
    static inline UInt64 GetFree(Void) { return MaxBytes - UsedBytes; }
private:
    static UInt64 MinAddress, MaxAddress, MaxBytes, UsedBytes;
    static UIntPtr PageCount, KernelStart, KernelEnd;
    static Page *Pages, *FreeList;
    static Boolean Initialized;
    static SpinLock Lock;
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
    static Page *FreeList;
    static SpinLock Lock;
#endif
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
            struct { Block *Next, *Prev; };
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
    static SpinLock Lock;
#endif
};

#ifdef KERNEL
template<class T, class U, class V> static inline
Status AllocatePages(T *&FreeList, U &Reverse, UIntPtr Count, V &Out, V Align) {
    T *prev = Null;

    for (T *cur = FreeList; cur != Null; prev = cur, cur = cur->NextGroup) {
        V addr = Reverse(cur), skip = (Align - (addr & (Align - 1))) >> PAGE_SHIFT;

        if (cur->Count < Count || ((addr & (Align - 1)) && cur->Count < Count + skip)) continue;
        else if (addr & (Align - 1)) {
            T *prev2 = cur, *cur2 = cur->NextSingle;

            for (UIntPtr i = 1; i < skip; i++) prev2 = cur2, cur2 = cur2->NextSingle;

            /* Two things we might have to do here: Or we need to just decrease the cur->Count, or we need to split the
             * block in two (which later might be rejoined in the FreePages func). */

            Out = Reverse(cur2);

            for (UIntPtr i = 0; i < Count; i++) {
                T *next = cur2->NextSingle;
                cur2->Count = 0;
                if constexpr (IsSameV<T, PhysMem::Page>) cur2->References = 1;
                cur2->NextSingle = Null;
                cur2 = next;
            }

            if (Reverse(cur2) >= addr + (cur->Count << PAGE_SHIFT)) cur->Count -= Count;
            else {
                cur2->Count = cur->Count - Count - skip;
                cur2->NextGroup = cur->NextGroup;
                cur->Count = skip;
                cur->NextGroup = cur2;
            }

            return prev2->NextSingle = cur2, Status::Success;
        }

        /* Taking the pages from the start is already enough (alignment is OK), we just need to move the group start
         * (or remove the group). */

        Out = addr;

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
        else prev->LastSingle->NextSingle = prev->NextGroup = cur;

        if (nsize) cur->NextGroup = group, cur->LastSingle = last, cur->Count = nsize;
        else if (prev != Null) prev->NextGroup = group;

        return Status::Success;
    }

    return Status::OutOfMemory;
}

template<class T, class U, class V, class W>
static Void FreePages(T *&FreeList, U Reverse, V GetEntry, W Start, UIntPtr Count) {
    if (!Count) return;

    UIntPtr i = 1;
    T *group = Null, *ent = GetEntry(Start), *cur = FreeList, *prev = Null;

    /* For the first entry (when group is Null) we need to find its right position (so that we can easily add the
     * others by using the ->LastSingle field). */

    for (; cur != Null && Reverse(ent) > Reverse(cur) && Reverse(ent) != Reverse(cur) + (cur->Count << PAGE_SHIFT);
           cur = cur->NextGroup) ;

    if (cur == Null || Reverse(ent) < Reverse(cur)) {
        if (prev == Null) FreeList = ent;
        else prev->LastSingle->NextSingle = prev->NextGroup = ent;

        ent->NextSingle = ent->NextGroup = cur;
        ent->LastSingle = ent;
        ent->Count = 1;

        group = ent;
    } else {
        ent->NextSingle = cur->LastSingle->NextSingle;
        ent->NextGroup = ent->LastSingle = Null;
        ent->Count = 1;

        cur->LastSingle->NextSingle = ent;
        cur->LastSingle = cur;
        cur->Count += 1;

        group = cur;
    }

    /* And now we can go through the remaining entries and just append them to the end of the group (no need to search
     * for the right place or anything, as all pages from GetEntry should be contiguous). */

    while (ent = GetEntry(Start + (i << PAGE_SHIFT)), i++ < Count) {
        ent->NextSingle = group->LastSingle->NextSingle;
        ent->NextGroup = ent->LastSingle = Null;
        ent->Count = 1;

        group->LastSingle->NextSingle = ent;
        group->LastSingle = ent;
        group->Count += 1;
    }

    /* Finally, everything that remains is merging the group with its neighbours (if possible). */

    while (True) {
        Boolean fuse = False;

        if (group->NextGroup != Null && Reverse(group->NextGroup) == Reverse(group) + (group->Count << PAGE_SHIFT)) {
            T *next = group->NextGroup;

            group->Count += next->Count;
            group->NextGroup = next->NextGroup;
            group->LastSingle = next->LastSingle;

            next->Count = 1;
            next->NextGroup = next->LastSingle = Null;

            fuse = True;
        }

        if (!fuse) break;
    }
}
#endif

}
