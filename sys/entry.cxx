/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 06 of 2021, at 12:22 BRT
 * Last edited on February 16 of 2021 at 19:55 BRT */

#include <arch.hxx>
#include <mm.hxx>
#include <panic.hxx>

using namespace CHicago;

Void Crash(Void) {
    /* CHicago is now built with SSP enabled, so doing this should crash this system: */

    UInt8 buf[8];
    SetMemory(buf, 0, 16);
}

extern "C" Void KernelEntry(BootInfo *Info) {
    /* Hello, World! The loader just exited the EFI environment, and gave control to the kernel! Right now the MMU
     * should be enabled, with everything we need (boot info struct, framebuffer, boot image file, etc) already mapped,
     * and we should be running on higher-half (probably). Before anything, let's call the _init function, this
     * function runs all the global constructors and things like that. */

    if (Info == Null || Info->Magic != BOOT_INFO_MAGIC) {
        Arch::Halt();
    }

#ifdef USE_INIT_ARRAY
    for (UIntPtr *i = &__init_array_start; i < &__init_array_end; i++) {
        reinterpret_cast<Void (*)(Void)>(*i)();
    }
#else
    _init();
#endif

    /* Initialize the debug interface (change this later to also possibly not use the screen). */

    Debug = TextConsole(*Info, 0, 0xFFFFFF00);
    Debug.SetForeground(0xFF00FF00);
    Debug.Write("initializing the kernel, arch = %s, version = %s\n", ARCH, VERSION);
    Debug.RestoreForeground();

    /* Initialize some important early system bits (backtrace symbol resolver, memory manager, etc). */

    StackTrace::Initialize(*Info);
    PhysMem::Initialize(*Info);
    VirtMem::Initialize(*Info);

    Debug.SetForeground(0xFF00FF00);
    Debug.Write("initialization finished, halting the machine\n");
    Debug.RestoreForeground();

    /* Test out the memory allocator by doing some allocations, and then call the crash function (that will test the
     * SSP). */

    Void *a1, *a2, *a3, *a4;

    Debug.Write("First allocation at " UINTPTR_MAX_HEX "\n", a1 = Heap::Allocate(8));
    Debug.Write("Second allocation at " UINTPTR_MAX_HEX "\n", a2 = Heap::Allocate(64));
    Debug.Write("Third allocation at " UINTPTR_MAX_HEX "\n", a3 = Heap::Allocate(8));
    Debug.Write("Fourth allocation at " UINTPTR_MAX_HEX "\n", a4 = Heap::Allocate(16));
    Heap::Deallocate(a2), Heap::Deallocate(a4);
    Debug.Write("Fifth allocation at " UINTPTR_MAX_HEX "\n", a2 = Heap::Allocate(16));
    Debug.Write("Sixth allocation at " UINTPTR_MAX_HEX "\n", a4 = Heap::Allocate(16));
    Heap::Deallocate(a1), Heap::Deallocate(a2), Heap::Deallocate(a3), Heap::Deallocate(a4);

    Crash();
    ASSERT(False);
}
