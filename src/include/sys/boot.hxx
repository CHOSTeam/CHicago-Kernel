/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 05 of 2021, at 20:33 BRT
 * Last edited on July 10 of 2021 at 12:01 BRT */

#pragma once

#include <sys/acpi.hxx>

#define BOOT_INFO_MAGIC 0xC4057D41

namespace CHicago {

struct packed BootInfoSymbol {
    UIntPtr Start, End;
    Char *Name;
};

struct packed BootInfoMemMap {
    UInt64 Base, Count;
    UInt8 Type;
};

struct packed BootInfo {
    UInt32 Magic;
    UIntPtr KernelStart, PhysMgrStart, KernelEnd;
    UInt64 EfiTempAddress, MinPhysicalAddress, MaxPhysicalAddress, PhysicalMemorySize;
    Void *Directory;

    struct packed {
        Boolean Extended;
        UInt32 Size;
        UInt64 Sdt;
    } Acpi;

    struct packed {
        UIntPtr Count;
        BootInfoSymbol *Start;
    } Symbols;

    struct packed {
        UIntPtr Count;
        BootInfoMemMap Entries[256];
    } MemoryMap;

    struct packed {
        UIntPtr Size, Index;
        UInt8 *Data;
    } BootImage;

    struct packed {
        UIntPtr Width, Height, BackBuffer, FrontBuffer;
    } FrameBuffer;

    UInt8 KernelStack[0x2000];
};

#ifdef USE_INIT_ARRAY
extern "C" UIntPtr __init_array_start, __init_array_end;
#else
extern "C" Void _init();
#endif

}
