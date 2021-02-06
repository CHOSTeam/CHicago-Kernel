/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 06 of 2021, at 12:22 BRT
 * Last edited on February 06 of 2021 at 17:22 BRT */

#include <arch.hxx>
#include <mm.hxx>

extern "C" Void KernelEntry(BootInfo *Info) {
    /* Hello, World! The loader just exited the EFI environment, and gave control to the kernel! Right now the MMU
     * should be enabled, with everything we need (boot info struct, framebuffer, boot image file ,etc) already mapped,
     * and we should be running on higher-half (probably). Before anything, let's call the _init function, this
     * function runs all the global constructors and things like that. */
	
	_init();

    /* Check if the bootloader passed a valid magic number (as we need a compatible boot info struct). */

    if (Info->Magic != BOOT_INFO_MAGIC) {
        Arch::Halt();
    }

    /* Initialize the memory manager (we just need to pass our Info struct, as the init functions will know what to do
     * with it). */

    PhysMem::Initialize(Info);

    /* Clear the screen (to indicate that everything went well) and halt. */

    for (UIntPtr i = 0; i < Info->FrameBuffer.Width * Info->FrameBuffer.Height; i++) {
        reinterpret_cast<UInt32*>(Info->FrameBuffer.Address)[i] = 0xFF00FF00;
    }

    Arch::Halt();
}
