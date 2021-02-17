/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 06 of 2021, at 12:22 BRT
 * Last edited on February 17 of 2021 at 11:47 BRT */

#include <arch.hxx>
#include <mm.hxx>
#include <panic.hxx>

using namespace CHicago;

Void Crash(Void) {
    /* On x86/amd64 we can use ud2 and test the exception handler, if we can't do that, just crash everything using
     * ubsan. */

#if defined(__i386__) || defined(__x86_64__)
    asm volatile("ud2");
#endif

    *((volatile UIntPtr*)nullptr) = 0;
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

    /* Initialize the arch-specific bits. */

    Arch::Initialize(*Info);

    Debug.SetForeground(0xFF00FF00);
    Debug.Write("initialization finished, halting the machine\n");
    Debug.RestoreForeground();

    Crash();
    ASSERT(False);
}
