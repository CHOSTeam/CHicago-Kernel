/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 05 of 2021, at 20:33 BRT
 * Last edited on February 05 of 2021, at 20:37 BRT */

#pragma once

#include <types.hxx>

#define BOOT_INFO_MAGIC 0xC4057D41

#define BOOT_INFO_MEM_KCODE 0x00
#define BOOT_INFO_MEM_KDATA 0x01
#define BOOT_INFO_MEM_MMU 0x02
#define BOOT_INFO_MEM_DEV 0x03
#define BOOT_INFO_MEM_RES 0x04
#define BOOT_INFO_MEM_FREE 0x05

struct packed BootInfoMemMap {
    UIntPtr Base, Size;
    UInt8 Type;
};

struct packed BootInfo {
    UInt32 Magic;
    UIntPtr KernelStart, RegionsStart, KernelEnd, EfiTempAddress;
    UInt64 MinPhysicalAddress, MaxPhysicalAddress, PhysicalMemorySize;
    Void *Directory;

    struct packed {
        UIntPtr Count;
        BootInfoMemMap *Entries;
    } MemoryMap;

    struct packed {
        UIntPtr Size, Index;
        UInt8 *Data;
    } BootImage;

    struct packed {
        UIntPtr Width, Height, Size, Address;
    } FrameBuffer;

    UInt8 KernelStack[0x10000];
};
