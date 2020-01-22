// File author is Ítalo Lima Marconato Matias
//
// Created on May 11 of 2018, at 13:14 BRT
// Last edited on January 20 of 2020, at 23:15 BRT

#include <chicago/arch.h>
#include <chicago/console.h>
#include <chicago/debug.h>
#include <chicago/device.h>
#include <chicago/display.h>
#include <chicago/ipc.h>
#include <chicago/panic.h>
#include <chicago/process.h>
#include <chicago/rand.h>
#include <chicago/shm.h>
#include <chicago/string.h>
#include <chicago/timer.h>
#include <chicago/version.h>
#include <chicago/virt.h>

Void KernelMain(Void) {
	ArchInitDebug();																															// Init the architecture-dependent debugging method
	DbgWriteFormated("[Kernel] Arch debugging initialized\r\n");
	
	DbgWriteFormated("[Kernel] CHicago %s (codename %s, for %s)\r\n", CHICAGO_VSTR_C, CHICAGO_CODENAME_C, CHICAGO_ARCH_C);						// Print the system version
	
	ArchInitFPU();																																// Init the architecture-dependent FPU (floating point unit)
	DbgWriteFormated("[Kernel] Arch FPU initialized\r\n");
	
	ArchInitPMM();																																// Init the physical memory manager
	DbgWriteFormated("[Kernel] Arch PMM initialized\r\n");
	
	ArchInitVMM();																																// Init the virtual memory manager
	DbgWriteFormated("[Kernel] Arch VMM initialized\r\n");
	
	ArchInitDisplay();																															// Init the display
	
	if ((ArchBootOptions & BOOT_OPTIONS_VERBOSE) == BOOT_OPTIONS_VERBOSE) {																		// Verbose boot?
		PImage con = ImgCreateBuf(DispGetWidth(), DispGetHeight() - 40, DispBackBuffer->bpp, DispBackBuffer->buf, False);						// Yes, try to create the console surface

		if (con != Null) {																														// Failed?
			ConSetSurface(con, True, True, 0, 0);																								// No, set the console surface
			DbgSetRedirect(True);																												// Enable the redirect feature of the Dbg* functions
			ConSetColor(0xFF0D3D56, 0xFFFFFFFF);																								// Set the background and foreground colors
			ConSetCursorEnabled(False);																											// Disable the cursor
			ConClearScreen();																													// Clear the screen
			ConWriteFormated(L"[Kernel] CHicago %s (codename %s, for %s): verbose boot\r\n", CHICAGO_VSTR, CHICAGO_CODENAME, CHICAGO_ARCH);		// And print the kernel version, codename, arch, and say that we are in verbose boot mode
		}
	}
	
	DbgWriteFormated("[Kernel] Arch display initialized\r\n");
	
	DispDrawProgessBar();																														// Draw the progress bar
	DbgWriteFormated("[Kernel] The boot progress bar has been shown\r\n");
	
	ArchInitSc();																																// Init the system call interface
	DbgWriteFormated("[Kernel] Arch system call interface initialized\r\n");
	
	ArchInit();																																	// Let's finish it by initalizating the other architecture-dependent bits of the kernel
	DispIncrementProgessBar();
	DbgWriteFormated("[Kernel] Arch initialized\r\n");
	
	FsInitDevices();																															// Now init the basic devices
	DispIncrementProgessBar();
	DbgWriteFormated("[Kernel] Devices initialized\r\n");
	
	FsInit();																																	// Init the filesystem list, mount point list, and add the basic mount points
	DispIncrementProgessBar();
	DbgWriteFormated("[Kernel] Filesystem initialized\r\n");
	
	PsInit();																																	// Init tasking
	ArchHalt();																																	// Halt
}

Void KernelMainLate(Void) {
	DispIncrementProgessBar();																													// Alright, tasking is now initialized!
	PsInitIdleProcess();																														// Create and add the idle process
	PsInitKillerThread();																														// Create and add the killer thread
	RandSetSeed(RandGenerateSeed());																											// Set the random generator seed
	DbgWriteFormated("[Kernel] Tasking initialized\r\n");
	
	DispFillProgressBar();																														// Ok, the kernel boot process is now finished!
	DbgWriteFormated("[Kernel] Kernel initialized\r\n\r\n");
	
	if ((ArchBootOptions & BOOT_OPTIONS_VERBOSE) == BOOT_OPTIONS_VERBOSE) {																		// In verbose boot?
		for (UIntPtr i = 0; i < 3; i++) {																										// Yes, let's just wait a bit before continuing...
			DbgWriteFormated(".");
			PsSleep(500);
		}
		
		DbgWriteFormated("\r\n");
		DbgSetRedirect(False);																													// Disable the redirect feature of the Dbg* functions
	}
	
	ConSetSurface(DispBackBuffer, True, False, 0, 0);																							// Init the console in fullscreen mode
	ConClearScreen();
	
	PsSwitchTask(PsDontRequeue);																												// And now, remove us from the queue
}
