/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 06 of 2021, at 12:22 BRT
 * Last edited on February 06 of 2021 at 12:58 BRT */

#include <arch.hxx>
#include <boot.hxx>

extern "C" Void KernelEntry(BootInfo *Info) {
    /* Hello, World! The loader just exited the EFI environment, and gave control to the kernel! Right now the MMU
     * should be enabled, with everything we need (boot info struct, framebuffer, boot image file ,etc) already mapped,
     * and we should be running on higher-half (probably). Before anything, let's call the _init function, this
     * function runs all the global constructors and things like that. */
	
	_init();

    /* Clear the screen (to indicate that we got into the kernel) and halt. */

    for (UIntPtr i = 0; i < Info->FrameBuffer.Width * Info->FrameBuffer.Height; i++) {
        ((UInt32*)Info->FrameBuffer.Address)[i] = 0xFF00FF00;
    }

    Arch::Halt();
}
