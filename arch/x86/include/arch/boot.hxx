/* File author is Ítalo Lima Marconato Matias
 *
 * Created on June 25 of 2020, at 07:52 BRT
 * Last edited on October 24 of 2020, at 17:21 BRT */

#ifndef __ARCH_BOOT_HXX__
#define __ARCH_BOOT_HXX__

#include <types.hxx>

/* This file contains the definition of the Boot Information struct, the struct is created
 * (and mapped) by the loader, it contains the system's memory map, initial page directory,
 * kernel and initrd (actually is the BOOT.SIA file) and some other important info. */
 
#define BOOT_INFO_MAGIC 0xC4057D41
#define BOOT_INFO_DEV_ATA 0x00
#define BOOT_INFO_DEV_SATA 0x01

struct packed BootInfoMemMap {
	UIntPtr Base, Size;
	Boolean Free;
};

struct packed BootInfo {
	UInt32 Magic;
	UIntPtr KernelStart;
	UInt8 *KernelStartVirt;
	UIntPtr KernelEnd;
	UInt8 *KernelEndVirt;
	UIntPtr RegionsStart;
	UInt8 *RegionsStartVirt;
	UIntPtr Directory;
	UIntPtr EfiMainAddress;
	UInt64 MaxPhysicalAddress, PhysicalMemorySize;
	
	struct packed {
		UIntPtr EntryCount;
		BootInfoMemMap *Entries;
	} MemoryMap;
	
	struct packed {
		UIntPtr Size;
		UInt8 *Contents;
	} BootSIA;
	
	struct packed {
		UIntPtr Width, Height;
		UIntPtr Size, Address;
	} FrameBuffer;
	
	struct packed {
		UInt8 Type;
		
		struct packed {
			UInt8 Bus;
			UInt8 Device;
			UInt8 Function;
		} PCI;
		
		union {
			struct packed {
				Boolean Primary;
				Boolean Master;
			} ATA;
			
			struct packed {
				UInt16 HBAPort;
				UInt16 Mult;
				UInt8 Lun;
			} SATA;
		};
	} BootDevice;
	
	UInt8 KernelStack[0x10000];
};

extern BootInfo *ArchBootInfo;

#endif
 