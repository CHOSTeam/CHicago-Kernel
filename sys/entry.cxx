/* File author is Ítalo Lima Marconato Matias
 *
 * Created on February 06 of 2021, at 12:22 BRT
 * Last edited on February 07 of 2021 at 22:18 BRT */

#include <arch.hxx>
#include <mm.hxx>
#include <img.hxx>

extern "C" Void KernelEntry(BootInfo *Info) {
    /* Hello, World! The loader just exited the EFI environment, and gave control to the kernel! Right now the MMU
     * should be enabled, with everything we need (boot info struct, framebuffer, boot image file ,etc) already mapped,
     * and we should be running on higher-half (probably). Before anything, let's call the _init function, this
     * function runs all the global constructors and things like that. */
	
	_init();

    /* Check if the bootloader passed a valid magic number (as we need a compatible boot info struct). */

    if (Info == Null || Info->Magic != BOOT_INFO_MAGIC) {
        Arch::Halt();
    }

    /* Initialize a test Image (with our screen as the buffer), and print something to the screen (we expect the screen
     * to be already cleared/black). */

    Image img(reinterpret_cast<UInt32*>(Info->FrameBuffer.Address), Info->FrameBuffer.Width, Info->FrameBuffer.Height);
    img.DrawString(0, 0, 0xFFFFFFFF, "Hello, World!\nDrawString uses VariadicFormat to write this! (0x%016X)\n",
                   0xDEADBEEFull);

    Arch::Halt();
}
