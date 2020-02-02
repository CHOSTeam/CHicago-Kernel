// File author is Ítalo Lima Marconato Matias
//
// Created on June 28 of 2018, at 18:48 BRT
// Last edited on February 02 of 2020, at 15:37 BRT

#ifndef __CHICAGO_MM_H__
#define __CHICAGO_MM_H__

#include <chicago/process.h>

#ifndef MM_PAGE_SIZE
#define MM_PAGE_SIZE 0x1000
#endif

#ifndef MM_PAGE_MASK
#if __INTPTR_WIDTH__ == 64
#define MM_PAGE_MASK 0xFFFFFFFFFF000
#else
#define MM_PAGE_MASK 0xFFFFF000
#endif
#endif

#ifndef MM_PAGE_SIZE_SHIFT
#define MM_PAGE_SIZE_SHIFT 12
#endif

#ifndef MM_USER_START
#define MM_USER_START 0
#endif

#ifndef MM_USER_END
#if __INTPTR_WIDTH__ == 64
#define MM_USER_END 0x800000000000
#else
#define MM_USER_END 0xC0000000
#endif
#endif

#define MM_MAP_USER 0x01
#define MM_MAP_KERNEL 0x02
#define MM_MAP_READ 0x04
#define MM_MAP_WRITE 0x08
#define MM_MAP_EXEC 0x10
#define MM_MAP_PROT (MM_MAP_USER | MM_MAP_KERNEL | MM_MAP_READ | MM_MAP_WRITE | MM_MAP_EXEC)
#define MM_MAP_AOR 0x20
#define MM_MAP_COW 0x40
#define MM_MAP_KDEF (MM_MAP_KERNEL | MM_MAP_READ | MM_MAP_WRITE)
#define MM_MAP_UDEF (MM_MAP_USER | MM_MAP_READ | MM_MAP_WRITE)

#define MM_PF_READWRITE_ON_NON_PRESENT 0x00
#define MM_PF_READWRITE_ON_KERNEL_ONLY 0x01
#define MM_PF_WRITE_ON_READONLY 0x02

#define MM_HINT_READ_AHEAD 0x00
#define MM_HINT_FREE 0x01

#define MM_FLAGS_READ 0x01
#define MM_FLAGS_WRITE 0x02
#define MM_FLAGS_EXEC 0x04
#define MM_FLAGS_PROT (MM_FLAGS_READ | MM_FLAGS_WRITE | MM_FLAGS_EXEC)
#define MM_FLAGS_HIGHEST 0x08
#define MM_FLAGS_ALLOC_NOW 0x10
#define MM_FLAGS_PRIVATE 0x20

#define MmKernelDirectory (MmGetPhys(((UIntPtr)(&MmKernelDirectoryInt))))

typedef struct {
	UIntPtr free;
	UIntPtr pages[(1024 * MM_PAGE_SIZE) / (8 * MM_PAGE_SIZE * sizeof(UIntPtr))];
} MmPageRegion, *PMmPageRegion;

typedef struct {
	UIntPtr start;
	UIntPtr size;
} MmKey, *PMmKey;

typedef struct MmRegionStruct {
	MmKey key;
	PWChar name;
	PFsNode file;
	UIntPtr off;
	UInt32 prot;
	UInt32 flgs;
	PThread th;
} MmRegion, *PMmRegion;

typedef struct MmVirtualRangeStruct {
	UIntPtr start;
	UIntPtr size;
	struct MmVirtualRangeStruct *next;
	struct MmVirtualRangeStruct *prev;
} MmVirtualRange, *PMmVirtualRange;

typedef struct {
	PFsNode file;
	List mappings;
} MmMappedFile, *PMmMappedFile;

extern UIntPtr MmKernelDirectoryInt;

UIntPtr MmBootAlloc(UIntPtr size, Boolean align);

Status MmAllocSinglePage(PUIntPtr ret);
Status MmAllocContigPages(UIntPtr count, PUIntPtr ret);
Status MmAllocNonContigPages(UIntPtr count, PUIntPtr ret);
Status MmFreeSinglePage(UIntPtr addr);
Status MmFreeContigPages(UIntPtr addr, UIntPtr count);
Status MmFreeNonContigPages(PUIntPtr pages, UIntPtr count);
Status MmReferenceSinglePage(UIntPtr addr, PUIntPtr ret);
Status MmReferenceContigPages(UIntPtr addr, UIntPtr count, PUIntPtr ret);
Status MmReferenceNonContigPages(PUIntPtr pages, UIntPtr count, PUIntPtr ret);
Status MmDereferenceSinglePage(UIntPtr addr);
Status MmDereferenceContigPages(UIntPtr addr, UIntPtr count);
Status MmDereferenceNonContigPages(PUIntPtr pages, UIntPtr count);
UIntPtr MmGetReferences(UIntPtr addr);
UIntPtr MmGetSize(Void);
UIntPtr MmGetUsage(Void);
UIntPtr MmGetFree(Void);

UIntPtr MmGetPhys(UIntPtr virt);
UInt32 MmQuery(UIntPtr virt);
UIntPtr MmFindFreeVirt(UIntPtr start, UIntPtr end, UIntPtr count);
UIntPtr MmFindHighestFreeVirt(UIntPtr start, UIntPtr end, UIntPtr count);
UIntPtr MmMapTemp(UIntPtr phys, UInt32 flags);
Status MmMap(UIntPtr virt, UIntPtr phys, UInt32 flags);
Status MmUnmap(UIntPtr virt);
Void MmPrepareMapFile(UIntPtr start, UIntPtr end);
Void MmPrepareUnmapFile(UIntPtr start, UIntPtr end);
UIntPtr MmCreateDirectory(Void);
Void MmFreeDirectory(UIntPtr dir);
UIntPtr MmGetCurrentDirectory(Void);
Void MmSwitchDirectory(UIntPtr dir);

Status MmPageFaultDoAOR(UIntPtr addr, UInt32 prot);
Status MmPageFaultDoCOW(UIntPtr addr, UInt32 prot);
Status MmPageFaultDoMapFile(PFsNode file, UIntPtr addr, UIntPtr off, UInt32 prot);
Status MmPageFaultHandler(PMmRegion region, PVoid arch, UIntPtr addr, UInt32 flags, UInt8 reason);
PAvlNode MmGetMapping(UIntPtr addr, UIntPtr size, Boolean exact);
Status MmMapMemory(PWChar name, UIntPtr virt, PUIntPtr phys, UIntPtr size, UInt32 flags);
Status MmAllocAddress(PWChar name, UIntPtr size, UInt32 flags, PUIntPtr ret);
Status MmUnmapMemory(UIntPtr start);
Status MmFreeAddress(UIntPtr start);
Status MmSyncMemory(UIntPtr start, UIntPtr size, Boolean inval);
Status MmGiveHint(UIntPtr start, UIntPtr size, UInt8 hint);
Status MmChangeProtection(UIntPtr start, UInt32 prot);
Void MmInitMappingTree(PProcess proc);
Status MmInitVirtualAddressAllocTree(PProcess proc);

#endif		// __CHICAGO_MM_H__
