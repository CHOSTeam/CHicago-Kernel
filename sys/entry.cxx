/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 06 of 2021, at 12:22 BRT
 * Last edited on February 08 of 2021 at 11:09 BRT */

#include <arch.hxx>
#include <mm.hxx>
#include <textout.hxx>

TextConsole Debug;

extern "C" Void KernelEntry(BootInfo *Info) {
    /* Hello, World! The loader just exited the EFI environment, and gave control to the kernel! Right now the MMU
     * should be enabled, with everything we need (boot info struct, framebuffer, boot image file, etc) already mapped,
     * and we should be running on higher-half (probably). Before anything, let's call the _init function, this
     * function runs all the global constructors and things like that. */
	
	_init();

    /* Check if the bootloader passed a valid magic number (as we need a compatible boot info struct). */

    if (Info == Null || Info->Magic != BOOT_INFO_MAGIC) {
        Arch::Halt();
    }

    /* Initialize the debug interface (change this later to also possibly not use the screen). */

    Debug = TextConsole(*Info, 0, 0xFFFFFF00);
    Debug.SetForeground(0xFF00FF00);
    Debug.Write("initializing the kernel, arch = %s, version = %s\n", ARCH, VERSION);
    Debug.RestoreForeground();

    /* Initialize the physical memory manager. */

    PhysMem::Initialize(*Info);

    Debug.SetForeground(0xFF00FF00);
    Debug.Write("initialization finished, halting the machine");
    Debug.RestoreForeground();

    Arch::Halt();
}
