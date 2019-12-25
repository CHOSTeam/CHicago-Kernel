// File author is Ítalo Lima Marconato Matias
//
// Created on May 11 of 2018, at 13:14 BRT
// Last edited on December 25 of 2019, at 17:30 BRT

#include <chicago/alloc.h>
#include <chicago/arch.h>
#include <chicago/config.h>
#include <chicago/console.h>
#include <chicago/debug.h>
#include <chicago/device.h>
#include <chicago/display.h>
#include <chicago/drv.h>
#include <chicago/exec.h>
#include <chicago/ipc.h>
#include <chicago/nls.h>
#include <chicago/panic.h>
#include <chicago/process.h>
#include <chicago/rand.h>
#include <chicago/shm.h>
#include <chicago/string.h>
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
	RandSetSeed(RandGenerateSeed());																									// Set the random generator seed
	DbgWriteFormated("[Kernel] Tasking initialized\r\n");
	
	IpcInit();																															// Init the IPC (message passing) interface
	ShmInit();																															// And the SHM (shared memory) interface
	DispIncrementProgessBar();
	DbgWriteFormated("[Kernel] IPC initialized\r\n");
	
	PList conf = ConfLoad(L"System.conf");																								// Load the configuration file, let's set the system language!
	
	if (conf != Null) {																													// Failed?
		PConfField lang = ConfGetField(conf, L"Language");																				// No! Let's get the system language
		
		if (lang != Null) {
			NlsSetLanguage(NlsGetLanguage(lang->value));
		}
		
		ConfFree(conf);																													// Now free the loaded conf file
	}
	
	if (DrvLoadLibCHKrnl() == Null) {																									// Load the kernel symbols into a handle, so the drivers can use it
		DbgWriteFormated("PANIC! Couldn't create the handle for libchkrnl.elf\r\n");
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	conf = ConfLoad(L"Drivers.conf");																									// Now, let's load the driver configuration file (contain all the driver that we need to load)

	if (conf != Null) {																													// Failed?
		ListForeach(conf, i) {																											// No! Let's load all the drivers!
			PConfField drv = (PConfField)i->data;
			Boolean success = DrvLoad(drv->value) != Null;																				// Load the driver
			PChar name = (PChar)MemAllocate(StrGetLength(drv->name) + 1);																// Alloc space for converting the name to ASCII (for the Dbg* functions)

			if (name == Null) {
				continue;
			}

			PChar path = (PChar)MemAllocate(StrGetLength(drv->value) + 1);																// Alloc space for converting the path to ASCII (for the Dbg* functions)

			if (path == Null) {
				MemFree((UIntPtr)name);
				continue;
			}

			StrCFromUnicode(name, drv->name, StrGetLength(drv->name));																	// Convert the name
			StrCFromUnicode(path, drv->value, StrGetLength(drv->value));																// And the path!
			DbgWriteFormated("[Kernel] %s driver '%s' (%s)\r\n", success ? "Loaded the" : "Couldn't load the", name, path);				// Print some info about the driver (name and path) and if the driver loaded with success
			MemFree((UIntPtr)name);																										// Free the name
			MemFree((UIntPtr)path);																										// Free the path
		}

		ConfFree(conf);																													// Now free the loaded conf file
	}
	
	DispFillProgressBar();																												// Ok, the kernel boot process is now finished!
	DbgWriteFormated("[Kernel] Kernel initialized\r\n\r\n");
	
	if ((ArchBootOptions & BOOT_OPTIONS_VERBOSE) == BOOT_OPTIONS_VERBOSE) {																// In verbose boot?
		for (UIntPtr i = 0; i < 3; i++) {																								// Yes, let's just wait a bit before continuing...
			DbgWriteFormated(".");
			PsSleep(500);
		}
		
		DbgWriteFormated("\r\n");
		DbgSetRedirect(False);																											// Disable the redirect feature of the Dbg* functions
	}
	
	ConSetSurface(DispBackBuffer, True, False, 0, 0);																					// Init the console in fullscreen mode
	
	PProcess proc = ExecCreateProcess(L"/System/Programs/osmngr.che");																	// Let's create the initial process
	
	if (proc == Null) {
		DbgWriteFormated("PANIC! Couldn't start the /System/Programs/osmngr.che program\r\n");											// Failed, so let's panic
		Panic(PANIC_OSMNGR_START_FAILED);
	}
	
	PsAddProcess(proc);																													// Add it
	PsWaitProcess(proc->id);																											// And it is never supposed to exit, so this should halt us
	DbgWriteFormated("PANIC! The /System/Programs/osmngr.che program closed\r\n");														// ... Panic
	Panic(PANIC_OSMNGR_PROCESS_CLOSED);
	ArchHalt();
}
