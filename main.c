// File author is Ítalo Lima Marconato Matias
//
// Created on May 11 of 2018, at 13:14 BRT
// Last edited on November 03 of 2019, at 12:50 BRT

#include <chicago/arch.h>
#include <chicago/console.h>
#include <chicago/debug.h>
#include <chicago/device.h>
#include <chicago/display.h>
#include <chicago/exec.h>
#include <chicago/nls.h>
#include <chicago/panic.h>
#include <chicago/process.h>
#include <chicago/version.h>

Void KernelMain(Void) {
	ArchInitDebug();																													// Init the architecture-dependent debugging method
	DbgWriteFormated("[Kernel] Arch debugging initialized\r\n");
	
	DbgWriteFormated("[Kernel] CHicago %s (codename %s, for %s)\r\n", CHICAGO_VSTR_C, CHICAGO_CODENAME_C, CHICAGO_ARCH_C);				// Print the system version
	
	ArchInitFPU();																														// Init the architecture-dependent FPU (floating point unit)
	DbgWriteFormated("[Kernel] Arch FPU initialized\r\n");
	
	ArchInitPMM();																														// Init the physical memory manager
	DbgWriteFormated("[Kernel] Arch PMM initialized\r\n");
	
	ArchInitVMM();																														// Init the virtual memory manager
	DbgWriteFormated("[Kernel] Arch VMM initialized\r\n");
	
	ArchInitDisplay();																													// Init the display
	
	if ((ArchBootOptions & BOOT_OPTIONS_VERBOSE) == BOOT_OPTIONS_VERBOSE) {																// Verbose boot?
		PImage con = ImgCreateBuf(DispGetWidth(), DispGetHeight() - 40, DispBackBuffer->bpp, DispBackBuffer->buf, False, False);		// Yes, try to create the console surface

		if (con != Null) {																												// Failed?
			ConSetSurface(con, True, True, 0, 0);																						// No, set the console surface
			DbgSetRedirect(True);																										// Enable the redirect feature of the Dbg* functions
			ConSetColor(0xFF0D3D56, 0xFFFFFFFF);																						// Set the background and foreground colors
			ConSetCursorEnabled(False);																									// Disable the cursor
			ConClearScreen();																											// Clear the screen
		}
	}
	
	DbgWriteFormated("[Kernel] Arch display initialized\r\n");
	
	DispDrawProgessBar();																												// Draw the progress bar
	DbgWriteFormated("[Kernel] The boot progress bar has been shown\r\n");
	
	ArchInitSc();																														// Init the system call interface
	DbgWriteFormated("[Kernel] Arch system call interface initialized\r\n");
	
	ArchInit();																															// Let's finish it by initalizating the other architecture-dependent bits of the kernel
	DispIncrementProgessBar();
	DbgWriteFormated("[Kernel] Arch initialized\r\n");
	
	FsInitDevices();																													// Now init the basic devices
	DispIncrementProgessBar();
	DbgWriteFormated("[Kernel] Devices initialized\r\n");
	
	FsInit();																															// Init the filesystem list, mount point list, and add the basic mount points
	DispIncrementProgessBar();
	DbgWriteFormated("[Kernel] Filesystem initialized\r\n");
	
	PsInit();																															// Init tasking
	ArchHalt();																															// Halt
}

Void KernelMainLate(Void) {
	DispIncrementProgessBar();																											// Alright, tasking is now initialized!
	PsInitKillerThread();																												// Create and add the killer thread
	DbgWriteFormated("[Kernel] Tasking initialized\r\n");
	
	DispFillProgressBar();																												// Ok, the kernel boot process is now finished!
	DbgWriteFormated("[Kernel] Kernel initialized\r\n\r\n");
	
	DbgSetRedirect(False);																												// Disable the redirect feature of the Dbg* functions, as it may be enabled
	ConSetSurface(DispBackBuffer, True, False, 0, 0);																					// Init the console in fullscreen mode
	ConClearScreen();																													// Clear the screen
	ConWriteFormated(NlsGetMessage(NLS_OS_NAME), CHICAGO_ARCH);																			// Print some system informations
	ConWriteFormated(NlsGetMessage(NLS_OS_CODENAME), CHICAGO_CODENAME);
	ConWriteFormated(NlsGetMessage(NLS_OS_VSTR), CHICAGO_MAJOR, CHICAGO_MINOR, CHICAGO_BUILD);
	
	PProcess proc = ExecCreateProcess(L"/System/Programs/sesmgr.che");																	// Let's create the session manager process
	
	if (proc == Null) {
		DbgWriteFormated("PANIC! Couldn't start the /System/Programs/sesmgr.che program\r\n");											// Failed, so let's panic
		Panic(PANIC_KERNEL_UNEXPECTED_ERROR);
	}
	
	PsAddProcess(proc);																													// Add the session manager process
	PsWaitProcess(proc->id);																											// And it is never supposed to exit, so this should halt us
	DbgWriteFormated("PANIC! The \\System\\Programs\\sesmgr.che program closed\r\n");													// ... Panic
	Panic(PANIC_KERNEL_UNEXPECTED_ERROR);
	ArchHalt();
}
