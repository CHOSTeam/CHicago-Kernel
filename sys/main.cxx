/* File author is Ítalo Lima Marconato Matias
 *
 * Created on June 25 of 2020, at 09:49 BRT
 * Last edited on August 12 of 2020, at 18:18 BRT */

#include <chicago/arch.hxx>
#include <chicago/display.hxx>
#include <chicago/img.hxx>
#include <chicago/siafs.hxx>
#include <chicago/textout.hxx>

/* Some global variables that don't fit anywhere else. */

IArch *Arch = Null;

Void KernelMain(Void) {
	/* Register the SIA filesystem interface. It allow us to ::Mount and easily manipulate SIA files.
	 * It's the only FS that comes bundled with the kernel, as all the other supported FSes should be loaded from the initrd (which we expect
	 * to be already mounted at /System/Boot). */
	
	if (SiaFs::Register() != Status::Success) {
		Debug->Write("PANIC! Couldn't register the SIA filesystem interface\n");
		Arch->Halt();
	}
	
	/* TEMP: Initialize the console (black background, white foreground, cursor disabled) and print some system info. */
	
	ConsoleImpl::InitConsoleInterface(0xFF000000, 0xFFFFFFFF, False);
	
#ifdef DBG
	Console->Write("This was compiled under the '[Version: %s] [Arch: %s] [Debug: yes]' environment\n", VERSION, ARCH);
#else
	Console->Write("This was compiled under the '[Version: %s] [Arch: %s] [Debug: no]' environment\n", VERSION, ARCH);
#endif
	Console->Write("Usable physical memory size size is %BB\n", PhysMem::GetSize());
	Console->Write("%BB of physical memory are being used, and %BB are free\n", PhysMem::GetUsage(), PhysMem::GetFree());
	Console->Write("The display resolution is %dx%d, and the font size is %dx%d\n", Display::GetWidth(), Display::GetHeight(),
				   DefaultFontData.Width, DefaultFontData.Height);
	
	Arch->Halt();
}
