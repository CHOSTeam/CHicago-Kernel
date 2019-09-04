// File author is Ítalo Lima Marconato Matias
//
// Created on May 11 of 2018, at 13:14 BRT
// Last edited on September 04 of 2019, at 19:08 BRT

#include <chicago/alloc.h>
#include <chicago/arch.h>
#include <chicago/config.h>
#include <chicago/console.h>
#include <chicago/debug.h>
#include <chicago/display.h>
#include <chicago/exec.h>
#include <chicago/file.h>
#include <chicago/gui.h>
#include <chicago/ipc.h>
#include <chicago/net.h>
#include <chicago/nls.h>
#include <chicago/panic.h>
#include <chicago/shell.h>
#include <chicago/string.h>
#include <chicago/timer.h>
#include <chicago/version.h>

Void KernelMain(Void) {
	ArchInitDebug();																										// Init the architecture-dependent debugging method
	DbgWriteFormated("[Kernel] Arch debugging initialized\r\n");
	
	DbgWriteFormated("[Kernel] CHicago %s (codename %s, for %s)\r\n", CHICAGO_VSTR_C, CHICAGO_CODENAME_C, CHICAGO_ARCH_C);	// Print the system version
	
	ArchInitFPU();																											// Init the architecture-dependent FPU (floating point unit)
	DbgWriteFormated("[Kernel] Arch FPU initialized\r\n");
	
	ArchInitPMM();																											// Init the physical memory manager
	DbgWriteFormated("[Kernel] Arch PMM initialized\r\n");
	
	ArchInitVMM();																											// Init the virtual memory manager
	DbgWriteFormated("[Kernel] Arch VMM initialized\r\n");
	
	ArchInitDisplay();																										// Init the display
	
	if ((ArchBootOptions & BOOT_OPTIONS_VERBOSE) == BOOT_OPTIONS_VERBOSE) {													// Verbose boot?
		PImage con = ImgCreateBuf(DispGetWidth(), DispGetHeight() - 40, DispBackBuffer->bpp, DispBackBuffer->buf);			// Yes, try to create the console surface

		if (con != Null) {																									// Failed?
			ConSetSurface(con, True, True, 0, 0);																			// No, set the console surface
			DbgSetRedirect(True);																							// Enable the redirect feature of the Dbg* functions
			ConSetColor(0xFF0D3D56, 0xFFFFFFFF);																			// Set the background and foreground colors
			ConSetCursorEnabled(False);																						// Disable the cursor
			ConClearScreen();																								// Clear the screen
		}
	}
	
	DbgWriteFormated("[Kernel] Arch display initialized\r\n");
	
	DispDrawProgessBar();																									// Draw the progress bar
	DbgWriteFormated("[Kernel] The boot progress bar has been shown\r\n");
	
	ArchInitMouse();																										// Init the mouse
	DbgWriteFormated("[Kernel] Arch mouse initialized\r\n");
	
	ArchInitKeyboard();																										// Init the keyboard
	DbgWriteFormated("[Kernel] Arch keyboard intialized\r\n");
	
	ArchInitSc();																											// Init the system call interface
	DbgWriteFormated("[Kernel] Arch system call interface initialized\r\n");
	
	ArchInit();																												// Let's finish it by initalizating the other architecture-dependent bits of the kernel
	DispIncrementProgessBar();
	DbgWriteFormated("[Kernel] Arch initialized\r\n");	
	
	FsInitDevices();																										// Now init the basic devices
	DispIncrementProgessBar();
	DbgWriteFormated("[Kernel] Devices initialized\r\n");
	
	FsInit();																												// Init the filesystem list, mount point list, and add the basic mount points
	DispIncrementProgessBar();
	DbgWriteFormated("[Kernel] Filesystem initialized\r\n");
	
	PsInit();																												// Init tasking
	ArchHalt();																												// Halt
}

Void KernelMainLate(Void) {
	DispIncrementProgessBar();
	DbgWriteFormated("[Kernel] Tasking initialized\r\n");																	// Tasking initialized
	
	IpcInit();																												// Init the IPC interface
	DispIncrementProgessBar();
	DbgWriteFormated("[Kernel] IPC initialized\r\n");
	
	ArchFinishKeyboard();																									// Now we can start handling the keyboard!
	NetFinish();																											// And start handling the network packets
	
	PList conf = ConfLoad(L"System.conf");																					// Load the configuration file, let's set the system language!
	
	if (conf != Null) {																										// Failed?
		PConfField lang = ConfGetField(conf, L"Language");																	// No! Let's get the system language
		
		if (lang != Null) {
			NlsSetLanguage(NlsGetLanguage(lang->value));
		}
		
		ConfFree(conf);																										// Now free the loaded conf file
	}
	
	conf = ConfLoad(L"Drivers.conf");																						// Now, let's load the driver configuration file (contain all the driver that we need to load)
	
	if (conf != Null) {																										// Failed?
		ListForeach(conf, i) {																								// No! Let's load all the drivers!
			PConfField drv = (PConfField)i->data; (Void)drv;																// Ok, for now we don't have anything to do lol
		}
		
		ConfFree(conf);																										// Now free the loaded conf file
	}
	
	DispFillProgressBar();																									// Kernel initialized
	DbgWriteFormated("[Kernel] Kernel initialized\r\n\r\n");
	
	if ((ArchBootOptions & BOOT_OPTIONS_VERBOSE) == BOOT_OPTIONS_VERBOSE) {													// Verbose boot?
		DbgWriteFormated("Press any key to finish the boot process...");													// Yes, wait for any keypress
		WChar discard[2];
		ConsoleDeviceReadKeyboard(1, discard);
	}
	
	DbgSetRedirect(False);																									// Disable the redirect feature of the Dbg* functions, as it may be enabled
	/*ConSetSurface(DispBackBuffer, True, False, 0, 0);																		// Init the console
	ConClearScreen();																										// Clear the screen
	ConWriteFormated(NlsGetMessage(NLS_OS_NAME), CHICAGO_ARCH);																// Print some system informations
	ConWriteFormated(NlsGetMessage(NLS_OS_CODENAME), CHICAGO_CODENAME);
	ConWriteFormated(NlsGetMessage(NLS_OS_VSTR), CHICAGO_MAJOR, CHICAGO_MINOR, CHICAGO_BUILD);*/
	
	GuiInit();																												// Init the gui
	
	/*PProcess proc = ExecCreateProcess(L"\\System\\Programs\\sesmgr.che");													// Let's try to create the session manager process
	
	if (proc == Null) {
		DbgWriteFormated("PANIC! Couldn't open \\System\\Programs\\sesmgr.che\r\n");										// Failed...
		Panic(PANIC_KERNEL_INIT_FAILED);
	}
	
	UIntPtr pid = proc->id;																									// Save the pid of the session manager process
	PsAddProcess(proc);																										// RUN!
	PsWaitProcess(pid);																										// Session manager is not supposed to exit, so this function should never return
	DbgWriteFormated("PANIC! The \\System\\Programs\\sesmgr.che program closed\r\n");										// ...
	Panic(PANIC_KERNEL_UNEXPECTED_ERROR);*/																					// Panic
	
	while (True) { PsSwitchTask(PsDontRequeue); }																			// Don't requeue us
}
